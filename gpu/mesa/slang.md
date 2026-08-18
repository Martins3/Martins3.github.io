https://github.com/shader-slang/slang

Slang can compile shader code to the following targets:

|    Target   |                          Output Formats                          |
|:-----------:|:----------------------------------------------------------------:|
| Direct3D 11 |                               HLSL                               |
| Direct3D 12 |                               HLSL                               |
|    Vulkan   |                            SPIRV, GLSL                           |
|    Metal    |                      Metal Shading Language                      |
|    WebGPU   |                               WGSL                               |
|     CUDA    |                        C++ (compute only)                        |
|    Optix    |                             C++ (WIP)                            |
|     CPU     | C++ (kernel), C++ (host), standalone executable, dynamic library |

简而言之，就是这样的。

## 那么就这么操作吧

```txt
  -- 好玩的东西哦
  {
    "NickTsaizer/shaderdebug",
    ft = { "slang" },
    dependencies = { "3rd/image.nvim" },
  },
```
