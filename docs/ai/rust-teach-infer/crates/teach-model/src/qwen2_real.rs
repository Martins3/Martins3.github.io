//! 真实 Qwen2 模型实现
//!
//! 使用 candle-transformers 的 qwen2 模块加载真实权重进行推理

use std::sync::Mutex;

use candle_core::{Device, DType, Tensor};
use candle_transformers::models::qwen2::{Config as Qwen2Config, ModelForCausalLM};
use teach_core::{DeviceKind, EngineError, SequenceState, TokenId, WeightFormat};

use crate::{BatchOutput, DecoderModel, ModelAssets, loader::SafetensorsLoader, resolve_candle_device};


/// HuggingFace Config 到 Candle Config 的转换
#[derive(Debug, Clone, serde::Deserialize)]
struct HfConfig {
    vocab_size: usize,
    hidden_size: usize,
    intermediate_size: usize,
    num_hidden_layers: usize,
    num_attention_heads: usize,
    #[serde(default)]
    num_key_value_heads: Option<usize>,
    rms_norm_eps: f64,
    #[serde(default = "default_rope_theta")]
    rope_theta: f64,
    #[serde(default)]
    max_position_embeddings: Option<usize>,
    #[serde(default)]
    sliding_window: Option<usize>,
    #[serde(default)]
    hidden_act: Option<String>,
}

fn default_rope_theta() -> f64 {
    10000.0
}

impl From<HfConfig> for Qwen2Config {
    fn from(hf: HfConfig) -> Self {
        let num_key_value_heads = hf.num_key_value_heads.unwrap_or(hf.num_attention_heads);
        let max_position_embeddings = hf.max_position_embeddings.unwrap_or(32768);
        let sliding_window = hf.sliding_window.unwrap_or(4096);
        
        // 解析激活函数
        let hidden_act = match hf.hidden_act.as_deref() {
            Some("gelu") => candle_nn::Activation::Gelu,
            Some("relu") => candle_nn::Activation::Relu,
            Some("silu") | Some("swish") => candle_nn::Activation::Silu,
            _ => candle_nn::Activation::Silu, // 默认 silu
        };

        Self {
            vocab_size: hf.vocab_size,
            hidden_size: hf.hidden_size,
            intermediate_size: hf.intermediate_size,
            num_hidden_layers: hf.num_hidden_layers,
            num_attention_heads: hf.num_attention_heads,
            num_key_value_heads,
            max_position_embeddings,
            sliding_window,
            max_window_layers: hf.num_hidden_layers, // 默认所有层使用滑动窗口
            tie_word_embeddings: false,
            rope_theta: hf.rope_theta,
            rms_norm_eps: hf.rms_norm_eps,
            use_sliding_window: sliding_window > 0,
            hidden_act,
        }
    }
}

/// 真实 Qwen2 模型包装器
pub struct RealQwen2Model {
    model: Mutex<ModelForCausalLM>,
    config: Qwen2Config,
    device: Device,
}

impl RealQwen2Model {
    /// 从资产创建模型
    pub fn from_assets(
        assets: &ModelAssets,
        device_kind: DeviceKind,
        weight_format: WeightFormat,
    ) -> Result<Self, EngineError> {
        // 1. 加载 HF Config 并转换
        let hf_config: HfConfig = {
            let content = std::fs::read_to_string(&assets.config_path)
                .map_err(|e| EngineError::new(format!("failed to read config: {}", e)))?;
            serde_json::from_str(&content)
                .map_err(|e| EngineError::new(format!("failed to parse config: {}", e)))?
        };
        let config: Qwen2Config = hf_config.into();

        // 2. 创建设备
        let device = resolve_candle_device(device_kind)?;

        // 3. 确定数据类型
        let dtype = match weight_format {
            WeightFormat::Fp32 => DType::F32,
            WeightFormat::Fp16 => DType::F16,
            WeightFormat::Bf16 => DType::BF16,
            WeightFormat::Int8 => {
                // INT8 需要量化支持，暂不支持
                return Err(EngineError::new("INT8 not yet supported for real weights"));
            }
        };

        // 4. 加载权重
        let loader = SafetensorsLoader::from_assets(assets, dtype);
        let vb = loader.load(&device)?;

        // 5. 创建模型
        let model = ModelForCausalLM::new(&config, vb)
            .map_err(|e| EngineError::new(format!("failed to create model: {}", e)))?;

        Ok(Self {
            model: Mutex::new(model),
            config,
            device,
        })
    }

