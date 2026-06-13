//! 可视化模块 - ASCII 图形输出

use std::collections::HashMap;
use std::sync::Mutex;

use teach_core::{RequestId, SequencePhase};

/// 可视化配置
#[derive(Clone, Debug, PartialEq)]
pub struct VisualConfig {
    /// 时间线宽度（字符数）
    pub timeline_width: usize,
    /// 时间线高度
    pub timeline_height: usize,
    /// 内存图宽度
    pub memory_width: usize,
    /// 是否显示 token 详情
    pub show_token_details: bool,
    /// 颜色输出（如果终端支持）
    pub use_colors: bool,
}

impl Default for VisualConfig {
    fn default() -> Self {
        Self {
            timeline_width: 80,
            timeline_height: 20,
            memory_width: 60,
            show_token_details: false,
            use_colors: true,
        }
    }
}

/// 时间线事件
#[derive(Clone, Debug)]
struct TimelineEvent {
    timestamp_ms: u64,
    request_id: RequestId,
    event_type: EventType,
}

#[derive(Clone, Debug)]
enum EventType {
    Submit,
    Prefill,
    Decode,
    Token,
    Finish,
    KvAllocate, // blocks
    KvRelease,  // blocks
}

/// 请求状态追踪
#[derive(Clone, Debug, Default)]
struct RequestState {
    submit_time: Option<u64>,
    _prefill_time: Option<u64>,
    first_token_time: Option<u64>,
    finish_time: Option<u64>,
    tokens_generated: usize,
    kv_blocks: usize,
}

/// 时间线可视化器
pub struct TimelineVisualizer {
    config: VisualConfig,
    events: Mutex<Vec<TimelineEvent>>,
    requests: Mutex<HashMap<RequestId, RequestState>>,
    start_time_ms: Mutex<u64>,
}

impl TimelineVisualizer {
    /// 创建新的 TimelineVisualizer
    pub fn new(config: VisualConfig) -> Self {
        Self {
            config,
            events: Mutex::new(Vec::new()),
            requests: Mutex::new(HashMap::new()),
            start_time_ms: Mutex::new(0),
        }
    }

    fn now_ms(&self) -> u64 {
        std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap_or_default()
            .as_millis() as u64
    }

    fn ensure_start_time(&self) {
        let mut start = self.start_time_ms.lock().unwrap();
        if *start == 0 {
            *start = self.now_ms();
        }
    }

    fn elapsed_ms(&self) -> u64 {
        self.ensure_start_time();
        let start = *self.start_time_ms.lock().unwrap();
        self.now_ms().saturating_sub(start)
    }

    /// 记录请求提交
    pub fn on_request_submit(&self, request_id: RequestId, _prompt_len: usize) {
        let elapsed = self.elapsed_ms();
        let mut events = self.events.lock().unwrap();
        let mut requests = self.requests.lock().unwrap();

        events.push(TimelineEvent {
            timestamp_ms: elapsed,
            request_id,
            event_type: EventType::Submit,
        });

        let mut state = RequestState::default();
        state.submit_time = Some(elapsed);
        requests.insert(request_id, state);
    }

    /// 记录调度
    pub fn on_schedule(&self, phase: SequencePhase, request_ids: &[RequestId]) {
        let elapsed = self.elapsed_ms();
        let mut events = self.events.lock().unwrap();

        for request_id in request_ids {
            let event_type = match phase {
                SequencePhase::Prefill => EventType::Prefill,
                SequencePhase::Decoding => EventType::Decode,
                _ => continue,
            };
            events.push(TimelineEvent {
                timestamp_ms: elapsed,
                request_id: *request_id,
                event_type,
            });
        }
    }

    /// 记录 token 生成
    pub fn on_token_generated(&self, request_id: RequestId, _token_id: u32, phase: SequencePhase) {
        let elapsed = self.elapsed_ms();
        let mut events = self.events.lock().unwrap();
        let mut requests = self.requests.lock().unwrap();

        events.push(TimelineEvent {
            timestamp_ms: elapsed,
            request_id,
            event_type: EventType::Token,
        });

        if let Some(state) = requests.get_mut(&request_id) {
            if state.first_token_time.is_none() && phase == SequencePhase::Prefill {
                state.first_token_time = Some(elapsed);
            }
            state.tokens_generated += 1;
        }
    }

