# 基本分析
- https://en.wikipedia.org/wiki/Standard_RAID_levels

## raid0

超级简单，但是除了一个稍微复杂点的 `raid0_takeover`

## raid10

准备四个 disk，首先两两组建为 raid1，然后将两个 raid1 组件为 raid0

[is-there-a-difference-between-raid-10-10-and-raid-01-01](https://serverfault.com/questions/145319/is-there-a-difference-between-raid-10-10-and-raid-01-01)

之所以不推荐 raid 0 + 1 ，是因为
1. 加入 4 个盘
2. 如果单盘坏掉，都无所谓。
3. 如果 3 个或者 4 坏掉，都挂掉。
4. 如果只是坏掉两个，一共六种情况，raid10 在两种情况中会坏掉，而 raid01 是 4 种。

当然，这里的前提是最多 raid0 认为其下的盘坏一个，整个 raid0 就报废了。
