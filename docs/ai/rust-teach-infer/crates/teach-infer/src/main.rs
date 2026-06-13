use std::env;
use std::io::{self, Write};
use std::path::PathBuf;
use std::process;
use std::sync::Arc;

use teach_core::{DeviceKind, EngineBuilderConfig, ModelImplementation, SamplingParams, TokenEvent, WeightFormat};
use teach_engine::EngineBuilder;
use teach_model::prepare_assets;
use teach_mode::{CompositeTracer, TeachModeConfig, TraceLevel};

fn main() {
    if let Err(err) = run() {
        eprintln!("[FAIL] {err}");
        process::exit(1);
    }
}

fn run() -> Result<(), Box<dyn std::error::Error>> {
    let mut args = env::args().skip(1);
    let Some(command) = args.next() else {
        print_help();
        return Ok(());
    };

    match command.as_str() {
        "prepare-model" => {
            let config = parse_engine_config(args.collect::<Vec<_>>().as_slice())?;
            let assets = prepare_assets(&config.model)?;
            println!("[OK] prepared model assets in {}", assets.model_dir.display());
            println!("[OK] config={}", assets.config_path.display());
            println!("[OK] tokenizer={}", assets.tokenizer_path.display());
            println!("[OK] weights={}", assets.weight_paths.len());
            if assets.weight_paths.is_empty() {
                println!("[WARN] no weights found. Options:");
                println!("  1. Manual download from: https://hf-mirror.com/Qwen/Qwen2.5-0.5B");
                println!("  2. Use Python script: python scripts/generate_test_weights.py");
                println!("  3. Use: export HF_ENDPOINT=https://hf-mirror.com && teach-infer prepare-model ...");
            }
            Ok(())
        }

        "run" => {
            let rest = args.collect::<Vec<_>>();
            let prompt = parse_flag(&rest, "--prompt").unwrap_or_else(|| "hello world".into());
            let max_new_tokens = parse_flag(&rest, "--max-new-tokens")
                .and_then(|v| v.parse::<usize>().ok())
                .unwrap_or(8);
            let teach_mode = parse_flag(&rest, "--teach-mode");
            let config = parse_engine_config(&rest)?;
            
            // 创建 tracer（如果启用了教学模式）
            let tracer = create_tracer(&teach_mode)?;
            
            let mut builder = EngineBuilder::new().with_config(config);
            if let Some(t) = tracer {
                builder = builder.with_tracer(t);
            }
            let mut engine = builder.build()?;
            
            let events = engine.run_to_completion(
                prompt,
                SamplingParams {
                    max_new_tokens,
                    ..SamplingParams::default()
                },
            )?;
            
            print_events(&events);
            
            // 输出教学模式结果
            if teach_mode.is_some() {
                println!("\n{}", engine.visual_output());
                if let Some(metrics) = engine.metrics() {
                    println!("\nMetrics collected for {} requests", metrics.requests.len());
                }
            }
            
            Ok(())
        }
        "chat" => {
            let rest = args.collect::<Vec<_>>();
            let config = parse_engine_config(&rest)?;
            let mut engine = EngineBuilder::new().with_config(config).build()?;
            println!("[OK] {}", engine.describe());
            println!("[OK] type an empty line to exit");
            loop {
                print!("prompt> ");
                io::stdout().flush()?;
                let mut line = String::new();
                io::stdin().read_line(&mut line)?;
                let line = line.trim().to_string();
                if line.is_empty() {
                    break;
                }
                let events = engine.run_to_completion(line, SamplingParams::default())?;
                print_events(&events);
            }
            Ok(())
        }
        "bench" => {
            let rest = args.collect::<Vec<_>>();
            let prompt_len = parse_flag(&rest, "--prompt-len")
                .and_then(|v| v.parse::<usize>().ok())
                .unwrap_or(16);
            let decode_len = parse_flag(&rest, "--decode-len")
                .and_then(|v| v.parse::<usize>().ok())
                .unwrap_or(8);
            let teach_mode = parse_flag(&rest, "--teach-mode");
            let config = parse_engine_config(&rest)?;
            
            // 创建 tracer（如果启用了教学模式）
            let tracer = create_tracer(&teach_mode)?;
            
            let mut builder = EngineBuilder::new().with_config(config);
            if let Some(t) = tracer {
                builder = builder.with_tracer(t);
            }
            let mut engine = builder.build()?;
            
            let output = engine.benchmark_batch(prompt_len, decode_len)?;
            println!(
                "[OK] request={} generated={} reason={:?}",
                output.request_id,
                output.tokens.len(),
                output.reason
            );
            println!("{}", output.text);
            
            // 输出教学模式结果
            if teach_mode.is_some() {
                println!("\n{}", engine.visual_output());
                if let Some(metrics) = engine.metrics() {
                    println!("\n=== Performance Metrics ===");
                    println!("Total requests: {}", metrics.summary.total_requests);
                    if let Some(ttft) = metrics.summary.avg_ttft_ms {
                        println!("Avg TTFT: {:.2} ms", ttft);
                    }
                    if let Some(tps) = metrics.summary.avg_throughput_tps {
                        println!("Avg throughput: {:.2} tokens/s", tps);
                    }
                    if let Some(itl) = metrics.summary.avg_itl_ms {
                        println!("Avg inter-token latency: {:.2} ms", itl);
                    }
                    println!("Peak batch size: {}", metrics.summary.peak_batch_size);
                }
            }
            
            Ok(())
        }
        "profile" => {
            let rest = args.collect::<Vec<_>>();
            let prompt_len = parse_flag(&rest, "--prompt-len")
                .and_then(|v| v.parse::<usize>().ok())
                .unwrap_or(64);
            let decode_len = parse_flag(&rest, "--decode-len")
                .and_then(|v| v.parse::<usize>().ok())
                .unwrap_or(32);
            let config = parse_engine_config(&rest)?;
            
            // profile 模式自动启用完整教学模式
            let tracer = Arc::new(CompositeTracer::full());
            let mut engine = EngineBuilder::new()
                .with_config(config)
                .with_tracer(tracer)
                .build()?;
            
            println!("[OK] profiling with prompt_len={} decode_len={}", prompt_len, decode_len);
            let output = engine.benchmark_batch(prompt_len, decode_len)?;
            
            println!("\n=== Profile Results ===");
            println!("Request ID: {}", output.request_id);
            println!("Generated tokens: {}", output.tokens.len());
            println!("Finish reason: {:?}", output.reason);
            
            // 输出详细指标
            if let Some(metrics) = engine.metrics() {
                println!("\n{}", format_metrics(&metrics));
            }
            
            // 输出时间线
            println!("\n{}", engine.visual_output());
            
            // 可选：导出 trace
            if parse_flag(&rest, "--export-trace").is_some() {
                let trace_json = engine.export_trace()?;
                println!("\n[OK] trace exported ({} bytes)", trace_json.len());
            }
            
            Ok(())
        }
        "teach" => {
            let rest = args.collect::<Vec<_>>();
            let scenario = parse_flag(&rest, "--scenario").unwrap_or_else(|| "batching".into());
            run_teach_scenario(&scenario)?;
            Ok(())
        }
        _ => {
            print_help();
            Ok(())
        }
    }
}

