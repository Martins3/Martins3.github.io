//! Trace 收集模块 - 记录引擎执行轨迹

use std::sync::Mutex;
use std::time::Instant;

use serde::{Deserialize, Serialize};
use teach_core::{EngineError, RequestId, SequencePhase};

/// Trace 级别
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TraceLevel {
    /// 关闭
    Off,
    /// 仅错误
    Error,
    /// 警告及以上
    Warn,
    /// 一般信息
    Info,
    /// 详细信息
    Debug,
    /// 所有细节
    Trace,
}

/// Trace 事件类型
#[derive(Clone, Debug, Serialize, Deserialize, PartialEq)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum TraceEvent {
    /// 请求提交
    RequestSubmit {
        timestamp_ms: u64,
        request_id: RequestId,
        prompt_len: usize,
    },
    /// 调度决策
    Schedule {
        timestamp_ms: u64,
        phase: String,
        request_ids: Vec<RequestId>,
    },
    /// Prefill 开始
    PrefillStart {
        timestamp_ms: u64,
        request_ids: Vec<RequestId>,
        total_tokens: usize,
    },
    /// Prefill 结束
    PrefillEnd {
        timestamp_ms: u64,
        request_ids: Vec<RequestId>,
        duration_ms: f64,
    },
    /// Decode 开始
    DecodeStart {
        timestamp_ms: u64,
        request_ids: Vec<RequestId>,
    },
    /// Decode 结束
    DecodeEnd {
        timestamp_ms: u64,
        request_ids: Vec<RequestId>,
        duration_ms: f64,
    },
    /// Token 生成
    TokenGenerated {
        timestamp_ms: u64,
        request_id: RequestId,
        token_id: u32,
        phase: String,
    },
    /// 请求完成
    RequestFinish {
        timestamp_ms: u64,
        request_id: RequestId,
        reason: String,
    },
    /// KV Cache 分配
    KvCacheAllocate {
        timestamp_ms: u64,
        request_id: RequestId,
        blocks: usize,
    },
    /// KV Cache 释放
    KvCacheRelease {
        timestamp_ms: u64,
        request_id: RequestId,
        blocks: usize,
    },
}

impl TraceEvent {
    /// 获取事件时间戳
    pub fn timestamp_ms(&self) -> u64 {
        match self {
            Self::RequestSubmit { timestamp_ms, .. } => *timestamp_ms,
            Self::Schedule { timestamp_ms, .. } => *timestamp_ms,
            Self::PrefillStart { timestamp_ms, .. } => *timestamp_ms,
            Self::PrefillEnd { timestamp_ms, .. } => *timestamp_ms,
            Self::DecodeStart { timestamp_ms, .. } => *timestamp_ms,
            Self::DecodeEnd { timestamp_ms, .. } => *timestamp_ms,
            Self::TokenGenerated { timestamp_ms, .. } => *timestamp_ms,
            Self::RequestFinish { timestamp_ms, .. } => *timestamp_ms,
            Self::KvCacheAllocate { timestamp_ms, .. } => *timestamp_ms,
            Self::KvCacheRelease { timestamp_ms, .. } => *timestamp_ms,
        }
    }

    /// 获取事件名称
    pub fn name(&self) -> &'static str {
        match self {
            Self::RequestSubmit { .. } => "request_submit",
            Self::Schedule { .. } => "schedule",
            Self::PrefillStart { .. } => "prefill_start",
            Self::PrefillEnd { .. } => "prefill_end",
            Self::DecodeStart { .. } => "decode_start",
            Self::DecodeEnd { .. } => "decode_end",
            Self::TokenGenerated { .. } => "token_generated",
            Self::RequestFinish { .. } => "request_finish",
            Self::KvCacheAllocate { .. } => "kv_cache_allocate",
            Self::KvCacheRelease { .. } => "kv_cache_release",
        }
    }
}

/// Trace 收集器
pub struct TraceCollector {
    level: TraceLevel,
    events: Mutex<Vec<TraceEvent>>,
    start_time: Instant,
}

impl TraceCollector {
    /// 创建新的 TraceCollector
    pub fn new(level: TraceLevel) -> Self {
        Self {
            level,
            events: Mutex::new(Vec::new()),
            start_time: Instant::now(),
        }
    }

    /// 获取当前时间戳（相对于开始时间）
    fn now_ms(&self) -> u64 {
        self.start_time.elapsed().as_millis() as u64
    }

    /// 检查级别是否允许记录
    fn should_record(&self, min_level: TraceLevel) -> bool {
        let level_val = |l: TraceLevel| match l {
            TraceLevel::Off => 0,
            TraceLevel::Error => 1,
            TraceLevel::Warn => 2,
            TraceLevel::Info => 3,
            TraceLevel::Debug => 4,
            TraceLevel::Trace => 5,
        };
        level_val(self.level) >= level_val(min_level)
    }

    /// 添加事件
    fn add_event(&self, event: TraceEvent) {
        if let Ok(mut events) = self.events.lock() {
            events.push(event);
        }
    }

    /// 记录请求提交
    pub fn on_request_submit(&self, request_id: RequestId, prompt_len: usize) {
        if !self.should_record(TraceLevel::Info) {
            return;
        }
        self.add_event(TraceEvent::RequestSubmit {
            timestamp_ms: self.now_ms(),
            request_id,
            prompt_len,
        });
    }