    /// 获取配置
    pub fn config(&self) -> &Qwen2Config {
        &self.config
    }

    /// 处理单个序列状态
    fn process_sequence(&self, state: &SequenceState) -> Result<TokenId, EngineError> {
        let mut model = self.model.lock()
            .map_err(|_| EngineError::new("model lock poisoned"))?;

        // 构建输入 tokens
        let input_tokens: Vec<u32> = if state.generated_tokens.is_empty() {
            // Prefill 阶段：使用所有 prompt tokens
            state.prompt_tokens.clone()
        } else {
            // Decode 阶段：只使用最后一个生成的 token
            vec![*state.generated_tokens.last().unwrap()]
        };

        if input_tokens.is_empty() {
            return Err(EngineError::new("empty input tokens"));
        }

        // 创建输入 tensor
        let input_ids = Tensor::new(input_tokens.clone(), &self.device)
            .and_then(|t| t.reshape((1, input_tokens.len())))
            .map_err(|e| EngineError::new(format!("failed to create input tensor: {}", e)))?;

        // 计算 seqlen_offset
        let seqlen_offset = if state.generated_tokens.is_empty() {
            0
        } else {
            state.prompt_tokens.len() + state.generated_tokens.len() - 1
        };

        // 前向传播
        let logits = model
            .forward(&input_ids, seqlen_offset)
            .map_err(|e| EngineError::new(format!("forward failed: {}", e)))?;

        // 获取最后一个位置的 logits
        let vocab_size = self.config.vocab_size;
        let logits_vec: Vec<f32> = logits
            .reshape((vocab_size,))
            .and_then(|t| t.to_dtype(DType::F32))  // 转换为 F32
            .and_then(|t| t.to_vec1())
            .map_err(|e| EngineError::new(format!("failed to flatten logits: {}", e)))?;

        // 贪婪解码：选择概率最高的 token
        let next_token = logits_vec
            .iter()
            .enumerate()
            .max_by(|(_, a), (_, b)| a.partial_cmp(b).unwrap())
            .map(|(idx, _)| idx as u32)
            .unwrap_or(0);

        Ok(next_token)
    }
}

impl DecoderModel for RealQwen2Model {
    fn family(&self) -> &str {
        "qwen2-real"
    }

    fn prefill(&self, states: &[SequenceState]) -> Result<Vec<BatchOutput>, EngineError> {
        states
            .iter()
            .map(|state| {
                let next_token = self.process_sequence(state)?;
                Ok(BatchOutput {
                    request_id: state.request_id,
                    next_token,
                })
            })
            .collect()
    }

    fn decode(&self, states: &[SequenceState]) -> Result<Vec<BatchOutput>, EngineError> {
        // 对于真实模型，decode 和 prefill 使用相同的逻辑
        // 但输入只包含最后一个 token（在 process_sequence 中处理）
        self.prefill(states)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_hf_config_conversion() {
        let hf = HfConfig {
            vocab_size: 151936,
            hidden_size: 896,
            intermediate_size: 4864,
            num_hidden_layers: 24,
            num_attention_heads: 14,
            num_key_value_heads: Some(2),
            rms_norm_eps: 1e-6,
            rope_theta: 1000000.0,
            max_position_embeddings: Some(32768),
            sliding_window: Some(4096),
            hidden_act: Some("silu".to_string()),
        };

        let config: Qwen2Config = hf.into();
        assert_eq!(config.vocab_size, 151936);
        assert_eq!(config.hidden_size, 896);
        assert_eq!(config.num_key_value_heads, 2);
    }
}
