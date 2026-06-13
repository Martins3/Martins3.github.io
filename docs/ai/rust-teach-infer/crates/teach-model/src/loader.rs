//! Safetensors 权重加载器

use std::collections::HashMap;
use std::path::{Path, PathBuf};

use candle_core::{Device, DType, Tensor};
use candle_nn::VarBuilder;
use safetensors::SafeTensors;
use teach_core::{EngineError, ModelConfig};

use crate::ModelAssets;

/// 权重文件状态
#[derive(Clone, Debug, PartialEq)]
pub enum WeightStatus {
    /// 权重完整
    Complete,
    /// 缺少文件
    Missing(Vec<String>),
    /// 分片索引缺失
    ShardIndexMissing,
    /// 配置缺失
    ConfigMissing,
}

/// Safetensors 权重加载器
pub struct SafetensorsLoader {
    model_dir: PathBuf,
    dtype: DType,
}

impl SafetensorsLoader {
    /// 创建新的加载器
    pub fn new(model_dir: PathBuf, dtype: DType) -> Self {
        Self { model_dir, dtype }
    }

    /// 从 ModelConfig 创建
    pub fn from_config(config: &ModelConfig, dtype: DType) -> Self {
        Self::new(config.model_dir.clone(), dtype)
    }

    /// 从 ModelAssets 创建
    pub fn from_assets(assets: &ModelAssets, dtype: DType) -> Self {
        Self::new(assets.model_dir.clone(), dtype)
    }

    /// 检查权重状态
    pub fn check_weights(&self) -> WeightStatus {
        // 检查配置
        let config_path = self.model_dir.join("config.json");
        if !config_path.exists() {
            return WeightStatus::ConfigMissing;
        }

        // 检查单文件权重
        let single_path = self.model_dir.join("model.safetensors");
        if single_path.exists() {
            return WeightStatus::Complete;
        }

        // 检查分片权重
        let index_path = self.model_dir.join("model.safetensors.index.json");
        if !index_path.exists() {
            return WeightStatus::ShardIndexMissing;
        }

        // 解析索引检查分片
        match self.parse_shard_index(&index_path) {
            Ok(shards) => {
                let missing: Vec<String> = shards
                    .into_iter()
                    .filter(|s| !self.model_dir.join(s).exists())
                    .collect();
                if missing.is_empty() {
                    WeightStatus::Complete
                } else {
                    WeightStatus::Missing(missing)
                }
            }
            Err(_) => WeightStatus::ShardIndexMissing,
        }
    }

