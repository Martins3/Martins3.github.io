
对于 terminal 的开发
https://terminal-code.com/

https://github.com/reubeno/brush
https://github.com/tio/tio
https://github.com/ogulcancelik/herdr
https://github.com/l0ng-ai/tty7

## wsl 的 magic
```txt
systemd─┬─NetworkManager───3*[{NetworkManager}]
        ├─2*[agetty]
        ├─dbus-broker-lau───dbus-broker
        ├─init-systemd(Fe─┬─SessionLeader───Relay(233)───bash───bash───sudo───sudo───yum───{yum}
        │                 ├─SessionLeader───Relay(340)───bash───pstree
        │                 ├─init───{init}
        │                 ├─login───bash
        │                 └─{init-systemd(Fe}
        ├─2*[systemd───(sd-pam)]
        ├─systemd-homed
        ├─systemd-hostnam
        ├─systemd-journal
        ├─systemd-logind
        ├─systemd-oomd
        ├─systemd-resolve
        ├─systemd-udevd
        └─systemd-userdbd───3*[systemd-userwor]
```
Relay 是什么东西?

https://wsl.dev/technical-documentation/relay/

https://github.com/microsoft/WSL

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
