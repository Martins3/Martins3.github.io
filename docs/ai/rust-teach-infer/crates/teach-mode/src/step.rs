//! 逐步执行控制模块

use std::sync::Mutex;

use teach_core::{RequestId, SequencePhase};

/// 逐步执行决策
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum StepDecision {
    /// 继续执行
    Continue,
    /// 暂停执行
    Pause,
    /// 单步执行（暂停一次后继续）
    Step,
}

/// 断点条件
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum BreakCondition {
    /// 在指定请求 ID 处暂停
    RequestId(RequestId),
    /// 在指定阶段暂停
    Phase(SequencePhase),
    /// 在指定请求和阶段组合处暂停
    RequestAndPhase(RequestId, SequencePhase),
    /// 在每个 token 生成后暂停
    EveryToken,
    /// 在每个批次处理后暂停
    EveryBatch,
}

/// 逐步执行控制器
pub struct StepController {
    state: Mutex<StepState>,
}

#[derive(Clone, Debug)]
struct StepState {
    /// 是否启用逐步执行
    enabled: bool,
    /// 已设置的断点
    breakpoints: Vec<BreakCondition>,
    /// 是否暂停中
    paused: bool,
    /// 单步模式（执行一次后恢复暂停）
    step_mode: bool,
    /// 当前请求上下文（用于匹配断点）
    current_request: Option<RequestId>,
    /// 当前阶段上下文
    current_phase: Option<SequencePhase>,
}

impl StepController {
    /// 创建新的 StepController
    pub fn new() -> Self {
        Self {
            state: Mutex::new(StepState {
                enabled: false,
                breakpoints: Vec::new(),
                paused: false,
                step_mode: false,
                current_request: None,
                current_phase: None,
            }),
        }
    }

    /// 启用逐步执行
    pub fn enable(&self) {
        if let Ok(mut state) = self.state.lock() {
            state.enabled = true;
            state.paused = true; // 默认从暂停开始
        }
    }

    /// 禁用逐步执行
    pub fn disable(&self) {
        if let Ok(mut state) = self.state.lock() {
            state.enabled = false;
            state.paused = false;
            state.step_mode = false;
        }
    }

    /// 添加断点
    pub fn add_breakpoint(&self, condition: BreakCondition) {
        if let Ok(mut state) = self.state.lock() {
            state.breakpoints.push(condition);
        }
    }

    /// 移除所有断点
    pub fn clear_breakpoints(&self) {
        if let Ok(mut state) = self.state.lock() {
            state.breakpoints.clear();
        }
    }

    /// 继续执行（取消暂停状态）
    pub fn resume(&self) {
        if let Ok(mut state) = self.state.lock() {
            state.paused = false;
            state.step_mode = false;
        }
    }

    /// 单步执行（执行一次后暂停）
    pub fn step(&self) {
        if let Ok(mut state) = self.state.lock() {
            state.paused = false;
            state.step_mode = true;
        }
    }

    /// 暂停执行
    pub fn pause(&self) {
        if let Ok(mut state) = self.state.lock() {
            state.paused = true;
        }
    }

    /// 检查是否暂停
    pub fn is_paused(&self) -> bool {
        self.state.lock().map(|s| s.paused).unwrap_or(false)
    }

    /// 获取当前状态描述
    pub fn status(&self) -> String {
        self.state
            .lock()
            .map(|s| {
                if !s.enabled {
                    "disabled".to_string()
                } else if s.paused {
                    format!(
                        "paused (breakpoints: {})",
                        s.breakpoints.len()
                    )
                } else if s.step_mode {
                    "stepping".to_string()
                } else {
                    "running".to_string()
                }
            })
            .unwrap_or_else(|_| "error".to_string())
    }

    /// 检查是否应该暂停
    pub fn should_pause(&self, request_id: RequestId, phase: SequencePhase) -> StepDecision {
        let state = match self.state.lock() {
            Ok(s) => s,
            Err(_) => return StepDecision::Continue,
        };

        if !state.enabled {
            return StepDecision::Continue;
        }

        // 如果已经暂停，保持暂停状态
        if state.paused {
            return StepDecision::Pause;
        }

        // 检查是否匹配任何断点
        let should_break = state.breakpoints.iter().any(|bp| match bp {
            BreakCondition::RequestId(id) => *id == request_id,
            BreakCondition::Phase(p) => *p == phase,
            BreakCondition::RequestAndPhase(id, p) => *id == request_id && *p == phase,
            BreakCondition::EveryToken => {
                matches!(phase, SequencePhase::Prefill | SequencePhase::Decoding)
            }
            BreakCondition::EveryBatch => {
                matches!(phase, SequencePhase::Prefill | SequencePhase::Decoding)
            }
        });

        if should_break {
            // 如果是单步模式，这次执行后需要重新暂停
            return if state.step_mode {
                StepDecision::Step
            } else {
                StepDecision::Pause
            };
        }

        StepDecision::Continue
    }

