## putty
- https://www.chiark.greenend.org.uk/~sgtatham/putty/

文档:
https://the.earth.li/~sgtatham/putty/0.83/htmldoc/

太复杂了
https://git.tartarus.org/simon/putty.git

putty 自己实现了 ssh 协议，很 nb

1. 为什么他还可以连 serial 都可以连

deepseek 生成

SSH 协议（Secure Shell Protocol）主要由以下几个 RFC 文档定义：

1. **RFC 4251** - The Secure Shell (SSH) Protocol Architecture
   - 该文档描述了 SSH 协议的整体架构，包括其组成部分和基本工作原理。

2. **RFC 4252** - The Secure Shell (SSH) Authentication Protocol
   - 该文档定义了 SSH 协议中的认证机制，包括用户认证和服务器认证。

3. **RFC 4253** - The Secure Shell (SSH) Transport Layer Protocol
   - 该文档描述了 SSH 协议的传输层，包括密钥交换、加密和完整性保护等。

4. **RFC 4254** - The Secure Shell (SSH) Connection Protocol
   - 该文档定义了 SSH 协议的连接层，包括多路复用、通道管理和端口转发等。

5. **RFC 4256** - Generic Message Exchange Authentication for the Secure Shell Protocol (SSH)
   - 该文档描述了 SSH 协议中的通用消息交换认证机制。

6. **RFC 4335** - The Secure Shell (SSH) Session Channel Break Extension
   - 该文档定义了 SSH 会话通道的中断扩展。

7. **RFC 4344** - The Secure Shell (SSH) Transport Layer Encryption Modes
   - 该文档描述了 SSH 传输层加密模式。

8. **RFC 4419** - Diffie-Hellman Group Exchange for the Secure Shell (SSH) Transport Layer Protocol
   - 该文档定义了 SSH 传输层协议中的 Diffie-Hellman 组交换。

9. **RFC 4432** - RSA Key Exchange for the Secure Shell (SSH) Transport Layer Protocol
   - 该文档描述了 SSH 传输层协议中的 RSA 密钥交换。

10. **RFC 4462** - Generic Security Service Application Program Interface (GSS-API) Authentication and Key Exchange for the Secure Shell (SSH) Protocol
    - 该文档定义了 SSH 协议中的 GSS-API 认证和密钥交换机制。

11. **RFC 4716** - The Secure Shell (SSH) Public Key File Format
    - 该文档描述了 SSH 公钥文件的格式。

12. **RFC 4819** - Secure Shell Public Key Subsystem
    - 该文档定义了 SSH 公钥子系统。

13. **RFC 5647** - AES Galois Counter Mode for the Secure Shell Transport Layer Protocol
    - 该文档描述了 SSH 传输层协议中的 AES Galois 计数器模式。

14. **RFC 5656** - Elliptic Curve Algorithm Integration in the Secure Shell Transport Layer
    - 该文档定义了 SSH 传输层中的椭圆曲线算法集成。

15. **RFC 6187** - X.509v3 Certificates for Secure Shell Authentication
    - 该文档描述了用于 SSH 认证的 X.509v3 证书。

16. **RFC 6239** - Suite B Cryptographic Suites for Secure Shell (SSH)
    - 该文档定义了 SSH 协议中的 Suite B 加密套件。

17. **RFC 6594** - Use of the SHA-256 Algorithm with RSA, Digital Signature Algorithm (DSA), and Elliptic Curve DSA (ECDSA) in SSHFP Resource Records
    - 该文档描述了在 SSHFP 资源记录中使用 SHA-256 算法与 RSA、DSA 和 ECDSA 的结合。

18. **RFC 6668** - SHA-2 Data Integrity Verification for the Secure Shell (SSH) Transport Layer Protocol
    - 该文档定义了 SSH 传输层协议中的 SHA-2 数据完整性验证。

19. **RFC 7479** - Ed25519 SSHFP Resource Records
    - 该文档描述了 Ed25519 SSHFP 资源记录。

20. **RFC 8308** - Extension Negotiation in the Secure Shell (SSH) Protocol
    - 该文档定义了 SSH 协议中的扩展协商机制。

21. **RFC 8332** - Use of RSA Keys with SHA-256 and SHA-512 in the Secure Shell (SSH) Protocol
    - 该文档描述了在 SSH 协议中使用 RSA 密钥与 SHA-256 和 SHA-512 的结合。

22. **RFC 8709** - Ed25519 and Ed448 Public Key Algorithms for the Secure Shell (SSH) Protocol
    - 该文档定义了 SSH 协议中的 Ed25519 和 Ed448 公钥算法。

23. **RFC 8731** - Secure Shell (SSH) Key Exchange Method Using Curve25519 and Curve448
    - 该文档描述了使用 Curve25519 和 Curve448 的 SSH 密钥交换方法。

24. **RFC 8732** - Generic Security Service Application Program Interface (GSS-API) Key Exchange with SHA-2
    - 该文档定义了使用 SHA-2 的 GSS-API 密钥交换。

25. **RFC 8768** - Secure Shell (SSH) Key Exchange Method Using Kyber
    - 该文档描述了使用 Kyber 的 SSH 密钥交换方法。

这些 RFC 文档共同定义了 SSH 协议的各个方面，包括架构、认证、传输层、连接层、加密算法、密钥交换机制等。

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
