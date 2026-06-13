use std::collections::{HashMap, VecDeque};
use std::sync::Arc;

use teach_core::{
    DeviceKind, EngineBuilderConfig, EngineError, FinishReason, GenerateOutput, GenerateRequest,
    RequestId, SequencePhase, SequenceState, StopCondition, TokenEvent,
};
use teach_cuda::{CudaBackend, CudaEnvironment, StubCudaBackend};
use teach_model::{BatchOutput, DecoderModel, TokenizerRuntime, build_model_runtime_with_impl};
use teach_mode::ModeTracer;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct BlockTable {
    pub request_id: RequestId,
    pub blocks: Vec<usize>,
}

#[derive(Clone, Debug)]
pub struct BlockPool {
    block_size: usize,
    free_blocks: Vec<usize>,
}

impl BlockPool {
    pub fn new(block_size: usize, max_blocks: usize) -> Self {
        let mut free_blocks = Vec::with_capacity(max_blocks);
        for block in (0..max_blocks).rev() {
            free_blocks.push(block);
        }
        Self {
            block_size,
            free_blocks,
        }
    }

    pub fn allocate_table(
        &mut self,
        request_id: RequestId,
        token_capacity: usize,
    ) -> Result<BlockTable, EngineError> {
        let blocks_needed = token_capacity.div_ceil(self.block_size).max(1);
        if self.free_blocks.len() < blocks_needed {
            return Err(EngineError::new("kv cache block pool exhausted"));
        }

        let mut blocks = Vec::with_capacity(blocks_needed);
        for _ in 0..blocks_needed {
            blocks.push(self.free_blocks.pop().expect("checked len before pop"));
        }
        Ok(BlockTable { request_id, blocks })
    }

    pub fn release_table(&mut self, table: &BlockTable) {
        for block in table.blocks.iter().rev() {
            self.free_blocks.push(*block);
        }
    }

    pub fn available_blocks(&self) -> usize {
        self.free_blocks.len()
    }

    pub fn total_blocks(&self) -> usize {
        self.free_blocks.len() + self.used_blocks()
    }

    fn used_blocks(&self) -> usize {
        // 这个值在运行时需要通过追踪分配来维护
        // 简化起见，我们假设可以在 Engine 中维护
        0
    }
}

#[derive(Clone, Debug)]
struct SequenceRuntime {
    request: GenerateRequest,
    state: SequenceState,
    block_table: BlockTable,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ScheduledBatch {
    pub phase: SequencePhase,
    pub request_ids: Vec<RequestId>,
}

#[derive(Debug)]
pub struct Scheduler {
    waiting: VecDeque<RequestId>,
    max_batch_size: usize,
}

impl Scheduler {
    pub fn new(max_batch_size: usize) -> Self {
        Self {
            waiting: VecDeque::new(),
            max_batch_size,
        }
    }

    pub fn enqueue(&mut self, request_id: RequestId) {
        self.waiting.push_back(request_id);
    }

    pub fn next_prefill_batch(&mut self) -> ScheduledBatch {
        let mut request_ids = Vec::new();
        while request_ids.len() < self.max_batch_size {
            let Some(id) = self.waiting.pop_front() else {
                break;
            };
            request_ids.push(id);
        }
        ScheduledBatch {
            phase: SequencePhase::Prefill,
            request_ids,
        }
    }

    fn next_decode_batch(&self, states: &HashMap<RequestId, SequenceRuntime>) -> ScheduledBatch {
        let request_ids = states
            .iter()
            .filter_map(|(id, runtime)| {
                (runtime.state.phase == SequencePhase::Decoding).then_some(*id)
            })
            .take(self.max_batch_size)
            .collect();
        ScheduledBatch {
            phase: SequencePhase::Decoding,
            request_ids,
        }
    }
}

pub struct EngineBuilder {
    config: EngineBuilderConfig,
    tracer: Option<Arc<dyn ModeTracer>>,
}

impl EngineBuilder {
    pub fn new() -> Self {
        Self {
            config: EngineBuilderConfig::default(),
            tracer: None,
        }
    }