    /// 通知执行完成，更新状态
    pub fn on_step_executed(&self) {
        if let Ok(mut state) = self.state.lock() {
            if state.step_mode {
                // 单步执行后恢复暂停
                state.paused = true;
                state.step_mode = false;
            }
        }
    }

    /// 设置当前上下文
    pub fn set_context(&self, request_id: Option<RequestId>, phase: Option<SequencePhase>) {
        if let Ok(mut state) = self.state.lock() {
            state.current_request = request_id;
            state.current_phase = phase;
        }
    }
}

impl Default for StepController {
    fn default() -> Self {
        Self::new()
    }
}

/// 创建交互式调试会话
pub struct DebugSession {
    controller: StepController,
}

impl DebugSession {
    /// 创建新的调试会话
    pub fn new() -> Self {
        let controller = StepController::new();
        controller.enable();
        Self { controller }
    }

    /// 在指定请求处设置断点
    pub fn breakpoint_on_request(&self, request_id: RequestId) {
        self.controller
            .add_breakpoint(BreakCondition::RequestId(request_id));
    }

    /// 在指定阶段设置断点
    pub fn breakpoint_on_phase(&self, phase: SequencePhase) {
        self.controller
            .add_breakpoint(BreakCondition::Phase(phase));
    }

    /// 在每个 token 处暂停
    pub fn break_every_token(&self) {
        self.controller.add_breakpoint(BreakCondition::EveryToken);
    }

    /// 在每个批次处暂停
    pub fn break_every_batch(&self) {
        self.controller.add_breakpoint(BreakCondition::EveryBatch);
    }

    /// 继续执行
    pub fn resume(&self) {
        self.controller.resume();
    }

    /// 单步执行
    pub fn step(&self) {
        self.controller.step();
    }

    /// 获取控制器
    pub fn controller(&self) -> &StepController {
        &self.controller
    }
}

impl Default for DebugSession {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_step_controller_basic() {
        let controller = StepController::new();

        // 未启用时应该继续
        assert_eq!(
            controller.should_pause(1, SequencePhase::Prefill),
            StepDecision::Continue
        );

        // 启用后默认暂停
        controller.enable();
        assert!(controller.is_paused());
        assert_eq!(
            controller.should_pause(1, SequencePhase::Prefill),
            StepDecision::Pause
        );

        // 继续执行
        controller.resume();
        assert!(!controller.is_paused());
    }

    #[test]
    fn test_breakpoint_request_id() {
        let controller = StepController::new();
        controller.enable();
        controller.add_breakpoint(BreakCondition::RequestId(5));
        controller.resume();

        // 非断点请求应该继续
        assert_eq!(
            controller.should_pause(1, SequencePhase::Prefill),
            StepDecision::Continue
        );

        // 断点请求应该暂停
        assert_eq!(
            controller.should_pause(5, SequencePhase::Prefill),
            StepDecision::Pause
        );
    }

    #[test]
    fn test_breakpoint_phase() {
        let controller = StepController::new();
        controller.enable();
        controller.add_breakpoint(BreakCondition::Phase(SequencePhase::Decoding));
        controller.resume();

        // Prefill 应该继续
        assert_eq!(
            controller.should_pause(1, SequencePhase::Prefill),
            StepDecision::Continue
        );

        // Decoding 应该暂停
        assert_eq!(
            controller.should_pause(1, SequencePhase::Decoding),
            StepDecision::Pause
        );
    }

    #[test]
    fn test_step_mode() {
        let controller = StepController::new();
        controller.enable();
        controller.resume();

        // 设置断点
        controller.add_breakpoint(BreakCondition::EveryToken);

        // 单步执行
        controller.step();
        assert_eq!(
            controller.should_pause(1, SequencePhase::Prefill),
            StepDecision::Step
        );

        // 执行后应该暂停
        controller.on_step_executed();
        assert!(controller.is_paused());
    }

    #[test]
    fn test_debug_session() {
        let session = DebugSession::new();
        assert!(session.controller().is_paused());

        session.breakpoint_on_request(10);
        session.break_every_token();
        session.resume();
    }
}
