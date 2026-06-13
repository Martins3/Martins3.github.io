//! 性能指标收集模块

use std::collections::HashMap;
use std::sync::Mutex;
use std::time::Instant;

use serde::{Deserialize, Serialize};
use teach_core::{RequestId, SequencePhase};

/// 单个请求的性能指标
#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct RequestMetrics {
    /// 请求 ID
    pub request_id: RequestId,
    /// Prompt token 数量
    pub prompt_len: usize,
    /// 生成的 token 数量
    pub generated_tokens: usize,
    /// TTFT (Time To First Token) 毫秒
    pub ttft_ms: Option<f64>,
    /// 总延迟（从提交到完成）毫秒
    pub total_latency_ms: Option<f64>,
    /// Prefill 阶段总耗时毫秒
    pub prefill_time_ms: f64,
    /// Decode 阶段总耗时毫秒
    pub decode_time_ms: f64,
    /// Inter-token 延迟平均值（decode 阶段）
    pub inter_token_latency_ms: Option<f64>,
    /// 吞吐量 tokens/second
    pub throughput_tps: Option<f64>,
    /// 开始时间戳
    pub start_time_ms: u64,
    /// 首 token 时间戳
    pub first_token_time_ms: Option<u64>,
    /// 完成时间戳
    pub end_time_ms: Option<u64>,
}

impl RequestMetrics {
    /// 计算 throughput
    fn calculate_throughput(&mut self) {
        if let Some(end) = self.end_time_ms {
            let total_time_sec = (end - self.start_time_ms) as f64 / 1000.0;
            if total_time_sec > 0.0 {
                self.throughput_tps = Some(self.generated_tokens as f64 / total_time_sec);
            }
        }
    }

    /// 计算 inter-token latency
    fn calculate_itl(&mut self) {
        if self.generated_tokens > 1 {
            let decode_tokens = self.generated_tokens.saturating_sub(1);
            if decode_tokens > 0 {
                self.inter_token_latency_ms =
                    Some(self.decode_time_ms / decode_tokens as f64);
            }
        }
    }
}

/// 批次性能指标
#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct BatchMetrics {
    /// 批次类型
    pub phase: String,
    /// 批次中的请求数
    pub batch_size: usize,
    /// 批次中的总 token 数
    pub total_tokens: usize,
    /// 执行耗时毫秒
    pub duration_ms: f64,
    /// 批次开始时间
    pub start_time_ms: u64,
    /// 批次结束时间
    pub end_time_ms: u64,
}

/// 内存使用指标
#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct MemoryMetrics {
    /// KV Cache 已用块数
    pub kv_cache_used_blocks: usize,
    /// KV Cache 可用块数
    pub kv_cache_free_blocks: usize,
    /// KV Cache 总块数
    pub kv_cache_total_blocks: usize,
    /// KV Cache 使用率
    pub kv_cache_utilization: f64,
}

/// 性能指标快照
#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct MetricsSnapshot {
    /// 时间戳
    pub timestamp_ms: u64,
    /// 所有完成的请求指标
    pub requests: Vec<RequestMetrics>,
    /// 批次历史
    pub batches: Vec<BatchMetrics>,
    /// 内存指标
    pub memory: MemoryMetrics,
    /// 汇总统计
    pub summary: SummaryStats,
}

/// 汇总统计
#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct SummaryStats {
    /// 总请求数
    pub total_requests: usize,
    /// 平均 TTFT 毫秒
    pub avg_ttft_ms: Option<f64>,
    /// 平均 throughput (tokens/s)
    pub avg_throughput_tps: Option<f64>,
    /// 平均 inter-token latency 毫秒
    pub avg_itl_ms: Option<f64>,
    /// 峰值 batch size
    pub peak_batch_size: usize,
    /// 总 prefill tokens
    pub total_prefill_tokens: usize,
    /// 总 decode tokens
    pub total_decode_tokens: usize,
}

impl SummaryStats {
    /// 从请求指标计算汇总统计
    fn from_requests(requests: &[RequestMetrics]) -> Self {
        let total_requests = requests.len();

        let avg_ttft_ms = if total_requests > 0 {
            let sum: f64 = requests.iter().filter_map(|r| r.ttft_ms).sum();
            let count = requests.iter().filter(|r| r.ttft_ms.is_some()).count();
            if count > 0 {
                Some(sum / count as f64)
            } else {
                None
            }
        } else {
            None
        };

        let avg_throughput_tps = if total_requests > 0 {
            let sum: f64 = requests.iter().filter_map(|r| r.throughput_tps).sum();
            let count = requests.iter().filter(|r| r.throughput_tps.is_some()).count();
            if count > 0 {
                Some(sum / count as f64)
            } else {
                None
            }
        } else {
            None
        };

        let avg_itl_ms = if total_requests > 0 {
            let sum: f64 = requests.iter().filter_map(|r| r.inter_token_latency_ms).sum();
            let count = requests
                .iter()
                .filter(|r| r.inter_token_latency_ms.is_some())
                .count();
            if count > 0 {
                Some(sum / count as f64)
            } else {
                None
            }
        } else {
            None
        };

        let total_prefill_tokens: usize = requests.iter().map(|r| r.prompt_len).sum();
        let total_decode_tokens: usize = requests.iter().map(|r| r.generated_tokens).sum();

        Self {
            total_requests,
            avg_ttft_ms,
            avg_throughput_tps,
            avg_itl_ms,
            peak_batch_size: 0, // 在 MetricsCollector 中更新
            total_prefill_tokens,
            total_decode_tokens,
        }
    }
}

