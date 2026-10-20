# 三阶魔方入门

直接打开 `index.html`，或使用现有 GitHub Pages 路径 `/math/cube/`。无需构建、外部字体、CDN 或网络。先认整块，再做三个立体动作实验，接着进入七步层先复原；抽象数学解释放在可展开的进阶说明中。

`cube.js` 使用整数三维坐标模拟 54 个贴纸的真实层转动。`app.js` 提供七步教学，示例通过公式逆序生成。七步均使用白底黄顶；第二步使用 `R U R′ U′`，与原理实验一致，覆盖需要 1、3、5 组的三种朝向。这些案例用于解释动作，不会自动识别实体魔方或为任意状态生成解法。

`principles.js` 提供三个递进实验：单层转动、四手动作的六组循环、`F [R U R′ U′] F′` 的作用范围。`visuals.js` 根据实际贴纸坐标生成 SVG 立体图，持续跟踪同一块，并提供不改变底层状态的前后观察视角。六面展开图作为可展开的辅助视图。

验证：在仓库根目录执行 `node docs/math/cube/tests/verify.cjs`。浏览器中打开页面可检查原理实验与时间点选择、步骤选择、演示切换、前进/后退、自动播放/暂停、重置、自由转动和窄屏布局。

Windows 环境可执行 `node docs/math/cube/tests/browser.cjs`，使用已安装的 Edge 检查实际页面、原理实验切换、七步原因说明和 390px 排版，并把截图放到系统临时目录。可用 `BROWSER_PATH` 指定支持 Chrome DevTools Protocol 的浏览器路径。

资料核对范围和重组理由见 [资料与编排说明](./sources.md)。七步保留统一的层先流程，未混用不同视频的最后一层步骤。

所有“保留哪些块”“循环几组回到起点”的具体效果都由本地模型验证；测试同时检查位置与朝向，不只检查面上的颜色。

## 参考资料
- [MIT《The Mathematics of the Rubik’s Cube》](https://web.mit.edu/sp.268/www/rubik.pdf)：阅读 20 页讲义，重点核对 Inverses、Macros、Commutator、Conjugation 与 Bottom up method。读取了共轭示例的 PDF 图页。它是 2009 年讲义；未把其中历史性的最短步数上界或排版错误作为本站结论。
- [震我一下魔方宅的层先入门教程](https://www.bilibili.com/video/BV1hJ411p7k6/)：核对八个分集的标题与顺序，下载公开视频的画面，按约 20 秒间隔抽帧检查认识魔方、十字、白角、中层及最后一层的示范。第一讲实际拆开魔方展示整块与中心，后续逐块指认目标和相关中心。
- [2017 入门教程](https://www.bilibili.com/video/BV1Xs411Q7ob/)：核对五个分集、简介公式与教学画面。第一层画面明确以 `R U R′ U′` 的重复作为短动作示范；第二层与第三层进一步组合动作，最后一层采用另一套朝向／位置处理顺序。
- [北师大魔方课程](https://www.bilibili.com/video/BV1eET3zsEqu/)：核对 53 个分集的目录，重点查看第一讲《魔方背后的数学原理》的字幕画面与幻灯片。该讲区分动作串与作用效果，介绍等效动作、有限状态与重复循环；后续是 CFOP 提速课程。本轮没有逐一查看其余 52 个提速分集。

角朝向与奇偶性进一步参照 [Jaap 的魔方数学](https://www.jaapsch.net/puzzles/theory.htm)。不将原文图像或视频截图放入网页；网页插图均由本站转动模型和 HTML/SVG 绘制。

## TODO

1. 能不能添加一个可以交互的结果
2. 添加一个魔方的示意图或者结构图是什么样子
2. 为什么我还是没看懂这个东西?

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
