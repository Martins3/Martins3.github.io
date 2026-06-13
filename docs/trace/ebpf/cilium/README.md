# 做做这里的教程
https://ebpf-go.dev/guides/getting-started/#the-go-application

所以，这个只是 go 的 bind 吗? 和 libbpf 啥关系 ?
```sh
go mod init ebpf-test
go mod tidy
go get github.com/cilium/ebpf/cmd/bpf2go
```

之后的操作:
```sh
go generate && go build && sudo ./ebpf-test
```

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
