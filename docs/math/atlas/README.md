# 本科数学全景图

独立编排的本科数学知识网站。直接打开 [index.html](./index.html)，或由仓库现有 GitHub Pages 发布到 `/math/atlas/`。没有外部运行时、CDN、登录、数据写入或本地 PDF 依赖。

## 内容范围

目前包含 8 个领域、26 门课程、53 个章节、295 个具体知识点，另有 32 个领域导览词条、157 条带解释的数学联系与 6 条主题路线。具体知识点与宽泛导览分别计数，页面主计数不把导览词条计入知识点。

课程覆盖集合逻辑、数学分析（一元、多元、级数）、高等代数、解析几何、抽象代数、数论、复变、实变、泛函、拓扑、微分几何与流形、常微分方程、偏微分方程、概率、统计、随机过程、数值分析、最优化、组合、图论、群表示、代数几何、信息编码和数学建模。

范围对照 [北大数学与应用数学培养方案](https://math.pku.edu.cn/puremath/bkspy/pyfa/index.htm)、[剑桥本科前三年课程目录](https://www.maths.cam.ac.uk/undergrad/node/40)和 [MIT 数学本科要求](https://catalog.mit.edu/degree-charts/mathematics-course-18/)组织，概念文字独立编写。课程级别为跨学校的导航分类，不代表某个学校的固定必修要求。覆盖常见本科核心与主要选修，不声称囊括所有学校每门专题课的全部定理。

内容不以 `docs/math/` 下的笔记或 PDF 为选题、范围、结构或阅读前提。增加课程时应依据数学知识本身及课程体系，不根据本地文件存在与否决定是否收录。

## 浏览

- 首页按领域、课程、章节直接铺开全部具体知识点；领域按钮可缩小阅读范围。
- 悬停或键盘聚焦查看定义；点击或 Enter 阅读定义、直觉与条件、公式、例子。
- 桌面详情栏随页面保持可见；手机使用可关闭的原生详情弹层，不必跳到长目录末尾。
- 详情中的“数学联系”解释概念为什么相连，可以跨课程跳转；隐藏目标所属领域时会自动恢复全部领域。
- “同章索引”仅表示课程归属，不将同章或相邻排列伪装为先修关系。
- “领域之间如何联系”展开原有关系图；其 32 个词条还可跳到完整课程。
- URL 中 `#jordan`、`#residue` 等标识可直接打开具体概念；旧链接如 `#linear` 保持有效。前进、后退恢复选择。Escape 关闭预览或手机弹层；桌面没有预览时回到全景。
- 不需要输入框；使用浏览器查找也可在默认展开的课程中定位词条。

## 文件组织

- `data.js`：领域、32 个导览词条及概览关系。
- `curriculum.js`：完整课程、章节与具体知识点，跨课程联系、覆盖参照和导览到课程的映射。
- `app.js`：课程树、详情、关系图、导航与手机弹层。
- `style.css`：布局和视觉样式。
- `index.html`：入口。

数据由静态服务器直接提供，普通脚本依次加载，因此 `file://` 本地打开同样可用。将来可由后台生成相同的数据文件，不必引入用户输入、数据库或客户端持久化。

## 扩展内容

在 `curriculum.js` 调用 `course(id, title, domain, level, description, chapters)` 定义课程。领域 id 取自 `data.js`；课程和概念 id 必须唯一且保持稳定，以免破坏已有链接。

每个章节为 `[章节名称, 条目数组]`；每条概念记录为：

```js
["稳定-id", "概念名称", "定义", "直觉或适用条件", "公式", "具体例子"]
```

每个具体概念恰有一个主要课程与章节归属。交叉归属通过数学联系表达，避免为同一知识点重复建立 id。公式中的假设应写在定义、条件或公式旁；示例需实际说明该概念，不能用统一模板填充。

`links` 中关系格式为 `[sourceId, targetId, type, explanation]`：

- `prerequisite`：有方向，source 是理解 target 的一种前置。
- `bridge`：无方向，解释共同思想、工具、推导或应用。

只有确实能说明机制的关系才添加边。不自动把章节内相邻词条连成依赖链。`data.js` 的 `featured` 标记控制概览关系图默认展示哪些跨领域关系；新知识点不会被挤入旧图的固定布局，而是在完整课程树中展示。

`overviewCourses` 把领域导览词条映射到具体课程，`journeys` 提供有真实关系支撑的主题阅读路线。

## 验证

在仓库根目录执行 `node docs/math/atlas/tests/verify.mjs`，检查字段、唯一标识、课程与章节的完整归属、关系端点、重复边、导览映射及路线连接。

执行 `node docs/math/atlas/tests/browser.mjs` 检查默认完整展示、悬停与固定详情、领域筛选、跨课程跳转、历史导航、键盘、每门课程点击、全部 295 个详情、手机弹层、视口切换及本地文件访问。需环境中已有 Playwright；可通过 `PLAYWRIGHT_MODULE` 指定其入口，通过 `CHROMIUM_PATH` 指定浏览器。截图输出到临时目录，页面本身无这些依赖。

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
