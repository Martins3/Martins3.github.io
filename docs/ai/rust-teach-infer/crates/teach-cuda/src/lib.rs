use std::env;
use std::path::Path;
use std::process::Command;

use teach_core::{CudaConfig, EngineError};

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct CudaEnvironment {
    pub cuda_home: String,
    pub nvcc_path: String,
    pub arch: String,
    pub note: String,
}

pub trait CudaBackend {
    fn label(&self) -> &'static str;
    fn environment(&self) -> &CudaEnvironment;
}

#[derive(Clone, Debug)]
pub struct StubCudaBackend {
    env: CudaEnvironment,
}

impl StubCudaBackend {
    pub fn from_environment(env: CudaEnvironment) -> Self {
        Self { env }
    }

    pub fn probe(config: &CudaConfig) -> Result<Self, EngineError> {
        let cuda_home = env::var("CUDA_HOME").unwrap_or_else(|_| "/usr/local/cuda".to_string());
        let nvcc_path = format!("{cuda_home}/bin/nvcc");
        if !Path::new(&nvcc_path).exists() {
            return Err(EngineError::new(format!(
                "missing nvcc at {nvcc_path}; source /home/martins3/data/vn/gpu_demo/cuda/env.sh first"
            )));
        }

        let nvcc_version = Command::new(&nvcc_path)
            .arg("--version")
            .output()
            .map_err(|err| EngineError::new(format!("failed to run nvcc: {err}")))?;
        if !nvcc_version.status.success() {
            return Err(EngineError::new("nvcc exists but failed to execute"));
        }

        let gpu_codes = Command::new(&nvcc_path)
            .arg("--list-gpu-code")
            .output()
            .map_err(|err| EngineError::new(format!("failed to query nvcc gpu codes: {err}")))?;
        let gpu_codes = String::from_utf8_lossy(&gpu_codes.stdout);
        if !gpu_codes.contains(&config.preferred_arch) {
            return Err(EngineError::new(format!(
                "nvcc does not advertise {}; expected GTX 1060 compatible toolchain",
                config.preferred_arch
            )));
        }

        Ok(Self {
            env: CudaEnvironment {
                cuda_home,
                nvcc_path,
                arch: config.preferred_arch.clone(),
                note: "stub backend: use this boundary to plug in a C/CUDA shim and cuBLAS calls"
                    .to_string(),
            },
        })
    }
}

impl CudaBackend for StubCudaBackend {
    fn label(&self) -> &'static str {
        "stub-cuda"
    }

    fn environment(&self) -> &CudaEnvironment {
        &self.env
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn probe_fails_without_nvcc() {
        let old = env::var("CUDA_HOME").ok();
        unsafe {
            env::set_var("CUDA_HOME", "/definitely/missing");
        }
        let err = StubCudaBackend::probe(&CudaConfig::default()).expect_err("probe should fail");
        assert!(err.message().contains("missing nvcc"));
        match old {
            Some(value) => unsafe {
                env::set_var("CUDA_HOME", value);
            },
            None => unsafe {
                env::remove_var("CUDA_HOME");
            },
        }
    }
}
