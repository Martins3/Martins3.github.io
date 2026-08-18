## Neutree Endpoint 部署记录

### 部署的 Endpoint

| Endpoint | 模型 | 引擎 | 状态 |
|----------|------|------|------|
| `quick-start-inference` | `afrideva/Tinystories-gpt-0.1-3m-GGUF` | llama-cpp v0.3.7 | Running |
| `qwen-coder-inference` | `Qwen2.5-Coder-3B-Instruct-GGUF` | llama-cpp v0.3.7 | Running |

```bash
# Qwen2.5-Coder（3B 参数，代码模型）
curl http://10.0.0.2:8000/default/qwen-coder-inference/v1/chat/completions
```

### 遇到的问题及修复

#### 1. ImageRegistry `public-docker` 连接失败

**现象**：`failed to get used image registries: image registry public-docker not ready`

**根因**：neutree-core 容器内 Go 的 `http.Transport` 默认不读取 `HTTP_PROXY` 环境变量，且 docker.io 被 DNS 污染。

**修复**：
- `internal/registry/image.go`：显式添加 `Proxy: http.ProxyFromEnvironment`
- `docker-compose.yml`：为 neutree-core 添加代理环境变量
- 使用 `CGO_ENABLED=0` 静态编译，通过 volume 挂载自定义二进制

#### 2. Ray 容器内网络不可达（`Network is unreachable`）

**现象**：Engine 容器无法访问 HuggingFace Hub 下载模型。

**根因**：Ray 容器和 Engine 容器都没有继承主机的代理配置。

**修复**：
- `internal/cluster/ray_ssh_operation.go`：`generateRayClusterConfig` 中自动将 neutree-core 的代理环境变量注入 `Docker.RunOptions`
- `internal/orchestrator/ray_orchestrator.go`：`buildEngineContainerConfigs` 中为 base 和 backend 容器的 `run_options` 追加代理环境变量

#### 3. 本地模型部署支持（`file://` URL）

**现象**：用户本地已有模型，不想重复从 HuggingFace Hub 下载。

**修复**：
- `internal/model_registry/hugging_face.go`：允许 `file://` URL，跳过 HTTP 健康检查
- `internal/orchestrator/ray_orchestrator.go`：
  - `EndpointToServeApplication`：识别 `file://` URL，将 `registry_type` 改为 `local`
  - `buildEngineContainerConfigs`：为 engine 容器添加 `-v <local_path>:<local_path>` 挂载

### 测试示例

```bash
# Qwen2.5-Coder 代码生成
curl -X POST http://10.0.0.2:8000/default/qwen-coder-inference/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "Qwen2.5-Coder-3B-Instruct-GGUF",
    "messages": [{"role": "user", "content": "Write a Python function to calculate factorial"}],
    "max_tokens": 200,
    "temperature": 0.7
  }'

# 代码补全
curl -X POST http://10.0.0.2:8000/default/qwen-coder-inference/v1/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "Qwen2.5-Coder-3B-Instruct-GGUF",
    "prompt": "def fibonacci(n):\n    \"\"\"Return the nth Fibonacci number.\"\"\"\n    ",
    "max_tokens": 100,
    "temperature": 0.2
  }'
```