/// 指标收集器内部状态
struct MetricsState {
    active_requests: HashMap<RequestId, RequestMetrics>,
    completed_requests: Vec<RequestMetrics>,
    batch_history: Vec<BatchMetrics>,
    current_prefill: Option<(Vec<RequestId>, usize)>,
    current_decode: Option<Vec<RequestId>>,
    memory: MemoryMetrics,
    peak_batch_size: usize,
}

/// 性能指标收集器
pub struct MetricsCollector {
    state: Mutex<MetricsState>,
    start_time: Instant,
}

impl MetricsCollector {
    /// 创建新的 MetricsCollector
    pub fn new() -> Self {
        Self {
            state: Mutex::new(MetricsState {
                active_requests: HashMap::new(),
                completed_requests: Vec::new(),
                batch_history: Vec::new(),
                current_prefill: None,
                current_decode: None,
                memory: MemoryMetrics::default(),
                peak_batch_size: 0,
            }),
            start_time: Instant::now(),
        }
    }

    fn now_ms(&self) -> u64 {
        self.start_time.elapsed().as_millis() as u64
    }

    /// 记录请求提交
    pub fn on_request_submit(&self, request_id: RequestId, prompt_len: usize) {
        if let Ok(mut state) = self.state.lock() {
            let metrics = RequestMetrics {
                request_id,
                prompt_len,
                start_time_ms: self.now_ms(),
                ..Default::default()
            };
            state.active_requests.insert(request_id, metrics);
        }
    }

    /// 记录 prefill 开始
    pub fn on_prefill_start(&self, request_ids: &[RequestId], total_tokens: usize) {
        if let Ok(mut state) = self.state.lock() {
            state.current_prefill = Some((request_ids.to_vec(), total_tokens));
            state.peak_batch_size = state.peak_batch_size.max(request_ids.len());
        }
    }

    /// 记录 prefill 结束
    pub fn on_prefill_end(&self, request_ids: &[RequestId], duration_ms: f64) {
        let now = self.now_ms();
        if let Ok(mut state) = self.state.lock() {
            // 记录批次指标
            if let Some((ids, tokens)) = state.current_prefill.take() {
                state.batch_history.push(BatchMetrics {
                    phase: "prefill".to_string(),
                    batch_size: ids.len(),
                    total_tokens: tokens,
                    duration_ms,
                    start_time_ms: now.saturating_sub(duration_ms as u64),
                    end_time_ms: now,
                });
            }

            // 更新请求指标
            for request_id in request_ids {
                if let Some(metrics) = state.active_requests.get_mut(request_id) {
                    metrics.prefill_time_ms = duration_ms;
                    metrics.first_token_time_ms = Some(now);
                    metrics.ttft_ms = Some(now as f64 - metrics.start_time_ms as f64);
                }
            }
        }
    }

    /// 记录 decode 开始
    pub fn on_decode_start(&self, request_ids: &[RequestId]) {
        if let Ok(mut state) = self.state.lock() {
            state.current_decode = Some(request_ids.to_vec());
            state.peak_batch_size = state.peak_batch_size.max(request_ids.len());
        }
    }

    /// 记录 decode 结束
    pub fn on_decode_end(&self, request_ids: &[RequestId], duration_ms: f64) {
        let now = self.now_ms();
        if let Ok(mut state) = self.state.lock() {
            // 记录批次指标
            if let Some(ids) = state.current_decode.take() {
                state.batch_history.push(BatchMetrics {
                    phase: "decode".to_string(),
                    batch_size: ids.len(),
                    total_tokens: ids.len(), // 每个请求一个 token
                    duration_ms,
                    start_time_ms: now.saturating_sub(duration_ms as u64),
                    end_time_ms: now,
                });
            }

            // 更新请求指标
            for request_id in request_ids {
                if let Some(metrics) = state.active_requests.get_mut(request_id) {
                    metrics.decode_time_ms += duration_ms;
                    metrics.generated_tokens += 1;
                }
            }
        }
    }

    /// 记录 token 生成
    pub fn on_token_generated(&self, request_id: RequestId, _token_id: u32, phase: SequencePhase) {
        // token 生成已在 decode_end 中统计，这里可用于更细粒度的追踪
        let _ = (request_id, phase);
    }