    pub fn with_config(mut self, config: EngineBuilderConfig) -> Self {
        self.config = config;
        self
    }

    /// 设置教学模式 tracer
    pub fn with_tracer(mut self, tracer: Arc<dyn ModeTracer>) -> Self {
        self.tracer = Some(tracer);
        self
    }

    pub fn build(self) -> Result<Engine, EngineError> {
        let backend = match self.config.device {
            DeviceKind::Cpu => StubCudaBackend::from_environment(CudaEnvironment {
                cuda_home: String::new(),
                nvcc_path: String::new(),
                arch: self.config.cuda.preferred_arch.clone(),
                note: "cpu-only build; CUDA path not probed".to_string(),
            }),
            DeviceKind::Cuda => StubCudaBackend::probe(&self.config.cuda)?,
        };
        let runtime = build_model_runtime_with_impl(
            &self.config.model,
            self.config.device,
            self.config.weight_format,
            self.config.model_impl,
        )?;

        Ok(Engine {
            scheduler: Scheduler::new(self.config.engine.max_batch_size),
            block_pool: BlockPool::new(
                self.config.engine.block_size,
                self.config.engine.max_blocks,
            ),
            config: self.config,
            runtimes: HashMap::new(),
            tokenizer: runtime.tokenizer,
            model: runtime.model,
            stop_token: runtime.stop_token,
            backend,
            next_request_id: 1,
            tracer: self.tracer,
        })
    }
}

impl Default for EngineBuilder {
    fn default() -> Self {
        Self::new()
    }
}

pub struct Engine {
    config: EngineBuilderConfig,
    scheduler: Scheduler,
    block_pool: BlockPool,
    runtimes: HashMap<RequestId, SequenceRuntime>,
    tokenizer: Box<dyn TokenizerRuntime>,
    model: Box<dyn DecoderModel>,
    stop_token: Option<u32>,
    backend: StubCudaBackend,
    next_request_id: RequestId,
    tracer: Option<Arc<dyn ModeTracer>>,
}

impl Engine {
    pub fn backend_label(&self) -> &'static str {
        self.backend.label()
    }

    pub fn cuda_note(&self) -> &str {
        &self.backend.environment().note
    }

    /// 获取 tracer（如果配置了）
    pub fn tracer(&self) -> Option<&Arc<dyn ModeTracer>> {
        self.tracer.as_ref()
    }

    pub fn submit(
        &mut self,
        prompt: String,
        sampling: teach_core::SamplingParams,
    ) -> Result<RequestId, EngineError> {
        let request_id = self.next_request_id;
        self.next_request_id += 1;
        let prompt_tokens = self.tokenizer.encode(&prompt)?;
        let token_capacity = prompt_tokens.len() + sampling.max_new_tokens;
        let block_table = self.block_pool.allocate_table(request_id, token_capacity)?;
        
        // Trace: KV Cache 分配
        if let Some(tracer) = &self.tracer {
            let blocks_needed = token_capacity.div_ceil(self.config.engine.block_size).max(1);
            tracer.on_kv_cache_allocate(request_id, blocks_needed);
            tracer.on_request_submit(request_id, prompt_tokens.len());
        }
        
        let request = GenerateRequest {
            id: request_id,
            prompt,
            prompt_tokens: prompt_tokens.clone(),
            sampling,
            stop: StopCondition {
                stop_token: self.stop_token,
            },
        };
        let state = SequenceState {
            request_id,
            prompt_tokens,
            generated_tokens: Vec::new(),
            phase: SequencePhase::Waiting,
        };
        self.runtimes.insert(
            request_id,
            SequenceRuntime {
                request,
                state,
                block_table,
            },
        );
        self.scheduler.enqueue(request_id);
        Ok(request_id)
    }

