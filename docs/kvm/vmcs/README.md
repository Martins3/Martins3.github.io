# VMCS / VMCB 字段地图

直接用浏览器打开 [index.html](index.html)，或在本目录运行 `node serve.mjs`，访问 <http://127.0.0.1:8765>。不需要 npm install、构建步骤、网络字体或后端。静态文件可随当前仓库的 GitHub Pages 流程发布，路径为 `/kvm/vmcs/`。

## 使用

- Intel / AMD / 双架构对照；同色表示共同实现一种功能，不表示逐位等价。
- 悬停或键盘聚焦查看说明；点击字段按钮固定详情，再次点击同一字段或按 Esc 解除固定。点击另一个字段会改为固定新字段；键盘 Enter / Space 同样可用。切换架构或重置视图会解除固定。
- 触屏点击字段跳到详情，使用“返回字段地图”继续浏览。
- 默认显示全部字段，可按功能组和硬件区域筛选。
- 搜索覆盖当前架构、功能组和区域内的所有字段。支持宏名、中文用途、编码、high 编码与 VMCB 偏移。`/` 聚焦搜索框。
- [事件流程文档](events.md) 解释二阶段缺页、I/O 拦截、中断注入中各字段的协作。
- 点击关联字段可跨架构跳转；“复制字段链接”可保存具体条目。

## 修改与扩展

| 文件 | 修改内容 |
| --- | --- |
| `data/vmcs.js` | Intel 字段说明 |
| `data/vmcb.js` | AMD 字段说明 |
| `data/model.js` | 功能组、颜色、源码阅读入口、字段关联 |
| `events.md` | 三条事件流程的讲解 |
| `app.js` | 搜索、筛选和交互逻辑 |
| `style.css` | 布局与视觉样式 |

每个字段都是一个独立对象，没有数据库或隐藏的生成依赖。直接编辑并刷新即可。例如在 `data/vmcs.js` 找到 `"id": "vmcs-201a"`，修改：

```js
"summary": "二阶段页表入口：GPA → HPA",
"detail": "这里写完整解释、适用条件、容易混淆的概念，以及后续补充。"
```

`summary` 用于卡片；`detail` 用于详情；`bits` 是 `[位段, 解释]` 二元数组。`group` 必须是 `model.js` 中的分组 ID。`id` 是字段链接的稳定键，避免更改。编码、宽度、区域属于架构事实，修改后应核对手册和源码。

在 `model.js` 的 `relations` 中把共同实现某项机制的字段 ID 放进同一数组，即可关联高亮。映射是教学用的功能联系，不能理解为两架构位级兼容。字段说明按纯文本显示，不执行 HTML。

## 来源与覆盖范围

- Intel 官方 [SDM](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)，093 版 [Vol. 3C](https://cdrdv2-public.intel.com/929361/326019-093-sdm-vol-3c.pdf) 的 Virtual Machine Control Structures、VM Entries、VM Exits、EPT、APIC 章节及 [Vol. 3D Appendix B](https://cdrdv2-public.intel.com/929362/332831-093-sdm-vol-3d.pdf)。覆盖 205 个独立字段、282 个 full/high 访问编码，high 并入原字段。Natural width 的宽度取决于处理器架构，编码不是内存偏移。
- AMD 官方 [APM Vol. 2 入口](https://docs.amd.com/v/u/en-US/24593_3.44_APM_Vol2)。基础机制与布局实际核对的是 [Rev. 3.41 原版手册镜像](https://kib.kiev.ua/x86docs/AMD/AMD64/24593_APM_v2-r3.41.pdf)，SVM 与 Appendix B；较新的字段以 Linux 定义和调用点为依据。网站不声称覆盖最新版 AMD APM 的全部字段。
- 用户指定的 `/home/martins3/data/kernel/linux-drm`，核对时 HEAD 为 `eb5a10dc0e0026d8144f904b4b98b742a335a1d9`。Intel 对应 `arch/x86/include/asm/vmx.h` 的 `enum vmcs_field`。AMD 对应 `arch/x86/include/asm/svm.h` 的 `vmcb_control_area`、`vmcb_save_area`、`vmcb_seg`；按本地结构展开 107 个非保留字段，不含 Hyper-V 软件保留区。GDTR/IDTR 的 selector/attrib 为保留槽，不作为有效字段展示。AMD save 成员偏移已加 0x400，统一以 VMCB 起点为基准。
- 每个字段提供手册章节；详情中的 KVM 函数是功能组阅读入口，不能推断该函数一定使用该组的每个扩展字段。

重要边界：普通 VMCB、SEV-ES/SNP 的独立 VMSA、Host HSAVE 不是同一布局；多数 GPR 由 KVM 软件切换。VMCS Host 状态由 VMM 预先配置，并不在每次 entry 时自动保存。字段存在于手册或 Linux 枚举也不意味着当前硬件支持。

## 校验

`node tests/verify.mjs` 校验字段键、编码、关联和事件文档的字段链接。`node tests/verify.mjs --kernel /home/martins3/data/kernel/linux-drm` 还会比对本地 VMCS 枚举，并使用 C 编译器对从原头文件抽出的 VMCB 结构执行 `offsetof` / `sizeof` 静态断言；不运行虚拟机，不修改内核。

浏览器测试用 `node tests/browser.mjs`。需要环境中已有 Playwright 和 Chromium；可以通过 `PLAYWRIGHT_MODULE` 指定 Playwright 的 `index.mjs` 绝对路径、`CHROMIUM_PATH` 指定 Chromium 可执行文件。测试自己启动临时静态服务器，截图写入系统临时目录。测试包含点击固定与解除、悬停切换、键盘访问、全字段显示、筛选、跨架构导航、深链接、窄屏、离线打开，以及不依赖浏览器存储的检查。

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