    /// 记录请求完成
    pub fn on_request_finish(&self, request_id: RequestId, _reason: &str) {
        let elapsed = self.elapsed_ms();
        let mut events = self.events.lock().unwrap();
        let mut requests = self.requests.lock().unwrap();

        events.push(TimelineEvent {
            timestamp_ms: elapsed,
            request_id,
            event_type: EventType::Finish,
        });

        if let Some(state) = requests.get_mut(&request_id) {
            state.finish_time = Some(elapsed);
        }
    }

    /// 记录 KV Cache 分配
    pub fn on_kv_cache_allocate(&self, request_id: RequestId, blocks: usize) {
        let elapsed = self.elapsed_ms();
        let mut events = self.events.lock().unwrap();
        let mut requests = self.requests.lock().unwrap();

        events.push(TimelineEvent {
            timestamp_ms: elapsed,
            request_id,
            event_type: EventType::KvAllocate,
        });

        if let Some(state) = requests.get_mut(&request_id) {
            state.kv_blocks += blocks;
        }
    }

    /// 记录 KV Cache 释放
    pub fn on_kv_cache_release(&self, request_id: RequestId, blocks: usize) {
        let elapsed = self.elapsed_ms();
        let mut events = self.events.lock().unwrap();
        let mut requests = self.requests.lock().unwrap();

        events.push(TimelineEvent {
            timestamp_ms: elapsed,
            request_id,
            event_type: EventType::KvRelease,
        });

        if let Some(state) = requests.get_mut(&request_id) {
            state.kv_blocks = state.kv_blocks.saturating_sub(blocks);
        }
    }

    /// 渲染时间线
    pub fn render(&self) -> String {
        let events = self.events.lock().unwrap();
        let requests = self.requests.lock().unwrap();

        if events.is_empty() {
            return "[No events recorded]".to_string();
        }

        let mut output = String::new();
        output.push_str("=== Execution Timeline ===\n\n");

        // 计算时间范围
        let min_time = events.iter().map(|e| e.timestamp_ms).min().unwrap_or(0);
        let max_time = events.iter().map(|e| e.timestamp_ms).max().unwrap_or(0);
        let time_range = max_time.saturating_sub(min_time).max(1);

        // 渲染每个请求的时间线
        let request_ids: Vec<_> = requests.keys().copied().collect();
        for request_id in request_ids {
            output.push_str(&format!("Request {:2}: ", request_id));

            // 收集该请求的所有事件
            let request_events: Vec<_> = events
                .iter()
                .filter(|e| e.request_id == request_id)
                .collect();

            // 创建时间线字符数组
            let width = self.config.timeline_width.saturating_sub(15);
            let mut timeline = vec!['.'; width];

            // 如果所有事件都挤在很小的 time_range 内，按顺序均匀分布
            let distinct_times: std::collections::BTreeSet<u64> = request_events
                .iter()
                .map(|e| e.timestamp_ms)
                .collect();
            let use_uniform = distinct_times.len() <= 2 || time_range < width as u64 / 2;

            let mut sorted_events = request_events.clone();
            sorted_events.sort_by_key(|e| e.timestamp_ms);

            for (idx, event) in sorted_events.iter().enumerate() {
                let pos = if use_uniform {
                    // 均匀分布，给每个重要事件一个独立位置
                    let important_count = sorted_events
                        .iter()
                        .filter(|e| {
                            matches!(
                                e.event_type,
                                EventType::Submit
                                    | EventType::Prefill
                                    | EventType::Decode
                                    | EventType::Token
                                    | EventType::Finish
                            )
                        })
                        .count();
                    let step = width as f64 / important_count.max(1) as f64;
                    let important_idx = sorted_events[..=idx]
                        .iter()
                        .filter(|e| {
                            matches!(
                                e.event_type,
                                EventType::Submit
                                    | EventType::Prefill
                                    | EventType::Decode
                                    | EventType::Token
                                    | EventType::Finish
                            )
                        })
                        .count()
                        .saturating_sub(1);
                    (important_idx as f64 * step) as usize
                } else {
                    let p = ((event.timestamp_ms - min_time) as f64 / time_range as f64
                        * width as f64) as usize;
                    p.min(width - 1)
                };
                let pos = pos.min(width - 1);

                let ch = match &event.event_type {
                    EventType::Submit => 'S',
                    EventType::Prefill => 'P',
                    EventType::Decode => 'D',
                    EventType::Token => '*',
                    EventType::Finish => 'F',
                    EventType::KvAllocate => 'A',
                    EventType::KvRelease => 'R',
                };
                // 按事件重要性分配优先级，避免密集调度事件覆盖关键 token
                let priority = match ch {
                    '*' | 'F' => 3,
                    'P' | 'D' => 2,
                    _ => 1,
                };
                let current_priority = match timeline[pos] {
                    '*' | 'F' => 3,
                    'P' | 'D' => 2,
                    _ => 1,
                };
                if priority >= current_priority {
                    timeline[pos] = ch;
                }
            }

            output.push_str(&timeline.into_iter().collect::<String>());

            // 添加统计信息
            if let Some(state) = requests.get(&request_id) {
                output.push_str(&format!(
                    "  ({} tokens)\n",
                    state.tokens_generated
                ));
            } else {
                output.push('\n');
            }
        }

        // 添加图例
        output.push_str("\nLegend: S=Submit P=Prefill D=Decode *=Token F=Finish A=Alloc R=Release\n");

        output
    }