    /// 记录调度决策
    pub fn on_schedule(&self, phase: SequencePhase, request_ids: &[RequestId]) {
        if !self.should_record(TraceLevel::Debug) {
            return;
        }
        self.add_event(TraceEvent::Schedule {
            timestamp_ms: self.now_ms(),
            phase: format!("{:?}", phase),
            request_ids: request_ids.to_vec(),
        });
    }

    /// 记录 prefill 开始
    pub fn on_prefill_start(&self, request_ids: &[RequestId], total_tokens: usize) {
        if !self.should_record(TraceLevel::Info) {
            return;
        }
        self.add_event(TraceEvent::PrefillStart {
            timestamp_ms: self.now_ms(),
            request_ids: request_ids.to_vec(),
            total_tokens,
        });
    }

    /// 记录 prefill 结束
    pub fn on_prefill_end(&self, request_ids: &[RequestId], duration_ms: f64) {
        if !self.should_record(TraceLevel::Info) {
            return;
        }
        self.add_event(TraceEvent::PrefillEnd {
            timestamp_ms: self.now_ms(),
            request_ids: request_ids.to_vec(),
            duration_ms,
        });
    }

    /// 记录 decode 开始
    pub fn on_decode_start(&self, request_ids: &[RequestId]) {
        if !self.should_record(TraceLevel::Debug) {
            return;
        }
        self.add_event(TraceEvent::DecodeStart {
            timestamp_ms: self.now_ms(),
            request_ids: request_ids.to_vec(),
        });
    }

    /// 记录 decode 结束
    pub fn on_decode_end(&self, request_ids: &[RequestId], duration_ms: f64) {
        if !self.should_record(TraceLevel::Debug) {
            return;
        }
        self.add_event(TraceEvent::DecodeEnd {
            timestamp_ms: self.now_ms(),
            request_ids: request_ids.to_vec(),
            duration_ms,
        });
    }

    /// 记录 token 生成
    pub fn on_token_generated(&self, request_id: RequestId, token_id: u32, phase: SequencePhase) {
        if !self.should_record(TraceLevel::Trace) {
            return;
        }
        self.add_event(TraceEvent::TokenGenerated {
            timestamp_ms: self.now_ms(),
            request_id,
            token_id,
            phase: format!("{:?}", phase),
        });
    }

    /// 记录请求完成
    pub fn on_request_finish(&self, request_id: RequestId, reason: &str) {
        if !self.should_record(TraceLevel::Info) {
            return;
        }
        self.add_event(TraceEvent::RequestFinish {
            timestamp_ms: self.now_ms(),
            request_id,
            reason: reason.to_string(),
        });
    }

    /// 记录 KV Cache 分配
    pub fn on_kv_cache_allocate(&self, request_id: RequestId, blocks: usize) {
        if !self.should_record(TraceLevel::Debug) {
            return;
        }
        self.add_event(TraceEvent::KvCacheAllocate {
            timestamp_ms: self.now_ms(),
            request_id,
            blocks,
        });
    }

    /// 记录 KV Cache 释放
    pub fn on_kv_cache_release(&self, request_id: RequestId, blocks: usize) {
        if !self.should_record(TraceLevel::Debug) {
            return;
        }
        self.add_event(TraceEvent::KvCacheRelease {
            timestamp_ms: self.now_ms(),
            request_id,
            blocks,
        });
    }

    /// 获取所有事件
    pub fn events(&self) -> Vec<TraceEvent> {
        self.events.lock().map(|e| e.clone()).unwrap_or_default()
    }

    /// 导出为 JSON
    pub fn export_json(&self) -> Result<String, EngineError> {
        let events = self.events();
        serde_json::to_string_pretty(&events)
            .map_err(|e| EngineError::new(format!("failed to serialize trace: {e}")))
    }

    /// 获取事件数量
    pub fn event_count(&self) -> usize {
        self.events.lock().map(|e| e.len()).unwrap_or(0)
    }

    /// 清空事件
    pub fn clear(&self) {
        if let Ok(mut events) = self.events.lock() {
            events.clear();
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_trace_collector() {
        let collector = TraceCollector::new(TraceLevel::Debug);

        collector.on_request_submit(1, 10);
        collector.on_kv_cache_allocate(1, 4);
        collector.on_prefill_start(&[1], 10);
        collector.on_prefill_end(&[1], 5.0);
        collector.on_request_finish(1, "max_tokens");

        let events = collector.events();
        assert_eq!(events.len(), 5);

        let json = collector.export_json().unwrap();
        assert!(json.contains("request_submit"));
        assert!(json.contains("prefill_start"));
    }

    #[test]
    fn test_trace_level_filtering() {
        let collector = TraceCollector::new(TraceLevel::Info);

        // Info 级别应该记录这些事件
        assert!(collector.should_record(TraceLevel::Error));
        assert!(collector.should_record(TraceLevel::Info));
        // Info 级别不应该记录 Trace 级别的事件
        assert!(!collector.should_record(TraceLevel::Trace));
    }

    #[test]
    fn test_off_level() {
        let collector = TraceCollector::new(TraceLevel::Off);
        collector.on_request_submit(1, 10);
        assert_eq!(collector.event_count(), 0);
    }
}
