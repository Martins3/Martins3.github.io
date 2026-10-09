===== C++17 default: optional NRVO enabled =====      ===== C++17 -fno-elide-constructors =====
./rvo-nrvo.out                                        ./rvo-nrvo-no-elide.out

=== 1. C++17 guaranteed RVO: return A(10) ===         === 1. C++17 guaranteed RVO: return A(10) ===
constructor: value=10                                 constructor: value=10
result: value=10                                      result: value=10
destructor: value=10                                  destructor: value=10

=== 2. optional NRVO: return named result ===         === 2. optional NRVO: return named result ===
constructor: value=20                                 constructor: value=20
result: value=20                                      move constructor: take value=20
destructor: value=20                                  destructor: value=-1
                                                      result: value=20
=== 3. explicit std::move construction ===            destructor: value=20
constructor: value=30
move constructor: take value=30                       === 3. explicit std::move construction ===
destructor: value=-1                                  constructor: value=30
result: value=30                                      move constructor: take value=30
destructor: value=30                                  move constructor: take value=30
                                                      destructor: value=-1
=== 4. copy construction for comparison ===           destructor: value=-1
constructor: value=40                                 result: value=30
copy constructor: value=40                            destructor: value=30
result: value=40
source: value=40                                      === 4. copy construction for comparison ===
destructor: value=40                                  constructor: value=40
destructor: value=40                                  copy constructor: value=40
                                                      result: value=40
=== 5. move assignment into an existing object ===    source: value=40
constructor: value=50                                 destructor: value=40
constructor: value=20                                 destructor: value=40
move assignment: take value=20
destructor: value=-1                                  === 5. move assignment into an existing object ===
target: value=20                                      constructor: value=50
destructor: value=20                                  constructor: value=20
                                                      move constructor: take value=20
                                                      destructor: value=-1
                                                      move assignment: take value=20
                                                      destructor: value=-1
                                                      target: value=20
                                                      destructor: value=20


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
