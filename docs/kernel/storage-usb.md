## 4k


## 4k 随机读 3.5M
```txt
🤒  sudo fio /home/martins3/core/vn/docs/kernel/code/aio/4k-read.fio
trash: (g=0): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=128
fio-3.33
Starting 1 process
^Cbs: 1 (f=1): [r(1)][70.0%][r=3480KiB/s][r=870 IOPS][eta 00m:03s]
fio: terminating on signal 2

trash: (groupid=0, jobs=1): err= 0: pid=1047476: Sat Jan  7 17:35:36 2023
  read: IOPS=869, BW=3477KiB/s (3560kB/s)(26.6MiB/7834msec)
    slat (usec): min=2, max=8856, avg=1149.11, stdev=572.75
    clat (msec): min=3, max=167, avg=144.74, stdev=12.06
     lat (msec): min=4, max=168, avg=145.89, stdev=12.06
    clat percentiles (msec):
     |  1.00th=[   91],  5.00th=[  138], 10.00th=[  140], 20.00th=[  142],
     | 30.00th=[  142], 40.00th=[  144], 50.00th=[  146], 60.00th=[  146],
     | 70.00th=[  148], 80.00th=[  150], 90.00th=[  155], 95.00th=[  157],
     | 99.00th=[  161], 99.50th=[  165], 99.90th=[  167], 99.95th=[  167],
     | 99.99th=[  167]
   bw (  KiB/s): min= 2472, max= 3608, per=98.08%, avg=3410.67, stdev=269.63, samples=15
   iops        : min=  618, max=  902, avg=852.67, stdev=67.41, samples=15
  lat (msec)   : 4=0.01%, 10=0.07%, 20=0.12%, 50=0.35%, 100=0.57%
  lat (msec)   : 250=98.87%
  cpu          : usr=0.04%, sys=0.28%, ctx=13228, majf=0, minf=138
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.2%, 32=0.5%, >=64=99.1%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.1%
     issued rwts: total=6809,0,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=128

Run status group 0 (all jobs):
   READ: bw=3477KiB/s (3560kB/s), 3477KiB/s-3477KiB/s (3560kB/s-3560kB/s), io=26.6MiB (27.9MB), run=7834-7834msec
```

## 256K 30M
```txt
Disk stats (read/write):
  sdb: ios=6653/1, merge=0/0, ticks=15273/1, in_queue=15274, util=98.73%
vn on  master [!+] took 8s
🤒  sudo fio /home/martins3/core/vn/docs/kernel/code/aio/4k-read.fio
trash: (g=0): rw=randread, bs=(R) 256KiB-256KiB, (W) 256KiB-256KiB, (T) 256KiB-256KiB, ioengine=libaio, iodepth=128
fio-3.33
Starting 1 process
^Cbs: 1 (f=1): [r(1)][60.0%][r=27.2MiB/s][r=109 IOPS][eta 00m:04s]
fio: terminating on signal 2

trash: (groupid=0, jobs=1): err= 0: pid=1047689: Sat Jan  7 17:35:50 2023
  read: IOPS=116, BW=29.1MiB/s (30.5MB/s)(178MiB/6107msec)
    slat (usec): min=3710, max=38756, avg=8589.63, stdev=3785.08
    clat (msec): min=5, max=1188, avg=997.41, stdev=244.21
     lat (msec): min=16, max=1197, avg=1006.00, stdev=244.09
    clat percentiles (msec):
     |  1.00th=[   64],  5.00th=[  326], 10.00th=[  634], 20.00th=[ 1028],
     | 30.00th=[ 1045], 40.00th=[ 1053], 50.00th=[ 1070], 60.00th=[ 1083],
     | 70.00th=[ 1116], 80.00th=[ 1133], 90.00th=[ 1150], 95.00th=[ 1167],
     | 99.00th=[ 1183], 99.50th=[ 1183], 99.90th=[ 1183], 99.95th=[ 1183],
     | 99.99th=[ 1183]
   bw (  KiB/s): min=25088, max=31744, per=98.40%, avg=29286.40, stdev=1972.66, samples=10
   iops        : min=   98, max=  124, avg=114.40, stdev= 7.71, samples=10
  lat (msec)   : 10=0.14%, 20=0.14%, 50=0.56%, 100=0.70%, 250=2.25%
  lat (msec)   : 500=3.66%, 750=4.37%, 1000=4.23%, 2000=83.94%
  cpu          : usr=0.02%, sys=0.23%, ctx=3262, majf=0, minf=8202
  IO depths    : 1=0.1%, 2=0.3%, 4=0.6%, 8=1.1%, 16=2.3%, 32=4.5%, >=64=91.1%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=99.8%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.2%
     issued rwts: total=710,0,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=128

Run status group 0 (all jobs):
   READ: bw=29.1MiB/s (30.5MB/s), 29.1MiB/s-29.1MiB/s (30.5MB/s-30.5MB/s), io=178MiB (186MB), run=6107-6107msec

Disk stats (read/write):
  sdb: ios=2059/1, merge=0/0, ticks=11758/0, in_queue=11759, util=98.38%
```