/// 创建教学模式 tracer
fn create_tracer(teach_mode: &Option<String>) -> Result<Option<Arc<dyn teach_mode::ModeTracer>>, Box<dyn std::error::Error>> {
    let Some(mode) = teach_mode else {
        return Ok(None);
    };
    
    let tracer: Arc<dyn teach_mode::ModeTracer> = match mode.as_str() {
        "trace" => Arc::new(CompositeTracer::trace_only(TraceLevel::Debug)),
        "visual" => Arc::new(CompositeTracer::visual_only()),
        "full" => Arc::new(CompositeTracer::full()),
        "summary" => {
            let config = TeachModeConfig {
                trace_level: TraceLevel::Off,
                enable_step: false,
                enable_visual: false,
                enable_metrics: true,
                ..Default::default()
            };
            Arc::new(CompositeTracer::new(&config))
        }
        _ => {
            return Err(format!("unknown teach mode: {}. Use: trace, visual, full, summary", mode).into());
        }
    };
    
    Ok(Some(tracer))
}

/// 运行教学场景
fn run_teach_scenario(scenario: &str) -> Result<(), Box<dyn std::error::Error>> {
    println!("=== Teaching Scenario: {} ===\n", scenario);
    
    let tracer = Arc::new(CompositeTracer::full());
    let config = EngineBuilderConfig::default();
    let mut engine = EngineBuilder::new()
        .with_config(config)
        .with_tracer(tracer)
        .build()?;
    
    match scenario {
        "batching" => {
            println!("演示批处理机制：");
            println!("1. 提交多个请求到等待队列");
            println!("2. 调度器将它们组合成批次");
            println!("3. 执行 prefill 和 decode\n");
            
            // 提交多个请求
            for i in 1..=3 {
                engine.run_to_completion(
                    format!("request {}", i),
                    SamplingParams {
                        max_new_tokens: 4,
                        ..SamplingParams::default()
                    },
                )?;
            }
        }
        "kv_cache" => {
            println!("演示 KV Cache 管理：");
            println!("1. 请求提交时分配 KV Cache 块");
            println!("2. Paged KV Cache 跟踪块分配");
            println!("3. 请求完成时释放块\n");
            
            engine.run_to_completion(
                "hello world".to_string(),
                SamplingParams {
                    max_new_tokens: 8,
                    ..SamplingParams::default()
                },
            )?;
        }
        "continuous_batching" => {
            println!("演示连续批处理：");
            println!("1. 第一个请求开始 prefill");
            println!("2. 新请求到达时动态加入批次");
            println!("3. 已完成的请求被移出批次\n");
            
            // 当前引擎不支持真正的连续批处理，模拟一下
            engine.run_to_completion(
                "continuous batching demo".to_string(),
                SamplingParams {
                    max_new_tokens: 6,
                    ..SamplingParams::default()
                },
            )?;
        }
        _ => {
            println!("Unknown scenario: {}", scenario);
            println!("Available scenarios: batching, kv_cache, continuous_batching");
            return Ok(());
        }
    }
    
    // 输出可视化
    println!("\n{}", engine.visual_output());
    
    if let Some(metrics) = engine.metrics() {
        println!("\n{}", format_metrics(&metrics));
    }
    
    Ok(())
}