    /// 渲染简短时间线摘要
    pub fn render_summary(&self) -> String {
        let requests = self.requests.lock().unwrap();
        let events = self.events.lock().unwrap();

        let total_requests = requests.len();
        let total_tokens: usize = requests.values().map(|r| r.tokens_generated).sum();
        let total_events = events.len();

        format!(
            "Timeline: {} requests, {} tokens, {} events",
            total_requests, total_tokens, total_events
        )
    }
}

impl Default for TimelineVisualizer {
    fn default() -> Self {
        Self::new(VisualConfig::default())
    }
}

/// 批次状态可视化器
pub struct BatchVisualizer {
    config: VisualConfig,
}

impl BatchVisualizer {
    /// 创建新的 BatchVisualizer
    pub fn new(config: VisualConfig) -> Self {
        Self { config }
    }

    /// 渲染批次状态图
    pub fn render_batch_state(&self, batch_sizes: &[(String, usize)]) -> String {
        let mut output = String::new();
        output.push_str("=== Batch Size Over Time ===\n\n");

        let max_size = batch_sizes.iter().map(|(_, s)| *s).max().unwrap_or(1).max(1);
        let height = self.config.timeline_height.min(10);

        for row in (0..height).rev() {
            let threshold = (row as f64 / height as f64 * max_size as f64) as usize;
            output.push_str(&format!("{:3} | ", threshold));

            for (_, size) in batch_sizes.iter().take(self.config.timeline_width / 3) {
                let ch = if *size >= threshold { '█' } else { ' ' };
                output.push(ch);
            }
            output.push('\n');
        }

        output.push_str("    +");
        output.push_str(&"-".repeat(batch_sizes.len().min(self.config.timeline_width / 3)));
        output.push('\n');

        output
    }

    /// 渲染批次组成
    pub fn render_batch_composition(&self, batches: &[Vec<RequestId>]) -> String {
        let mut output = String::new();
        output.push_str("=== Batch Composition ===\n\n");

        for (i, batch) in batches.iter().enumerate() {
            output.push_str(&format!("Batch {:2}: [", i));
            for (j, request_id) in batch.iter().enumerate() {
                if j > 0 {
                    output.push_str(", ");
                }
                output.push_str(&format!("R{}", request_id));
            }
            output.push_str(&format!("] ({} requests)\n", batch.len()));
        }

        output
    }
}

