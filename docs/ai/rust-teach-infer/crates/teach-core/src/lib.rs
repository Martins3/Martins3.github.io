use std::fmt;
use std::path::PathBuf;

pub type RequestId = u64;
pub type TokenId = u32;

#[derive(Clone, Debug, PartialEq)]
pub struct SamplingParams {
    pub max_new_tokens: usize,
    pub temperature: f32,
    pub top_k: usize,
    pub seed: u64,
}

impl Default for SamplingParams {
    fn default() -> Self {
        Self {
            max_new_tokens: 16,
            temperature: 0.0,
            top_k: 1,
            seed: 0,
        }
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct StopCondition {
    pub stop_token: Option<TokenId>,
}

impl Default for StopCondition {
    fn default() -> Self {
        Self {
            stop_token: Some(0),
        }
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct GenerateRequest {
    pub id: RequestId,
    pub prompt: String,
    pub prompt_tokens: Vec<TokenId>,
    pub sampling: SamplingParams,
    pub stop: StopCondition,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SequencePhase {
    Waiting,
    Prefill,
    Decoding,
    Finished,
    Errored,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum TokenEvent {
    Started {
        request_id: RequestId,
        prompt_len: usize,
    },
    Token {
        request_id: RequestId,
        token_id: TokenId,
        text: String,
        phase: SequencePhase,
    },
    Finished {
        request_id: RequestId,
        output_tokens: Vec<TokenId>,
        text: String,
        reason: FinishReason,
    },
    Error {
        request_id: RequestId,
        message: String,
    },
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum FinishReason {
    MaxTokens,
    StopToken,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct GenerateOutput {
    pub request_id: RequestId,
    pub tokens: Vec<TokenId>,
    pub text: String,
    pub reason: FinishReason,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct SequenceState {
    pub request_id: RequestId,
    pub prompt_tokens: Vec<TokenId>,
    pub generated_tokens: Vec<TokenId>,
    pub phase: SequencePhase,
}

#[derive(Clone, Debug, PartialEq)]
pub struct EngineConfig {
    pub max_batch_size: usize,
    pub block_size: usize,
    pub max_blocks: usize,
}

impl Default for EngineConfig {
    fn default() -> Self {
        Self {
            max_batch_size: 4,
            block_size: 16,
            max_blocks: 64,
        }
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ModelConfig {
    pub model_family: String,
    pub model_id: String,
    pub model_dir: PathBuf,
    pub context_len: usize,
}

impl Default for ModelConfig {
    fn default() -> Self {
        Self {
            model_family: "qwen2".to_string(),
            model_id: "Qwen/Qwen2.5-0.5B".to_string(),
            model_dir: PathBuf::from("./models/Qwen2.5-0.5B"),
            context_len: 512,
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum DeviceKind {
    Cpu,
    Cuda,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum WeightFormat {
    Fp32,
    Fp16,
    Bf16,
    Int8,
}

/// 模型实现类型
#[derive(Clone, Copy, Debug, PartialEq, Eq, Default)]
pub enum ModelImplementation {
    /// 参考实现（stub）
    #[default]
    Reference,
    /// 真实权重实现
    Real,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct CudaConfig {
    pub preferred_arch: String,
    pub min_memory_mib: usize,
}

impl Default for CudaConfig {
    fn default() -> Self {
        Self {
            preferred_arch: "sm_120".to_string(),
            min_memory_mib: 3072,
        }
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct EngineBuilderConfig {
    pub engine: EngineConfig,
    pub model: ModelConfig,
    pub cuda: CudaConfig,
    pub device: DeviceKind,
    pub weight_format: WeightFormat,
    pub model_impl: ModelImplementation,
}

impl Default for EngineBuilderConfig {
    fn default() -> Self {
        Self {
            engine: EngineConfig::default(),
            model: ModelConfig::default(),
            cuda: CudaConfig::default(),
            device: DeviceKind::Cpu,
            weight_format: WeightFormat::Fp32,
            model_impl: ModelImplementation::default(),
        }
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct EngineError {
    message: String,
}

impl EngineError {
    pub fn new(message: impl Into<String>) -> Self {
        Self {
            message: message.into(),
        }
    }

    pub fn message(&self) -> &str {
        &self.message
    }
}

impl fmt::Display for EngineError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.message)
    }
}

impl std::error::Error for EngineError {}