    pub fn run_to_completion(
        &mut self,
        prompt: String,
        sampling: teach_core::SamplingParams,
    ) -> Result<Vec<TokenEvent>, EngineError> {
        self.submit(prompt, sampling)?;
        self.drain()
    }

    pub fn drain(&mut self) -> Result<Vec<TokenEvent>, EngineError> {
        let mut events = Vec::new();

        loop {
            let prefill = self.scheduler.next_prefill_batch();
            if !prefill.request_ids.is_empty() {
                // Trace: 调度决策
                if let Some(tracer) = &self.tracer {
                    tracer.on_schedule(SequencePhase::Prefill, &prefill.request_ids);
                }
                self.run_prefill(prefill.request_ids, &mut events)?;
                continue;
            }

            let decode = self.scheduler.next_decode_batch(&self.runtimes);
            if !decode.request_ids.is_empty() {
                // Trace: 调度决策
                if let Some(tracer) = &self.tracer {
                    tracer.on_schedule(SequencePhase::Decoding, &decode.request_ids);
                }
                self.run_decode(decode.request_ids, &mut events)?;
                continue;
            }

            break;
        }

        Ok(events)
    }

    pub fn benchmark_batch(
        &mut self,
        prompt_len: usize,
        decode_len: usize,
    ) -> Result<GenerateOutput, EngineError> {
        let prompt = (0..prompt_len)
            .map(|idx| format!("w{idx}"))
            .collect::<Vec<_>>()
            .join(" ");
        let events = self.run_to_completion(
            prompt,
            teach_core::SamplingParams {
                max_new_tokens: decode_len,
                ..teach_core::SamplingParams::default()
            },
        )?;

        events
            .into_iter()
            .find_map(|event| match event {
                TokenEvent::Finished {
                    request_id,
                    output_tokens,
                    text,
                    reason,
                } => Some(GenerateOutput {
                    request_id,
                    tokens: output_tokens,
                    text,
                    reason,
                }),
                _ => None,
            })
            .ok_or_else(|| EngineError::new("benchmark did not produce a final output"))
    }

    fn run_prefill(
        &mut self,
        request_ids: Vec<RequestId>,
        events: &mut Vec<TokenEvent>,
    ) -> Result<(), EngineError> {
        // Trace: prefill 开始
        let total_tokens: usize = request_ids
            .iter()
            .filter_map(|id| self.runtimes.get(id))
            .map(|r| r.state.prompt_tokens.len())
            .sum();
        
        if let Some(tracer) = &self.tracer {
            tracer.on_prefill_start(&request_ids, total_tokens);
        }

        let start_time = std::time::Instant::now();
        let states = self.batch_states(&request_ids, SequencePhase::Prefill, events);
        let outputs = self.model.prefill(&states)?;
        let duration_ms = start_time.elapsed().as_secs_f64() * 1000.0;
        
        // Trace: prefill 结束
        if let Some(tracer) = &self.tracer {
            tracer.on_prefill_end(&request_ids, duration_ms);
        }
        
        self.apply_outputs(outputs, SequencePhase::Decoding, events)
    }

    fn run_decode(
        &mut self,
        request_ids: Vec<RequestId>,
        events: &mut Vec<TokenEvent>,
    ) -> Result<(), EngineError> {
        // Trace: decode 开始
        if let Some(tracer) = &self.tracer {
            tracer.on_decode_start(&request_ids);
        }

        let start_time = std::time::Instant::now();
        let states = self.batch_states(&request_ids, SequencePhase::Decoding, events);
        let outputs = self.model.decode(&states)?;
        let duration_ms = start_time.elapsed().as_secs_f64() * 1000.0;
        
        // Trace: decode 结束
        if let Some(tracer) = &self.tracer {
            tracer.on_decode_end(&request_ids, duration_ms);
        }
        
        self.apply_outputs(outputs, SequencePhase::Decoding, events)
    }

