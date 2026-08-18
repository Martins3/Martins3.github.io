## 切换了 wayland 测试
- https://nixos.wiki/wiki/Wayland
- https://drakerossman.com/blog/wayland-on-nixos-confusion-conquest-triumph

不太可以，gnome.nix 的程序需要重新配置，太复杂了。

## Fedora gnome 默认是 wayland

应该的测试方法是，将 GPU 直通给他，然后测试。

- [Learn Wayland by writing a GUI from scratch](https://news.ycombinator.com/item?id=37876896)


## X windows
- [How X Window Managers Work, And How To Write One (Part I)](https://jichu4n.com/posts/how-x-window-managers-work-and-how-to-write-one-part-i/)

- [Iconography of the X Window System: The Boot Stipple](https://news.ycombinator.com/item?id=40936808)

## 看看这个
https://blog.vaxry.net/articles/2024-wlrootsRewrite

所以 wlroots 和 HyprLand 是什么关系?

为什么 HyprLand 现在有 2w star 了啊

## 一个小的 x server 的实现
https://git.dec05eba.com/phoenix/about/


A scrollable-tiling Wayland compositor.
https://github.com/YaLTeR/niri
	- https://github.com/Vigintillionn/niri-sidebar


## 判断是不是 wayland 的方法
$XDG_SESSION_TYPE

当然，这个是需要在图形环境中执行才可以:
如果是 ssh 进去的

```txt
🧀  echo $XDG_SESSION_TYPE                

tty

```
