# Replace my mi book with 3A5000

<!-- vim-markdown-toc GitLab -->

- [Background](#background)
- [Neovim](#neovim)
- [Terminal](#terminal)
- [Browser](#browser)
- [科学上网](#科学上网)
- [No more works in 3A5000](#no-more-works-in-3a5000)

<!-- vim-markdown-toc -->
![](../img/3a5000.png)
(Tested on 2021.12.29)

Because I don't wanna to spend too much time to tune the fcixt, as you see, I will give up Chinese temporarily.
## Background
Several months ago, when I'm still working the [loongson-dune](https://github.com/Martins3/loongson-dune), I tried to
sync my [My Linux Config][https://github.com/Martins3/My-Linux-Config] to Longson 3a5000.

As for zsh, it's not too hard. Although it define many macro related architecture, while fixing them is fairly easy.
zsh complaints that it doesn't recognize the loongarch64 arch, I comment the related code, then it compiles and works fine.

But neovim is not so easy because it depends on luajit which has a huge portion of [architecture related code](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/vm_mips64.dasc)
I'm not a compiler expert, mips expert and loongarch expert, it will cost me too much time, weeks or months, to make it work.

But last day I found the luajit has been ported by loonson engineers, it's time to give it a try.

## Neovim
```sh
sudo apt install libluajit-5.1-dev
sudo apt install lua5.1
sudo apt install luarocks
git clone https://github.com/neovim/neovim
cd neovim
```

Neovim separate it's compile into three stage[^1]:
- compiles bundles. Bundles are the dependencies of neovim, you can compile it from source or install it with package manager
- compile the neovim.
- link the neovim with the bundles

```sh
make BUNDLED_CMAKE_FLAG="-DUSE_BUNDLED=ON -DUSE_BUNDLED_LUAJIT=OFF -DUSE_BUNDLED_LUAROCKS=OFF"
```
This command will compile all the bundles from source except luajit and luarocks.

The reason to exempt luajit is easy, but leaving luarocks out is kind of mysterious. I don't know why, it just works.

## Terminal
[alacritty](https://github.com/alacritty/alacritty) can't be compiled correctly because of outdated rust toolchain.
So I use the system default gnome-terminal, it works perfectly.

## Browser
Based on [chromium](https://www.chromium.org/), no crash, nothing new except the logo.

But there is one thing makes me sick. Loongson removed Google's original profile with it's own, so I can't sync my bookmarks from my x86 Chrome's bookmarks and passwords.
## 科学上网
In order to access Google, v2ray and qv2ray are necessary.
I don't try to port them. use another x86 to setup the network proxy and share it to 3A5000.

## No more works in 3A5000
| What                                                | Why                                                                                   |
|-----------------------------------------------------|---------------------------------------------------------------------------------------|
| [lazygit](https://github.com/jesseduffield/lazygit) | Outdated golang toolchain                                                             |
| All Chrome plugin                                   | I don't know why                                                                      |
| pynvim                                              | [greenlet](https://github.com/python-greenlet/greenlet) has architecture related code |
| wakatime                                            | wakatime only deploy amd64 client                                                     |

[^1]: https://github.com/neovim/neovim/wiki/Building-Neovim#how-to-build-without-bundled-dependencies
[^2]: https://martins3.github.io/gfw.html#share-proxy-cross-lan
