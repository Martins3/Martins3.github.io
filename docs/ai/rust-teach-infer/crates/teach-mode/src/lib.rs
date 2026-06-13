//! 教学模式框架 - 用于 LLM 推理系统的可视化、trace 和性能分析
//!
//! 提供以下功能：
//! - TraceCollector: 收集引擎执行轨迹
//! - Visualizer: ASCII 可视化输出
//! - StepController: 逐步执行控制
//! - MetricsCollector: 性能指标收集

pub mod metrics;
pub mod step;
pub mod trace;
pub mod visual;

pub use metrics::{MetricsCollector, MetricsSnapshot, RequestMetrics};
pub use step::{BreakCondition, StepController, StepDecision};
pub use trace::{TraceCollector, TraceEvent, TraceLevel};
pub use visual::{BatchVisualizer, MemoryVisualizer, TimelineVisualizer, VisualConfig};

use std::fmt;
use std::sync::Arc;
use teach_core::{EngineError, RequestId, SequencePhase};

/// 教学模式配置
#[derive(Clone, Debug, PartialEq)]
pub struct TeachModeConfig {
    /// Trace 级别
    pub trace_level: TraceLevel,
    /// 是否启用逐步执行
    pub enable_step: bool,
    /// 是否启用可视化
    pub enable_visual: bool,
    /// 是否启用指标收集
    pub enable_metrics: bool,
    /// 可视化配置
    pub visual_config: VisualConfig,
}

impl Default for TeachModeConfig {
    fn default() -> Self {
        Self {
            trace_level: TraceLevel::Info,
            enable_step: false,
            enable_visual: true,
            enable_metrics: true,
            visual_config: VisualConfig::default(),
        }
    }
}

/// 教学模式 trait - 引擎通过此接口与教学模式交互
pub trait ModeTracer: Send + Sync {
    /// 记录请求提交
    fn on_request_submit(&self, request_id: RequestId, prompt_len: usize);

    /// 记录调度决策
    fn on_schedule(&self, phase: SequencePhase, request_ids: &[RequestId]);

    /// 记录 prefill 开始
    fn on_prefill_start(&self, request_ids: &[RequestId], total_tokens: usize);

    /// 记录 prefill 完成
    fn on_prefill_end(&self, request_ids: &[RequestId], duration_ms: f64);

    /// 记录 decode 开始
    fn on_decode_start(&self, request_ids: &[RequestId]);

    /// 记录 decode 完成
    fn on_decode_end(&self, request_ids: &[RequestId], duration_ms: f64);

    /// 记录 token 生成
    fn on_token_generated(&self, request_id: RequestId, token_id: u32, phase: SequencePhase);

    /// 记录请求完成
    fn on_request_finish(&self, request_id: RequestId, reason: &str);

    /// 记录 KV Cache 分配
    fn on_kv_cache_allocate(&self, request_id: RequestId, blocks: usize);

    /// 记录 KV Cache 释放
    fn on_kv_cache_release(&self, request_id: RequestId, blocks: usize);

    /// 检查是否需要暂停（逐步执行模式）
    fn should_pause(&self, request_id: RequestId, phase: SequencePhase) -> StepDecision;

    /// 获取当前收集的指标
    fn metrics(&self) -> Option<MetricsSnapshot>;

    /// 获取 ASCII 可视化输出
    fn visual_output(&self) -> String;

    /// 导出 trace 数据为 JSON
    fn export_trace_json(&self) -> Result<String, EngineError>;
}

/// 组合式教学模式实现
pub struct CompositeTracer {
    trace: Option<Arc<TraceCollector>>,
    step: Option<Arc<StepController>>,
    visual: Option<Arc<TimelineVisualizer>>,
    metrics: Option<Arc<MetricsCollector>>,
}

impl CompositeTracer {
    pub fn new(config: &TeachModeConfig) -> Self {
        Self {
            trace: (config.trace_level != TraceLevel::Off)
                .then(|| Arc::new(TraceCollector::new(config.trace_level))),
            step: config.enable_step.then(|| Arc::new(StepController::new())),
            visual: config
                .enable_visual
                .then(|| Arc::new(TimelineVisualizer::new(config.visual_config.clone()))),
            metrics: config.enable_metrics.then(|| Arc::new(MetricsCollector::new())),
        }
    }

    /// 创建仅用于 trace 的 tracer
    pub fn trace_only(level: TraceLevel) -> Self {
        Self {
            trace: Some(Arc::new(TraceCollector::new(level))),
            step: None,
            visual: None,
            metrics: None,
        }
    }

    /// 创建用于可视化的 tracer
    pub fn visual_only() -> Self {
        Self {
            trace: None,
            step: None,
            visual: Some(Arc::new(TimelineVisualizer::default())),
            metrics: None,
        }
    }

    /// 创建完整教学模式 tracer
    pub fn full() -> Self {
        Self::new(&TeachModeConfig::default())
    }
}