    /// 加载权重到 VarBuilder
    pub fn load(&self, device: &Device) -> Result<VarBuilder<'_>, EngineError> {
        match self.check_weights() {
            WeightStatus::Complete => self.load_weights(device),
            WeightStatus::Missing(files) => Err(EngineError::new(format!(
                "missing weight files: {}",
                files.join(", ")
            ))),
            WeightStatus::ShardIndexMissing => Err(EngineError::new(
                "model.safetensors or model.safetensors.index.json not found"
            )),
            WeightStatus::ConfigMissing => Err(EngineError::new("config.json not found")),
        }
    }

    /// 内部：加载权重
    fn load_weights(&self, device: &Device) -> Result<VarBuilder<'_>, EngineError> {
        let single_path = self.model_dir.join("model.safetensors");
        
        if single_path.exists() {
            // 单文件加载
            self.load_single_file(&single_path, device)
        } else {
            // 分片加载
            let index_path = self.model_dir.join("model.safetensors.index.json");
            self.load_sharded(&index_path, device)
        }
    }

    /// 加载单文件权重
    fn load_single_file(
        &self,
        path: &Path,
        device: &Device,
    ) -> Result<VarBuilder<'_>, EngineError> {
        let tensor_map = self.load_tensor_map(path, device)?;
        Ok(VarBuilder::from_tensors(tensor_map, self.dtype, device))
    }

    /// 加载分片权重
    fn load_sharded(
        &self,
        index_path: &Path,
        device: &Device,
    ) -> Result<VarBuilder<'_>, EngineError> {
        let shards = self.parse_shard_index(index_path)?;
        let mut all_tensors = HashMap::new();

        for shard in shards {
            let shard_path = self.model_dir.join(&shard);
            for (name, tensor) in self.load_tensor_map(&shard_path, device)? {
                if all_tensors.insert(name.clone(), tensor).is_some() {
                    return Err(EngineError::new(format!(
                        "duplicate tensor {name} found while loading sharded weights"
                    )));
                }
            }
        }

        Ok(VarBuilder::from_tensors(all_tensors, self.dtype, device))
    }

    fn load_tensor_map(
        &self,
        path: &Path,
        device: &Device,
    ) -> Result<HashMap<String, Tensor>, EngineError> {
        let bytes = std::fs::read(path)
            .map_err(|e| EngineError::new(format!("failed to read {}: {}", path.display(), e)))?;

        let tensors = SafeTensors::deserialize(&bytes)
            .map_err(|e| EngineError::new(format!("failed to parse safetensors: {}", e)))?;

        let mut tensor_map = HashMap::new();
        for name in tensors.names() {
            let view = tensors
                .tensor(name)
                .map_err(|e| EngineError::new(format!("failed to get tensor {}: {}", name, e)))?;

            let shape: Vec<usize> = view.shape().iter().copied().collect();
            let data = view.data();

            let tensor = match view.dtype() {
                safetensors::Dtype::F32 => {
                    let vec: Vec<f32> = data
                        .chunks_exact(4)
                        .map(|b| f32::from_le_bytes([b[0], b[1], b[2], b[3]]))
                        .collect();
                    Tensor::from_vec(vec, shape, device)
                }
                safetensors::Dtype::F16 => {
                    let vec: Vec<half::f16> = data
                        .chunks_exact(2)
                        .map(|b| half::f16::from_le_bytes([b[0], b[1]]))
                        .collect();
                    Tensor::from_vec(vec, shape, device)
                }
                safetensors::Dtype::BF16 => {
                    let vec: Vec<half::bf16> = data
                        .chunks_exact(2)
                        .map(|b| half::bf16::from_le_bytes([b[0], b[1]]))
                        .collect();
                    Tensor::from_vec(vec, shape, device)
                }
                other => return Err(EngineError::new(format!("unsupported dtype: {:?}", other))),
            }
            .map_err(|e| EngineError::new(format!("failed to create tensor: {}", e)))?;

            let tensor = if self.dtype != tensor.dtype() {
                tensor
                    .to_dtype(self.dtype)
                    .map_err(|e| EngineError::new(format!("failed to convert dtype: {}", e)))?
            } else {
                tensor
            };

            tensor_map.insert(name.to_string(), tensor);
        }

        Ok(tensor_map)
    }

    /// 解析分片索引
    fn parse_shard_index(&self, index_path: &Path) -> Result<Vec<String>, EngineError> {
        #[derive(serde::Deserialize)]
        struct IndexFile {
            weight_map: HashMap<String, String>,
        }

        let content = std::fs::read_to_string(index_path).map_err(|e| {
            EngineError::new(format!("failed to read index: {}", e))
        })?;

        let parsed: IndexFile = serde_json::from_str(&content).map_err(|e| {
            EngineError::new(format!("failed to parse index: {}", e))
        })?;

        // 提取唯一的分片文件名
        let mut shards: Vec<String> = parsed
            .weight_map
            .values()
            .cloned()
            .collect::<std::collections::HashSet<_>>()
            .into_iter()
            .collect();
        shards.sort();
        Ok(shards)
    }

    /// 获取需要的分片文件列表
    pub fn get_required_shards(&self) -> Result<Vec<String>, EngineError> {
        let index_path = self.model_dir.join("model.safetensors.index.json");
        if index_path.exists() {
            self.parse_shard_index(&index_path)
        } else {
            // 检查单文件
            let single = self.model_dir.join("model.safetensors");
            if single.exists() {
                Ok(vec!["model.safetensors".to_string()])
            } else {
                Ok(vec![])
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_check_weights_missing() {
        let loader = SafetensorsLoader::new(
            PathBuf::from("/nonexistent/path"),
            DType::F32,
        );
        assert_eq!(loader.check_weights(), WeightStatus::ConfigMissing);
    }
}
