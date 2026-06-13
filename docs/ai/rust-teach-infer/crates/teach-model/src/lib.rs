use std::fs;
use std::path::{Path, PathBuf};

use candle_core::{DType, Device, Tensor};
use hf_hub::{Repo, RepoType, api::sync::Api};
use serde::Deserialize;
use teach_core::{
    DeviceKind, EngineError, GenerateRequest, ModelConfig, ModelImplementation, SequenceState, TokenId, WeightFormat,
};
use tokenizers::Tokenizer;

pub mod loader;
pub mod qwen2_real;

pub use loader::{SafetensorsLoader, WeightStatus};
pub use qwen2_real::RealQwen2Model;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct BatchInput {
    pub request_id: u64,
    pub prompt_len: usize,
    pub generated_len: usize,
    pub last_token: Option<TokenId>,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct BatchOutput {
    pub request_id: u64,
    pub next_token: TokenId,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct SpecialTokenIds {
    pub bos: Option<TokenId>,
    pub eos: Option<TokenId>,
    pub pad: Option<TokenId>,
}

pub trait TokenizerRuntime: Send + Sync {
    fn encode(&self, text: &str) -> Result<Vec<TokenId>, EngineError>;
    fn decode_token(&self, token: TokenId) -> Result<String, EngineError>;
    fn decode_tokens(&self, tokens: &[TokenId]) -> Result<String, EngineError>;
    fn special_tokens(&self) -> &SpecialTokenIds;
    fn vocab_size(&self) -> usize;
}

pub trait DecoderModel: Send + Sync {
    fn family(&self) -> &str;
    fn prefill(&self, states: &[SequenceState]) -> Result<Vec<BatchOutput>, EngineError>;
    fn decode(&self, states: &[SequenceState]) -> Result<Vec<BatchOutput>, EngineError>;
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ModelAssets {
    pub model_dir: PathBuf,
    pub config_path: PathBuf,
    pub tokenizer_path: PathBuf,
    pub generation_config_path: Option<PathBuf>,
    pub tokenizer_config_path: Option<PathBuf>,
    pub weight_paths: Vec<PathBuf>,
}

#[derive(Clone, Debug, Deserialize, PartialEq)]
pub struct Qwen2Config {
    pub vocab_size: usize,
    pub hidden_size: usize,
    pub intermediate_size: usize,
    pub num_hidden_layers: usize,
    pub num_attention_heads: usize,
    #[serde(default)]
    pub num_key_value_heads: Option<usize>,
    pub rms_norm_eps: f64,
    #[serde(default = "default_rope_theta")]
    pub rope_theta: f64,
    #[serde(default)]
    pub bos_token_id: Option<TokenId>,
    #[serde(default)]
    pub eos_token_id: Option<TokenId>,
    #[serde(default)]
    pub pad_token_id: Option<TokenId>,
}

fn default_rope_theta() -> f64 {
    10000.0
}

fn cuda_debug_enabled() -> bool {
    std::env::var("TEACH_CUDA_DEBUG").is_ok()
}

fn cuda_debug_log(message: impl AsRef<str>) {
    if cuda_debug_enabled() {
        eprintln!("[teach-cuda-debug] {}", message.as_ref());
    }
}

pub fn prepare_assets(config: &ModelConfig) -> Result<ModelAssets, EngineError> {
    fs::create_dir_all(&config.model_dir).map_err(|err| {
        EngineError::new(format!(
            "failed to create model directory {}: {err}",
            config.model_dir.display()
        ))
    })?;

    let config_path = ensure_local_or_download(config, "config.json")?;
    let tokenizer_path = ensure_local_or_download(config, "tokenizer.json")?;
    let generation_config_path = optional_local_or_download(config, "generation_config.json")?;
    let tokenizer_config_path = optional_local_or_download(config, "tokenizer_config.json")?;
    // 尝试下载权重文件
    let model_index_path = ensure_local_or_download(config, "model.safetensors.index.json").ok();
    
    let weight_paths = if let Some(ref index_path) = model_index_path {
        // 分片权重
        let shards = parse_weight_index(index_path)?;
        let mut paths = Vec::new();
        for shard in shards {
            let shard_path = ensure_local_or_download(config, &shard)?;
            paths.push(shard_path);
        }
        paths
    } else {
        // 单文件权重
        match ensure_local_or_download(config, "model.safetensors") {
            Ok(path) => vec![path],
            Err(_) => Vec::new(),
        }
    };

    Ok(ModelAssets {
        model_dir: config.model_dir.clone(),
        config_path,
        tokenizer_path,
        generation_config_path,
        tokenizer_config_path,
        weight_paths,
    })
}

fn ensure_local_or_download(config: &ModelConfig, filename: &str) -> Result<PathBuf, EngineError> {
    let local = config.model_dir.join(filename);
    if local.exists() {
        return Ok(local);
    }
    download_file(config, filename)
}

fn optional_local_or_download(
    config: &ModelConfig,
    filename: &str,
) -> Result<Option<PathBuf>, EngineError> {
    let local = config.model_dir.join(filename);
    if local.exists() {
        return Ok(Some(local));
    }
    match download_file(config, filename) {
        Ok(path) => Ok(Some(path)),
        Err(_) => Ok(None),
    }
}

fn download_file(config: &ModelConfig, filename: &str) -> Result<PathBuf, EngineError> {
    let api = Api::new().map_err(|err| EngineError::new(format!("hf api init failed: {err}")))?;
    let repo = api.repo(Repo::with_revision(
        config.model_id.clone(),
        RepoType::Model,
        "main".to_string(),
    ));
    let path = repo
        .get(filename)
        .map_err(|err| EngineError::new(format!("failed to fetch {filename}: {err}")))?;
    let destination = config.model_dir.join(filename);
    if path != destination {
        fs::copy(&path, &destination).map_err(|err| {
            EngineError::new(format!(
                "failed to copy {} to {}: {err}",
                path.display(),
                destination.display()
            ))
        })?;
    }
    Ok(destination)
}

fn parse_weight_index(index_path: &Path) -> Result<Vec<String>, EngineError> {
    #[derive(Deserialize)]
    struct IndexFile {
        weight_map: std::collections::BTreeMap<String, String>,
    }

    let content = fs::read_to_string(index_path).map_err(|err| {
        EngineError::new(format!(
            "failed to read weight index {}: {err}",
            index_path.display()
        ))
    })?;
    let parsed: IndexFile = serde_json::from_str(&content)
        .map_err(|err| EngineError::new(format!("invalid weight index json: {err}")))?;
    let mut unique = parsed.weight_map.into_values().collect::<Vec<_>>();
    unique.sort();
    unique.dedup();
    Ok(unique)
}

#[derive(Debug)]
pub struct HfTokenizerRuntime {
    tokenizer: Tokenizer,
    special: SpecialTokenIds,
    vocab_size: usize,
}

impl HfTokenizerRuntime {
    pub fn from_assets(assets: &ModelAssets, qwen_config: &Qwen2Config) -> Result<Self, EngineError> {
        let tokenizer = Tokenizer::from_file(&assets.tokenizer_path)
            .map_err(|err| EngineError::new(format!("failed to load tokenizer: {err}")))?;
        let vocab_size = tokenizer.get_vocab_size(false);
        Ok(Self {
            tokenizer,
            special: SpecialTokenIds {
                bos: qwen_config.bos_token_id,
                eos: qwen_config.eos_token_id,
                pad: qwen_config.pad_token_id,
            },
            vocab_size,
        })
    }
}

impl TokenizerRuntime for HfTokenizerRuntime {
    fn encode(&self, text: &str) -> Result<Vec<TokenId>, EngineError> {
        let encoding = self
            .tokenizer
            .encode(text, true)
            .map_err(|err| EngineError::new(format!("tokenizer encode failed: {err}")))?;
        Ok(encoding.get_ids().to_vec())
    }

    fn decode_token(&self, token: TokenId) -> Result<String, EngineError> {
        self.tokenizer
            .decode(&[token], true)
            .map_err(|err| EngineError::new(format!("tokenizer decode failed: {err}")))
    }

    fn decode_tokens(&self, tokens: &[TokenId]) -> Result<String, EngineError> {
        let mut pieces = Vec::new();
        for token in tokens {
            let piece = self.decode_token(*token)?;
            if !piece.is_empty() {
                pieces.push(piece);
            }
        }
        Ok(pieces.join(" "))
    }

    fn special_tokens(&self) -> &SpecialTokenIds {
        &self.special
    }

    fn vocab_size(&self) -> usize {
        self.vocab_size
    }
}

pub fn load_qwen2_config(path: &Path) -> Result<Qwen2Config, EngineError> {
    let content = fs::read_to_string(path)
        .map_err(|err| EngineError::new(format!("failed to read config {}: {err}", path.display())))?;
    serde_json::from_str(&content).map_err(|err| EngineError::new(format!("invalid qwen config: {err}")))
}

pub fn resolve_candle_device(device_kind: DeviceKind) -> Result<Device, EngineError> {
    match device_kind {
        DeviceKind::Cpu => Ok(Device::Cpu),
        DeviceKind::Cuda => resolve_cuda_device(),
    }
}

#[cfg(feature = "cuda")]
fn resolve_cuda_device() -> Result<Device, EngineError> {
    cuda_debug_log("creating candle CUDA device 0");
    Device::new_cuda(0).map_err(|err| {
        EngineError::new(format!(
            "failed to create CUDA device 0: {err}; source /home/martins3/data/vn/gpu_demo/cuda/env.sh first"
        ))
    }).inspect(|_| cuda_debug_log("created candle CUDA device 0"))
}

#[cfg(not(feature = "cuda"))]
fn resolve_cuda_device() -> Result<Device, EngineError> {
    Err(EngineError::new(
        "CUDA support is not compiled in; rebuild with `cargo run -p teach-infer --features cuda -- ...` after `source /home/martins3/data/vn/gpu_demo/cuda/env.sh`",
    ))
}

#[derive(Debug)]
pub struct CandleQwen2Reference {
    config: Qwen2Config,
    device: Device,
    weight_format: WeightFormat,
}

impl CandleQwen2Reference {
    pub fn from_assets(
        assets: &ModelAssets,
        device_kind: DeviceKind,
        weight_format: WeightFormat,
    ) -> Result<Self, EngineError> {
        let config = load_qwen2_config(&assets.config_path)?;
        let device = resolve_candle_device(device_kind)?;
        Ok(Self {
            config,
            device,
            weight_format,
        })
    }

    pub fn config(&self) -> &Qwen2Config {
        &self.config
    }

    fn next_token_for_state(&self, state: &SequenceState) -> Result<TokenId, EngineError> {
        let eos = self.config.eos_token_id.unwrap_or(151645);
        if state.generated_tokens.len() >= 8 {
            return Ok(eos);
        }

        let mut combined = state.prompt_tokens.clone();
        combined.extend(state.generated_tokens.iter().copied());
        if combined.is_empty() {
            combined.push(self.config.bos_token_id.unwrap_or(0));
        }

        cuda_debug_log(format!(
            "reference model building tensor on {:?} with {} tokens",
            self.device,
            combined.len()
        ));
        let tensor = Tensor::new(combined.as_slice(), &self.device)
            .and_then(|tensor| tensor.to_dtype(DType::F32))
            .map_err(|err| EngineError::new(format!("candle tensor build failed: {err}")))?;
        cuda_debug_log("reference model tensor build succeeded");
        let sum = tensor
            .sum_all()
            .and_then(|tensor| tensor.to_scalar::<f32>())
            .map_err(|err| EngineError::new(format!("candle reduction failed: {err}")))?;
        cuda_debug_log("reference model reduction succeeded");
        let vocab = self.config.vocab_size.max(32) as u32;
        let salt = match self.weight_format {
            WeightFormat::Fp32 => 17u32,
            WeightFormat::Fp16 => 19u32,
            WeightFormat::Bf16 => 23u32,
            WeightFormat::Int8 => 29u32,
        };
        let value = ((sum as u32).wrapping_mul(13).wrapping_add(salt)) % vocab;
        if value == eos {
            Ok((value + 1) % vocab)
        } else {
            Ok(value)
        }
    }
}

impl DecoderModel for CandleQwen2Reference {
    fn family(&self) -> &str {
        "qwen2-candle-reference"
    }

    fn prefill(&self, states: &[SequenceState]) -> Result<Vec<BatchOutput>, EngineError> {
        states
            .iter()
            .map(|state| {
                Ok(BatchOutput {
                    request_id: state.request_id,
                    next_token: self.next_token_for_state(state)?,
                })
            })
            .collect()
    }

    fn decode(&self, states: &[SequenceState]) -> Result<Vec<BatchOutput>, EngineError> {
        self.prefill(states)
    }
}

pub struct ModelRuntime {
    pub assets: ModelAssets,
    pub tokenizer: Box<dyn TokenizerRuntime>,
    pub model: Box<dyn DecoderModel>,
    pub stop_token: Option<TokenId>,
}

pub fn build_model_runtime(
    config: &ModelConfig,
    device_kind: DeviceKind,
    weight_format: WeightFormat,
) -> Result<ModelRuntime, EngineError> {
    build_model_runtime_with_impl(config, device_kind, weight_format, ModelImplementation::Reference)
}

pub fn build_model_runtime_with_impl(
    config: &ModelConfig,
    device_kind: DeviceKind,
    weight_format: WeightFormat,
    implementation: ModelImplementation,
) -> Result<ModelRuntime, EngineError> {
    let assets = prepare_assets(config)?;
    let qwen_config = load_qwen2_config(&assets.config_path)?;
    let tokenizer = HfTokenizerRuntime::from_assets(&assets, &qwen_config)?;
    let stop_token = tokenizer.special_tokens().eos;
    
    let model: Box<dyn DecoderModel> = match implementation {
        ModelImplementation::Reference => {
            Box::new(CandleQwen2Reference::from_assets(&assets, device_kind, weight_format)?)
        }
        ModelImplementation::Real => {
            Box::new(qwen2_real::RealQwen2Model::from_assets(&assets, device_kind, weight_format)?)
        }
    };
    
    Ok(ModelRuntime {
        assets,
        tokenizer: Box::new(tokenizer),
        model,
        stop_token,
    })
}

pub fn prepare_request<T: TokenizerRuntime + ?Sized>(
    tokenizer: &T,
    mut request: GenerateRequest,
) -> Result<GenerateRequest, EngineError> {
    request.prompt_tokens = tokenizer.encode(&request.prompt)?;
    Ok(request)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_qwen2_config() {
        let json = r#"{
            "vocab_size": 151936,
            "hidden_size": 896,
            "intermediate_size": 4864,
            "num_hidden_layers": 24,
            "num_attention_heads": 14,
            "num_key_value_heads": 2,
            "rms_norm_eps": 1e-6,
            "rope_theta": 1000000.0,
            "bos_token_id": 151643,
            "eos_token_id": 151645,
            "pad_token_id": 151643
        }"#;
        let parsed: Qwen2Config = serde_json::from_str(json).expect("config json should parse");
        assert_eq!(parsed.vocab_size, 151936);
        assert_eq!(parsed.eos_token_id, Some(151645));
    }

    #[cfg(not(feature = "cuda"))]
    #[test]
    fn requesting_cuda_without_feature_returns_error() {
        let err = resolve_candle_device(DeviceKind::Cuda).expect_err("cuda should require feature");
        assert!(err.message().contains("CUDA support is not compiled in"));
    }
}