    fn batch_states(
        &mut self,
        request_ids: &[RequestId],
        phase: SequencePhase,
        events: &mut Vec<TokenEvent>,
    ) -> Vec<SequenceState> {
        let mut states = Vec::new();
        for request_id in request_ids {
            if let Some(runtime) = self.runtimes.get_mut(request_id) {
                runtime.state.phase = phase;
                if phase == SequencePhase::Prefill {
                    events.push(TokenEvent::Started {
                        request_id: *request_id,
                        prompt_len: runtime.state.prompt_tokens.len(),
                    });
                }
                states.push(runtime.state.clone());
            }
        }
        states
    }

    fn apply_outputs(
        &mut self,
        outputs: Vec<BatchOutput>,
        next_phase: SequencePhase,
        events: &mut Vec<TokenEvent>,
    ) -> Result<(), EngineError> {
        let mut finished = Vec::new();

        for output in outputs {
            let Some(runtime) = self.runtimes.get_mut(&output.request_id) else {
                return Err(EngineError::new("model returned output for unknown request"));
            };

            runtime.state.generated_tokens.push(output.next_token);
            let text = self.tokenizer.decode_token(output.next_token)?;
            
            // Trace: token 生成
            if let Some(tracer) = &self.tracer {
                tracer.on_token_generated(output.request_id, output.next_token, runtime.state.phase);
            }
            
            events.push(TokenEvent::Token {
                request_id: output.request_id,
                token_id: output.next_token,
                text,
                phase: runtime.state.phase,
            });

            let reached_max =
                runtime.state.generated_tokens.len() >= runtime.request.sampling.max_new_tokens;
            let hit_stop = runtime.request.stop.stop_token == Some(output.next_token);

            if reached_max || hit_stop {
                let reason = if hit_stop {
                    FinishReason::StopToken
                } else {
                    FinishReason::MaxTokens
                };
                runtime.state.phase = SequencePhase::Finished;
                finished.push((output.request_id, reason));
            } else {
                runtime.state.phase = next_phase;
            }
        }

        for (request_id, reason) in finished {
            let runtime = self
                .runtimes
                .remove(&request_id)
                .ok_or_else(|| EngineError::new("request disappeared before finish"))?;
            self.block_pool.release_table(&runtime.block_table);
            
            // Trace: KV Cache 释放和请求完成
            if let Some(tracer) = &self.tracer {
                tracer.on_kv_cache_release(request_id, runtime.block_table.blocks.len());
                tracer.on_request_finish(request_id, &format!("{:?}", reason));
            }
            
            events.push(TokenEvent::Finished {
                request_id,
                output_tokens: runtime.state.generated_tokens.clone(),
                text: self
                    .tokenizer
                    .decode_tokens(&runtime.state.generated_tokens)?,
                reason,
            });
        }

        Ok(())
    }

    pub fn describe(&self) -> String {
        format!(
            "model_id={} context_len={} device={:?} weight_format={:?} backend={} free_blocks={}",
            self.config.model.model_id,
            self.config.model.context_len,
            self.config.device,
            self.config.weight_format,
            self.backend_label(),
            self.block_pool.available_blocks(),
        )
    }

    /// 获取教学模式可视化输出（如果配置了 tracer）
    pub fn visual_output(&self) -> String {
        self.tracer
            .as_ref()
            .map(|t| t.visual_output())
            .unwrap_or_default()
    }

    /// 获取性能指标（如果配置了 tracer）
    pub fn metrics(&self) -> Option<teach_mode::MetricsSnapshot> {
        self.tracer.as_ref().and_then(|t| t.metrics())
    }

    /// 导出 trace JSON（如果配置了 tracer）
    pub fn export_trace(&self) -> Result<String, EngineError> {
        self.tracer
            .as_ref()
            .map(|t| t.export_trace_json())
            .unwrap_or(Ok("[]".to_string()))
    }
}