impl ModeTracer for CompositeTracer {
    fn on_request_submit(&self, request_id: RequestId, prompt_len: usize) {
        if let Some(t) = &self.trace {
            t.on_request_submit(request_id, prompt_len);
        }
        if let Some(v) = &self.visual {
            v.on_request_submit(request_id, prompt_len);
        }
        if let Some(m) = &self.metrics {
            m.on_request_submit(request_id, prompt_len);
        }
    }

    fn on_schedule(&self, phase: SequencePhase, request_ids: &[RequestId]) {
        if let Some(t) = &self.trace {
            t.on_schedule(phase, request_ids);
        }
        if let Some(v) = &self.visual {
            v.on_schedule(phase, request_ids);
        }
    }

    fn on_prefill_start(&self, request_ids: &[RequestId], total_tokens: usize) {
        if let Some(t) = &self.trace {
            t.on_prefill_start(request_ids, total_tokens);
        }
        if let Some(m) = &self.metrics {
            m.on_prefill_start(request_ids, total_tokens);
        }
    }

    fn on_prefill_end(&self, request_ids: &[RequestId], duration_ms: f64) {
        if let Some(t) = &self.trace {
            t.on_prefill_end(request_ids, duration_ms);
        }
        if let Some(m) = &self.metrics {
            m.on_prefill_end(request_ids, duration_ms);
        }
    }

    fn on_decode_start(&self, request_ids: &[RequestId]) {
        if let Some(t) = &self.trace {
            t.on_decode_start(request_ids);
        }
        if let Some(m) = &self.metrics {
            m.on_decode_start(request_ids);
        }
    }

    fn on_decode_end(&self, request_ids: &[RequestId], duration_ms: f64) {
        if let Some(t) = &self.trace {
            t.on_decode_end(request_ids, duration_ms);
        }
        if let Some(m) = &self.metrics {
            m.on_decode_end(request_ids, duration_ms);
        }
    }

    fn on_token_generated(&self, request_id: RequestId, token_id: u32, phase: SequencePhase) {
        if let Some(t) = &self.trace {
            t.on_token_generated(request_id, token_id, phase);
        }
        if let Some(v) = &self.visual {
            v.on_token_generated(request_id, token_id, phase);
        }
        if let Some(m) = &self.metrics {
            m.on_token_generated(request_id, token_id, phase);
        }
    }

    fn on_request_finish(&self, request_id: RequestId, reason: &str) {
        if let Some(t) = &self.trace {
            t.on_request_finish(request_id, reason);
        }
        if let Some(v) = &self.visual {
            v.on_request_finish(request_id, reason);
        }
        if let Some(m) = &self.metrics {
            m.on_request_finish(request_id, reason);
        }
    }

    fn on_kv_cache_allocate(&self, request_id: RequestId, blocks: usize) {
        if let Some(t) = &self.trace {
            t.on_kv_cache_allocate(request_id, blocks);
        }
        if let Some(v) = &self.visual {
            v.on_kv_cache_allocate(request_id, blocks);
        }
    }

    fn on_kv_cache_release(&self, request_id: RequestId, blocks: usize) {
        if let Some(t) = &self.trace {
            t.on_kv_cache_release(request_id, blocks);
        }
        if let Some(v) = &self.visual {
            v.on_kv_cache_release(request_id, blocks);
        }
    }

    fn should_pause(&self, request_id: RequestId, phase: SequencePhase) -> StepDecision {
        self.step
            .as_ref()
            .map(|s| s.should_pause(request_id, phase))
            .unwrap_or(StepDecision::Continue)
    }

    fn metrics(&self) -> Option<MetricsSnapshot> {
        self.metrics.as_ref().map(|m| m.snapshot())
    }

    fn visual_output(&self) -> String {
        self.visual
            .as_ref()
            .map(|v| v.render())
            .unwrap_or_default()
    }

    fn export_trace_json(&self) -> Result<String, EngineError> {
        self.trace
            .as_ref()
            .map(|t| t.export_json())
            .unwrap_or(Ok("[]".to_string()))
    }
}

impl fmt::Debug for CompositeTracer {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("CompositeTracer")
            .field("trace", &self.trace.is_some())
            .field("step", &self.step.is_some())
            .field("visual", &self.visual.is_some())
            .field("metrics", &self.metrics.is_some())
            .finish()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_composite_tracer_creation() {
        let tracer = CompositeTracer::full();
        assert!(tracer.trace.is_some());
        assert!(tracer.visual.is_some());
        assert!(tracer.metrics.is_some());
    }

    #[test]
    fn test_trace_only() {
        let tracer = CompositeTracer::trace_only(TraceLevel::Debug);
        assert!(tracer.trace.is_some());
        assert!(tracer.visual.is_none());
    }

    #[test]
    fn test_default_config() {
        let config = TeachModeConfig::default();
        assert_eq!(config.trace_level, TraceLevel::Info);
        assert!(config.enable_visual);
        assert!(config.enable_metrics);
    }
}