    /// 记录请求完成
    pub fn on_request_finish(&self, request_id: RequestId, _reason: &str) {
        let now = self.now_ms();
        if let Ok(mut state) = self.state.lock() {
            if let Some(mut metrics) = state.active_requests.remove(&request_id) {
                metrics.end_time_ms = Some(now);
                metrics.total_latency_ms = Some(now as f64 - metrics.start_time_ms as f64);
                metrics.calculate_throughput();
                metrics.calculate_itl();
                state.completed_requests.push(metrics);
            }
        }
    }

    /// 更新 KV Cache 指标
    pub fn update_kv_cache(&self, used: usize, free: usize, total: usize) {
        if let Ok(mut state) = self.state.lock() {
            state.memory = MemoryMetrics {
                kv_cache_used_blocks: used,
                kv_cache_free_blocks: free,
                kv_cache_total_blocks: total,
                kv_cache_utilization: if total > 0 {
                    used as f64 / total as f64
                } else {
                    0.0
                },
            };
        }
    }

    /// 获取当前快照
    pub fn snapshot(&self) -> MetricsSnapshot {
        let now = self.now_ms();
        if let Ok(state) = self.state.lock() {
            let mut summary = SummaryStats::from_requests(&state.completed_requests);
            summary.peak_batch_size = state.peak_batch_size;

            MetricsSnapshot {
                timestamp_ms: now,
                requests: state.completed_requests.clone(),
                batches: state.batch_history.clone(),
                memory: state.memory.clone(),
                summary,
            }
        } else {
            MetricsSnapshot {
                timestamp_ms: now,
                requests: Vec::new(),
                batches: Vec::new(),
                memory: MemoryMetrics::default(),
                summary: SummaryStats::default(),
            }
        }
    }

    /// 格式化输出摘要
    pub fn format_summary(&self) -> String {
        let snapshot = self.snapshot();
        let summary = &snapshot.summary;

        let mut output = String::new();
        output.push_str("=== Performance Summary ===\n");
        output.push_str(&format!("Total requests: {}\n", summary.total_requests));
        output.push_str(&format!("Peak batch size: {}\n", summary.peak_batch_size));
        output.push_str(&format!(
            "Total tokens (prefill/decode): {}/{}\n",
            summary.total_prefill_tokens, summary.total_decode_tokens
        ));

        if let Some(ttft) = summary.avg_ttft_ms {
            output.push_str(&format!("Avg TTFT: {:.2} ms\n", ttft));
        }
        if let Some(tps) = summary.avg_throughput_tps {
            output.push_str(&format!("Avg throughput: {:.2} tokens/s\n", tps));
        }
        if let Some(itl) = summary.avg_itl_ms {
            output.push_str(&format!("Avg inter-token latency: {:.2} ms\n", itl));
        }

        output.push_str(&format!(
            "KV Cache utilization: {:.1}%\n",
            snapshot.memory.kv_cache_utilization * 100.0
        ));

        output
    }
}

impl Default for MetricsCollector {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_metrics_collector() {
        let collector = MetricsCollector::new();

        // 模拟一个完整请求
        collector.on_request_submit(1, 10);
        collector.update_kv_cache(4, 60, 64);

        collector.on_prefill_start(&[1], 10);
        collector.on_prefill_end(&[1], 5.0);

        collector.on_decode_start(&[1]);
        collector.on_decode_end(&[1], 2.0);

        collector.on_request_finish(1, "max_tokens");

        let snapshot = collector.snapshot();
        assert_eq!(snapshot.requests.len(), 1);
        assert_eq!(snapshot.summary.total_requests, 1);
        assert_eq!(snapshot.batches.len(), 2); // prefill + decode
    }

    #[test]
    fn test_summary_stats() {
        let requests = vec![
            RequestMetrics {
                request_id: 1,
                prompt_len: 10,
                generated_tokens: 8,
                ttft_ms: Some(10.0),
                throughput_tps: Some(100.0),
                inter_token_latency_ms: Some(5.0),
                ..Default::default()
            },
            RequestMetrics {
                request_id: 2,
                prompt_len: 20,
                generated_tokens: 16,
                ttft_ms: Some(20.0),
                throughput_tps: Some(80.0),
                inter_token_latency_ms: Some(10.0),
                ..Default::default()
            },
        ];

        let summary = SummaryStats::from_requests(&requests);
        assert_eq!(summary.total_requests, 2);
        assert_eq!(summary.avg_ttft_ms, Some(15.0));
        assert_eq!(summary.avg_throughput_tps, Some(90.0));
        assert_eq!(summary.avg_itl_ms, Some(7.5));
        assert_eq!(summary.total_prefill_tokens, 30);
        assert_eq!(summary.total_decode_tokens, 24);
    }

    #[test]
    fn test_format_summary() {
        let collector = MetricsCollector::new();
        collector.on_request_submit(1, 10);
        collector.on_prefill_start(&[1], 10);
        collector.on_prefill_end(&[1], 5.0);
        collector.on_request_finish(1, "done");

        let summary = collector.format_summary();
        assert!(summary.contains("Performance Summary"));
        assert!(summary.contains("Total requests: 1"));
    }
}