impl Default for BatchVisualizer {
    fn default() -> Self {
        Self::new(VisualConfig::default())
    }
}

/// 内存使用可视化器
pub struct MemoryVisualizer {
    config: VisualConfig,
}

impl MemoryVisualizer {
    /// 创建新的 MemoryVisualizer
    pub fn new(config: VisualConfig) -> Self {
        Self { config }
    }

    /// 渲染 KV Cache 状态
    pub fn render_kv_cache(&self, used: usize, free: usize) -> String {
        let total = used + free;
        if total == 0 {
            return "[No KV Cache allocated]".to_string();
        }

        let mut output = String::new();
        output.push_str("=== KV Cache Usage ===\n\n");

        let width = self.config.memory_width.min(50);
        let used_chars = (used as f64 / total as f64 * width as f64) as usize;

        output.push_str("[");
        output.push_str(&"█".repeat(used_chars));
        output.push_str(&"░".repeat(width - used_chars));
        output.push_str(&format!(
            "]  {}/{} blocks ({:.1}%)\n",
            used,
            total,
            used as f64 / total as f64 * 100.0
        ));

        output.push_str(&format!("Used: {} blocks | Free: {} blocks\n", used, free));

        output
    }

    /// 渲染内存时间线
    pub fn render_memory_timeline(&self, usage_history: &[(u64, usize, usize)]) -> String {
        let mut output = String::new();
        output.push_str("=== Memory Usage Timeline ===\n\n");

        if usage_history.is_empty() {
            return output;
        }

        let max_used = usage_history.iter().map(|(_, u, _)| *u).max().unwrap_or(1);
        let height = 5;

        for row in (0..height).rev() {
            let threshold = (row as f64 / height as f64 * max_used as f64) as usize;
            output.push_str(&format!("{:4} | ", threshold));

            for (_, used, _) in usage_history.iter().take(self.config.timeline_width / 2) {
                let ch = if *used >= threshold { '▓' } else { ' ' };
                output.push(ch);
            }
            output.push('\n');
        }

        output.push_str("     +");
        output.push_str(&"-".repeat(usage_history.len().min(self.config.timeline_width / 2)));
        output.push('\n');

        output
    }
}

impl Default for MemoryVisualizer {
    fn default() -> Self {
        Self::new(VisualConfig::default())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_timeline_visualizer() {
        let viz = TimelineVisualizer::default();

        viz.on_request_submit(1, 10);
        viz.on_kv_cache_allocate(1, 4);
        viz.on_schedule(SequencePhase::Prefill, &[1]);
        viz.on_token_generated(1, 100, SequencePhase::Prefill);
        viz.on_token_generated(1, 101, SequencePhase::Decoding);
        viz.on_request_finish(1, "done");

        let output = viz.render();
        assert!(output.contains("Execution Timeline"));
        assert!(output.contains("Request"));
    }

    #[test]
    fn test_batch_visualizer() {
        let viz = BatchVisualizer::default();

        let batch_sizes = vec![
            ("prefill".to_string(), 2),
            ("decode".to_string(), 3),
            ("decode".to_string(), 2),
        ];
        let output = viz.render_batch_state(&batch_sizes);
        assert!(output.contains("Batch Size"));

        let batches = vec![vec![1, 2], vec![1, 2, 3], vec![2, 3]];
        let output = viz.render_batch_composition(&batches);
        assert!(output.contains("Batch Composition"));
        assert!(output.contains("R1"));
    }

    #[test]
    fn test_memory_visualizer() {
        let viz = MemoryVisualizer::default();

        let output = viz.render_kv_cache(40, 24);
        assert!(output.contains("KV Cache"));
        assert!(output.contains("40/64"));

        let history = vec![(0, 10, 54), (100, 20, 44), (200, 30, 34)];
        let output = viz.render_memory_timeline(&history);
        assert!(output.contains("Memory Usage"));
    }
}