/// 格式化指标输出
fn format_metrics(metrics: &teach_mode::MetricsSnapshot) -> String {
    let mut output = String::new();
    output.push_str("=== Performance Metrics ===\n");
    output.push_str(&format!("Total requests: {}\n", metrics.summary.total_requests));
    output.push_str(&format!("Peak batch size: {}\n", metrics.summary.peak_batch_size));
    output.push_str(&format!(
        "Total tokens (prefill/decode): {}/{}\n",
        metrics.summary.total_prefill_tokens, metrics.summary.total_decode_tokens
    ));
    
    if let Some(ttft) = metrics.summary.avg_ttft_ms {
        output.push_str(&format!("Avg TTFT: {:.2} ms\n", ttft));
    }
    if let Some(tps) = metrics.summary.avg_throughput_tps {
        output.push_str(&format!("Avg throughput: {:.2} tokens/s\n", tps));
    }
    if let Some(itl) = metrics.summary.avg_itl_ms {
        output.push_str(&format!("Avg inter-token latency: {:.2} ms\n", itl));
    }
    
    output.push_str(&format!(
        "KV Cache utilization: {:.1}%\n",
        metrics.memory.kv_cache_utilization * 100.0
    ));
    
    output
}

fn parse_engine_config(args: &[String]) -> Result<EngineBuilderConfig, Box<dyn std::error::Error>> {
    let mut config = EngineBuilderConfig::default();
    if let Some(model_id) = parse_flag(args, "--model-id") {
        config.model.model_id = model_id;
    }
    if let Some(model_dir) = parse_flag(args, "--model-dir") {
        config.model.model_dir = PathBuf::from(model_dir);
    }
    if let Some(context_len) = parse_flag(args, "--context-len").and_then(|v| v.parse().ok()) {
        config.model.context_len = context_len;
    }
    if let Some(device) = parse_flag(args, "--device") {
        config.device = match device.as_str() {
            "cpu" => DeviceKind::Cpu,
            "cuda" => DeviceKind::Cuda,
            other => return Err(format!("unsupported device: {other}").into()),
        };
    }
    if let Some(weight_format) = parse_flag(args, "--weight-format") {
        config.weight_format = match weight_format.as_str() {
            "fp32" => WeightFormat::Fp32,
            "fp16" => WeightFormat::Fp16,
            "bf16" => WeightFormat::Bf16,
            "int8" => WeightFormat::Int8,
            other => return Err(format!("unsupported weight format: {other}").into()),
        };
    }
    if parse_flag(args, "--real-weights").is_some() {
        config.model_impl = ModelImplementation::Real;
    }
    Ok(config)
}

fn parse_flag(args: &[String], flag: &str) -> Option<String> {
    args.windows(2).find_map(|pair| {
        if pair[0] == flag {
            Some(pair[1].clone())
        } else {
            None
        }
    })
}

fn print_events(events: &[TokenEvent]) {
    for event in events {
        match event {
            TokenEvent::Started {
                request_id,
                prompt_len,
            } => println!("[START] request={request_id} prompt_len={prompt_len}"),
            TokenEvent::Token {
                request_id,
                token_id,
                text,
                phase,
            } => println!(
                "[TOKEN] request={request_id} phase={phase:?} token={token_id} text={text}"
            ),
            TokenEvent::Finished {
                request_id,
                output_tokens,
                text,
                reason,
            } => println!(
                "[DONE] request={request_id} generated={} reason={reason:?} text={text}",
                output_tokens.len()
            ),
            TokenEvent::Error {
                request_id,
                message,
            } => println!("[ERROR] request={request_id} {message}"),
        }
    }
}

fn print_help() {
    println!("teach-infer commands:");
    println!("  prepare-model [--model-id <id>] [--model-dir <path>]");
    println!("  run --prompt <text> [--device cpu|cuda] [--weight-format fp32|fp16|bf16|int8] [--real-weights] [--teach-mode trace|visual|full|summary]");
    println!("  chat [--device cpu|cuda] [--weight-format fp32|fp16] [--real-weights]");
    println!("  bench [--prompt-len <n>] [--decode-len <n>] [--real-weights] [--teach-mode <mode>]");
    println!("  profile [--prompt-len <n>] [--decode-len <n>] [--real-weights] [--export-trace]");
    println!("  teach --scenario <name>  (scenarios: batching, kv_cache, continuous_batching)");
    println!();
    println!("Network issues? Use: export HF_ENDPOINT=https://hf-mirror.com");
    println!("GPU run? Use: ./scripts/with_cuda_env.sh cargo run -p teach-infer --features cuda -- run --device cuda --real-weights --weight-format fp16");
    println!("See MODEL_DOWNLOAD.md for manual download instructions");
}
