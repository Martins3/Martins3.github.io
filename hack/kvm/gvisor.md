# gvisor
https://www.infoq.com/presentations/gvisor-os-go/

## How to setup environment
- https://stackoverflow.com/questions/55411277/how-can-i-setup-vscode-for-go-project-built-with-bazel
- open directory bazel-bin/gopath as root directory


export GOPATH=~/test/gv/gopath
/home/maritns3/test/gv/gopath/gvisor.dev
go mod init gvisor.dev

## code overview

./abi
Go                              65            998           2481           4255
./amutex
Go                               2             29             65            117
./atomicbitops
Go                               3             35             55            260
./binary
Go                               2             63             59            410
./bits
Go                               5             31             91            171
./bpf
Go                               8            138            269           1776
./buffer
Go                               8            131            275           1070
./cleanup
Go                               2             14             44             68
./compressio
Go                               2            125            238            700
./context
Go                               1             29             87             76
./control
Go                               2             28             67             98
./coverage
Go                               1             25             82            129
./cpuid
Go                               6            254            489           1331
./crypto
Go                               2              5             30             13
./eventchannel
Go                               4             56            116            252
./fd
Go                               2             43             95            254
./fdchannel
Go                               2             31             64            180
./fdnotifier
       3 text files.
       3 unique files.
       1 file ignored.

github.com/AlDanial/cloc v 1.82  T=0.00 s (404.6 files/s, 57456.2 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
Go                               2             47             69            168
-------------------------------------------------------------------------------
SUM:                             2             47             69            168
-------------------------------------------------------------------------------
./flipcall
      11 text files.
      11 unique files.
       1 file ignored.

github.com/AlDanial/cloc v 1.82  T=0.01 s (1133.1 files/s, 169630.5 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
Go                              10            150            366            981
-------------------------------------------------------------------------------
SUM:                            10            150            366            981
-------------------------------------------------------------------------------
./fspath
       5 text files.
       5 unique files.
       1 file ignored.

github.com/AlDanial/cloc v 1.82  T=0.01 s (669.0 files/s, 82119.9 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
Go                               4             37            140            314
-------------------------------------------------------------------------------
SUM:                             4             37            140            314
-------------------------------------------------------------------------------
./gate
       3 text files.
       3 unique files.
       1 file ignored.

github.com/AlDanial/cloc v 1.82  T=0.00 s (402.9 files/s, 65674.5 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
Go                               2             48            101            177
-------------------------------------------------------------------------------
SUM:                             2             48            101            177
-------------------------------------------------------------------------------
./gohacks
       2 text files.
       2 unique files.
       1 file ignored.

github.com/AlDanial/cloc v 1.82  T=0.00 s (235.4 files/s, 13420.6 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
Go                               1              5             31             21
-------------------------------------------------------------------------------
./goid
       5 text files.
       5 unique files.
       1 file ignored.

github.com/AlDanial/cloc v 1.82  T=0.01 s (759.9 files/s, 37043.8 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
Go                               2             16             52             85
Assembly                         2              4             30              8
-------------------------------------------------------------------------------
SUM:                             4             20             82             93
-------------------------------------------------------------------------------
./ilist
       3 text files.
       3 unique files.
       1 file ignored.

github.com/AlDanial/cloc v 1.82  T=0.01 s (375.5 files/s, 87682.5 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
Go                               2             72             88            307
-------------------------------------------------------------------------------
SUM:                             2             72             88            307
-------------------------------------------------------------------------------
./iovec
Go                               2             24             40            132
./linewriter
Go                               2             23             41             96
./log
Go                               6             99            208            448
./marshal
Go                               3             83            278            250
./memutil
Go                               1              5             16             21
./merkletree
Go                               2             73            209           1343
./metric
Go                               2             81            102            325
./p9
Go                              20           1698           2664           8274
./pool
Go                               2             21             39             71
./procid
Go                               3             21             56             51
./rand
Go                               2             16             41             49
./refs
Go                               3            101            242            395
./refsvfs2
Go                               3             39            123            158
./safecopy
Go                               3            109            238            787
./safemem
Go                               6            117            322            984
./seccomp
Go                               9            140            390           1542
./secio
Go                               3             28             76            161
./segment
Go                               5            189            629           1964
./sentry
Go                             762          22131          47054         118021
./shim
Go                              28            484            692           3928
./sleep
Go                               2            113            304            558
./state
Go                              31            650           1559           3858
./sync
Go                              23            299            625           1833
./syncevent
Go                               8            175            408            928
./syserr
Go                               3             38             88            329
./syserror
Go                               2             33             80            247
./tcpip
Go                             214          13862          21419          78055
./test
Go                               9            276            455           1578
./unet
Go                               3            231            271           1094
./urpc
Go                               2            110            210            526
./usermem
Go                              10            181            563           1228
./waiter
Go                               2             60            158            227

- sentry
- p9 : 
- tcpip
