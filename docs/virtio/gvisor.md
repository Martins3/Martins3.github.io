# gvisor
https://www.infoq.com/presentations/gvisor-os-go/

## How to setup environment
ref :  https://stackoverflow.com/questions/55411277/how-can-i-setup-vscode-for-go-project-built-with-bazel

1. genereate gopath style source code
```plain
bazel build :gopath
```
2. for clarity and simplisity, copy code somewhere else
```c
cp -Lr bazel-bin/gopath ~/test/gv
```
3. init go mod in `~/test/gv/gopath/gvisor.dev`
```plain
go mod init gvisor.dev
```

4. setup env:
  - for vscode:
```plain
➜  gvisor git:(master) ✗ cat .vscode/settings.json
{
    "go.toolsEnvVars": {
         "GOPATH": "~/test/gv/gopath"
    }
}%
```
  - for vim:
```plain
➜  gvisor git:(master) ✗ export GOPATH=~/test/gv/gopath
```
5. open directory `~/test/gv/gopath/gvisor.dev` as root directory with vim or vscode


## code overview

| dir          | files | blank | comment | code   |
|--------------|-------|-------|---------|--------|
| abi          | 65    | 998   | 2481    | 4255   |
| amutex       | 2     | 29    | 65      | 117    |
| atomicbitops | 3     | 35    | 55      | 260    |
| binary       | 2     | 63    | 59      | 410    |
| bits         | 5     | 31    | 91      | 171    |
| bpf          | 8     | 138   | 269     | 1776   |
| buffer       | 8     | 131   | 275     | 1070   |
| cleanup      | 2     | 14    | 44      | 68     |
| compressio   | 2     | 125   | 238     | 700    |
| context      | 1     | 29    | 87      | 76     |
| control      | 2     | 28    | 67      | 98     |
| coverage     | 1     | 25    | 82      | 129    |
| cpuid        | 6     | 254   | 489     | 1331   |
| crypto       | 2     | 5     | 30      | 13     |
| eventchannel | 4     | 56    | 116     | 252    |
| fd           | 2     | 43    | 95      | 254    |
| fdchannel    | 2     | 31    | 64      | 180    |
| fdnotifier   | 2     | 47    | 69      | 168    |
| flipcall     | 10    | 150   | 366     | 981    |
| fspath       | 4     | 37    | 140     | 314    |
| gate         | 2     | 48    | 101     | 177    |
| gohacks      | 1     | 5     | 31      | 21     |
| goid         | 2     | 16    | 52      | 85     |
| ilist        | 2     | 72    | 88      | 307    |
| iovec        | 2     | 24    | 40      | 132    |
| linewriter   | 2     | 23    | 41      | 96     |
| log          | 6     | 99    | 208     | 448    |
| marshal      | 3     | 83    | 278     | 250    |
| memutil      | 1     | 5     | 16      | 21     |
| merkletree   | 2     | 73    | 209     | 1343   |
| metric       | 2     | 81    | 102     | 325    |
| p9           | 20    | 1698  | 2664    | 8274   |
| pool         | 2     | 21    | 39      | 71     |
| procid       | 3     | 21    | 56      | 51     |
| rand         | 2     | 16    | 41      | 49     |
| refs         | 3     | 101   | 242     | 395    |
| refsvfs2     | 3     | 39    | 123     | 158    |
| safecopy     | 3     | 109   | 238     | 787    |
| safemem      | 6     | 117   | 322     | 984    |
| seccomp      | 9     | 140   | 390     | 1542   |
| secio        | 3     | 28    | 76      | 161    |
| segment      | 5     | 189   | 629     | 1964   |
| sentry       | 762   | 22131 | 47054   | 118021 |
| shim         | 28    | 484   | 692     | 3928   |
| sleep        | 2     | 113   | 304     | 558    |
| state        | 31    | 650   | 1559    | 3858   |
| sync         | 23    | 299   | 625     | 1833   |
| syncevent    | 8     | 175   | 408     | 928    |
| syserr       | 3     | 38    | 88      | 329    |
| syserror     | 2     | 33    | 80      | 247    |
| tcpip        | 214   | 13862 | 21419   | 78055  |
| test         | 9     | 276   | 455     | 1578   |
| unet         | 3     | 231   | 271     | 1094   |
| urpc         | 2     | 110   | 210     | 526    |
| usermem      | 10    | 181   | 563     | 1228   |
| waiter       | 2     | 60    | 158     | 227    |

# todo
As the code indicates, the most complicated part are:

- sentry : user mode linux kernel
- p9 : something I fear
- tcpip : user mode network stack
