# 原始素材

以下 PNG 均逐字节取自原项目提交 `29ccda67802e`，未重绘、未重新生成、未修改文件内容：

| 本地文件 | 原仓库路径（相对于 `resources/assets/models/standard/resources/`） |
| --- | --- |
| `original-cover.png` | `cover.png` |
| `original-background.png` | `background.png` |
| `original-key-a.png` | `left-keys/KeyA.png` |

原始素材的 MIT 授权声明保留在 `LICENSE-MIT`。
之前生成的替代底图和手绘猫爪已移除，不再使用 imagegen 生成图片。

默认待机画面直接使用原封面。按键时沿用原项目运行时的抬爪遮罩区域，
叠加原始 KeyA 猫爪图，并缓存结果。鼠标移动时，在内存中提取原封面中的
鼠标爪区域，以原背景填补活动区域，然后对原像素进行局部插值变形；
肩部保持固定。鼠标点击添加轻微下压位移。
这些是运行时动画处理，不会回写或修改 PNG 文件。

`cargo test idle_is_pixel_identical_to_original_cover` 验证 612×354 下
默认待机画面与原始封面解码结果逐像素一致；显示缩放只在加载时进行。
`cargo test render_previews -- --ignored` 将应用渲染的四种预览写入
`target/previews/`，便于检查拍爪、鼠标跟随及透明边缘。
