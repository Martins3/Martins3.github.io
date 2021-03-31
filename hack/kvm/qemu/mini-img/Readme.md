# 使用最小的 image 从而用于调试 Qemu 和 内核

- [ ] create_net.sh 的作用是什么 ?

似乎串口无法使用:
1. https://fadeevab.com/how-to-setup-qemu-output-to-console-and-automate-using-shell-script/
2. https://dev.to/alexeyden/quick-qemu-setup-for-linux-kernel-module-debugging-2nde

- [x] 使用 raw.sh 无法数据无法保存到 image 中间
  - 看来是损坏了这个 image 了, 还是小心手动看看里面在搞什么

## Notes
[官方文档应该很清晰的告诉了这是怎么回事](https://docs.yoctoproject.org/index.html)

## Licence
Copyright belongs to https://github.com/linux-kernel-labs
