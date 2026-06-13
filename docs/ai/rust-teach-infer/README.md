# rust-teach-infer

`rust-teach-infer` is a teaching-oriented inference framework skeleton in Rust.
It is shaped like a much smaller `vLLM` or `SGLang`:

- request queue
- continuous batching
- prefill vs decode
- paged KV cache
- model runner boundary
- CUDA backend boundary

The current implementation now includes:

- a real `tokenizer.json` + `config.json` loading path
- a Candle-based CPU reference model stub
- Hugging Face asset preparation hooks
- an offline local fixture model so the project runs without network
- a CUDA backend abstraction and environment probe for the GTX 1060 3GB setup documented in `/home/martins3/data/vn/gpu_demo/cuda/README.md`

## Layout

- `crates/teach-core`: shared request and event types
- `crates/teach-engine`: scheduler, engine loop, paged KV cache
- `crates/teach-model`: HF asset preparation, tokenizer runtime, Candle reference model
- `crates/teach-cuda`: CUDA environment probe and backend abstraction
- `crates/teach-infer`: CLI
- `crates/teach-mode`: teaching mode framework (trace, visualize, step, profile)

## Usage

```bash
cd /home/martins3/data/vn/docs/ai/rust-teach-infer
cargo run -p teach-infer -- run --prompt "hello world"
```

This uses the bundled offline fixture under `models/dev-qwen2-fixture`.

Interactive mode:

```bash
cargo run -p teach-infer -- chat
```

Prepare a model directory:

```bash
cargo run -p teach-infer -- prepare-model
```

Attempt to prepare a remote Hugging Face model:

```bash
cargo run -p teach-infer -- prepare-model \
  --model-id Qwen/Qwen2.5-0.5B \
  --model-dir ./models/Qwen2.5-0.5B
```

Engine benchmark stub:

```bash
cargo run -p teach-infer -- bench --prompt-len 16 --decode-len 8
```

## Teaching Mode (教学模式)

The `teach-mode` crate provides teaching/learning features:

### Visual Mode (时间线可视化)

```bash
cargo run -p teach-infer -- run --prompt "hello" --teach-mode visual
```

Shows ASCII timeline of request execution:
- `S` = Submit
- `P` = Prefill
- `D` = Decode  
- `*` = Token
- `F` = Finish
- `A` = KV Cache Allocate
- `R` = KV Cache Release

### Trace Mode (详细追踪)

```bash
cargo run -p teach-infer -- run --prompt "hello" --teach-mode trace
```

Collects detailed trace events in JSON format.

### Profile Mode (性能分析)

```bash
cargo run -p teach-infer -- profile --prompt-len 64 --decode-len 32
```

Outputs performance metrics:
- TTFT (Time To First Token)
- Throughput (tokens/s)
- Inter-token latency
- Peak batch size
- KV Cache utilization

### Teaching Scenarios (教学场景)

```bash
# Demonstrate batching mechanism
cargo run -p teach-infer -- teach --scenario batching

# Demonstrate KV Cache management
cargo run -p teach-infer -- teach --scenario kv_cache

# Demonstrate continuous batching
cargo run -p teach-infer -- teach --scenario continuous_batching
```

### Real Weights Mode (真实权重)

使用 HuggingFace 真实权重进行推理：

```bash
# 使用真实权重 (需要下载 Qwen2.5-0.5B 权重)
cargo run -p teach-infer -- run --prompt "hello" --real-weights --weight-format fp16

# 准备真实模型权重
cargo run -p teach-infer -- prepare-model \
  --model-id Qwen/Qwen2.5-0.5B \
  --model-dir ./models/Qwen2.5-0.5B

# 使用真实权重进行性能分析
cargo run -p teach-infer -- profile --prompt-len 64 --decode-len 32 --real-weights
```

**显存要求** (Qwen2.5-0.5B):
- FP16: ~2.0 GB (适合 GTX 1060 3GB)
- FP32: ~3.5 GB (需要更多显存)

启用 GPU 路径时，推荐统一通过项目脚本注入环境：

```bash
./scripts/with_cuda_env.sh \
  cargo run -p teach-infer --features cuda -- run \
  --prompt "hello" \
  --device cuda \
  --real-weights \
  --weight-format fp16
```

这个脚本会：

- 自动检测 `CUDA_HOME`（优先 `/usr/local/cuda-13.1`，回退到其他版本）
- 自动查找 Nix 64 位 gcc lib，提供兼容的 `libstdc++.so.6`
- 固定 `NVCC_CCBIN=/usr/bin/g++-14`
- 在项目内生成 `.cuda-runtime/`，只暴露 `libcuda.so.1` 和 `libnvidia-ptxjitcompiler.so.1`
- 避免把整个 `/usr/lib64` 塞进 `LD_LIBRARY_PATH`，从而绕开 Nix loader 和宿主 glibc 混用导致的崩溃

如果没有使用 `--features cuda` 重新构建，`--device cuda` 现在会明确报错，而不是悄悄回退到 CPU。

## CUDA Notes

The CUDA teaching path is modeled around:

- GPU: GTX 1060 3GB (教学参考)
- toolkit: CUDA 12.8
- arch: `sm_61`

当前宿主环境实际配置：

- GPU: NVIDIA GeForce RTX 5060 Ti
- toolkit: CUDA 13.1
- arch: `sm_120`

`teach-core` 中的默认 `preferred_arch` 已适配为 `sm_120`，可直接在当前环境编译运行。

The current Rust implementation does not yet launch custom CUDA kernels. Instead it:

- probes for the documented CUDA environment
- compiles the Candle CUDA stack on the detected arch via a local `candle-kernels` compatibility patch
- fails fast when `--device cuda` is requested without rebuilding with `--features cuda`
- runs correctly on this host via `scripts/with_cuda_env.sh`, which provides only the required NVIDIA driver libraries to the Nix-linked binary
- exposes a backend trait boundary
- leaves clear extension points for a C/CUDA shim and cuBLAS-backed execution

Current limitations:

- the CPU reference uses Candle tensor ops but not real Qwen weights yet
- remote Hugging Face downloads depend on external network access
- the CUDA path still relies on Candle rather than this repository's own kernel launch path

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
