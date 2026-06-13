A notification chain is simply a list of functions to execute when a given event occurs.

 The owner simply defines the list; any kernel subsystem can
register a callback function with that chain to receive the notification.

- notifier_chain_register
- notifier_call_chain

inetaddr_chain : Sends notifications about the insertion, removal, and change of an Internet Protocol Version 4 (IPv4) address on a local interface.
netdev_chain : Sends notifications about the registration status of network devices.

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
