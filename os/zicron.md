# env setup
https://fuchsia.dev/fuchsia-src/get-started/get_fuchsia_source

1. cd ~
2. curl -s "https://fuchsia.googlesource.com/fuchsia/+/HEAD/scripts/bootstrap?format=TEXT" | base64 --decode | bash
  - maybe take hours

3. add this to .zshrc
```c
export PATH=~/fuchsia/.jiri_root/bin:$PATH
source ~/fuchsia/scripts/fx-env.sh
```
4. fx set core.qemu-x64
5. fx compdb


# [kernel docs reading notes](https://fuchsia.dev/fuchsia-src/concepts/kernel)

- [ ] fbl
- [ ] https://github.com/littlekernel/lk : arose my interest, oh my god, it's huge project
