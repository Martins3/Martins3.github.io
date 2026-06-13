# HDD 已死
hdd : 2T 379
ssd : 1T 598

在任何状态下都是几十倍，但是价格都很便宜。

## nvme 4k randread

```txt
🤒  sudo fio /home/martins3/core/vn/docs/kernel/code/aio/4k-read.fio
[sudo] password for martins3:
trash: (g=0): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=128
fio-3.33
Starting 1 process
Jobs: 1 (f=1): [r(1)][100.0%][r=3039MiB/s][r=778k IOPS][eta 00m:00s]
trash: (groupid=0, jobs=1): err= 0: pid=960477: Wed Feb  1 18:00:08 2023
  read: IOPS=802k, BW=3133MiB/s (3285MB/s)(30.6GiB/10001msec)
    slat (nsec): min=428, max=147498, avg=780.67, stdev=372.69
    clat (usec): min=11, max=6450, avg=158.53, stdev=29.83
     lat (usec): min=11, max=6451, avg=159.31, stdev=29.88
    clat percentiles (usec):
     |  1.00th=[  118],  5.00th=[  127], 10.00th=[  137], 20.00th=[  139],
     | 30.00th=[  151], 40.00th=[  161], 50.00th=[  165], 60.00th=[  167],
     | 70.00th=[  169], 80.00th=[  172], 90.00th=[  176], 95.00th=[  180],
     | 99.00th=[  194], 99.50th=[  198], 99.90th=[  215], 99.95th=[  223],
     | 99.99th=[  355]
   bw (  MiB/s): min= 3020, max= 3624, per=100.00%, avg=3138.85, stdev=213.14, samples=19
   iops        : min=773296, max=927744, avg=803546.95, stdev=54563.89, samples=19
  lat (usec)   : 20=0.01%, 50=0.01%, 100=0.01%, 250=99.98%, 500=0.01%
  lat (usec)   : 750=0.01%, 1000=0.01%
  lat (msec)   : 2=0.01%, 10=0.01%
  cpu          : usr=34.74%, sys=63.02%, ctx=124580, majf=0, minf=138
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.1%, 32=0.1%, >=64=100.0%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.1%
     issued rwts: total=8021669,0,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=128

Run status group 0 (all jobs):
   READ: bw=3133MiB/s (3285MB/s), 3133MiB/s-3133MiB/s (3285MB/s-3285MB/s), io=30.6GiB (32.9GB), run=10001-10001msec

Disk stats (read/write):
  nvme1n1: ios=7935963/0, merge=0/0, ticks=349055/0, in_queue=349055, util=98.94%
```

## nvme 256 read
```txt
🧀  sudo fio /home/martins3/core/vn/docs/kernel/code/aio/4k-read.fio
trash: (g=0): rw=read, bs=(R) 256KiB-256KiB, (W) 256KiB-256KiB, (T) 256KiB-256KiB, ioengine=libaio, iodepth=128
fio-3.33
Starting 1 process
Jobs: 1 (f=1): [R(1)][100.0%][r=6806MiB/s][r=27.2k IOPS][eta 00m:00s]
trash: (groupid=0, jobs=1): err= 0: pid=960644: Wed Feb  1 18:00:33 2023
  read: IOPS=27.2k, BW=6795MiB/s (7125MB/s)(66.4GiB/10005msec)
    slat (usec): min=2, max=166, avg= 3.14, stdev= 1.06
    clat (usec): min=4329, max=9334, avg=4704.77, stdev=94.96
     lat (usec): min=4334, max=9337, avg=4707.91, stdev=95.30
    clat percentiles (usec):
     |  1.00th=[ 4686],  5.00th=[ 4686], 10.00th=[ 4686], 20.00th=[ 4686],
     | 30.00th=[ 4686], 40.00th=[ 4686], 50.00th=[ 4686], 60.00th=[ 4686],
     | 70.00th=[ 4686], 80.00th=[ 4686], 90.00th=[ 4686], 95.00th=[ 4686],
     | 99.00th=[ 4752], 99.50th=[ 4752], 99.90th=[ 6325], 99.95th=[ 7111],
     | 99.99th=[ 8356]
   bw (  MiB/s): min= 6728, max= 6810, per=100.00%, avg=6795.17, stdev=23.07, samples=20
   iops        : min=26912, max=27240, avg=27180.70, stdev=92.25, samples=20
  lat (msec)   : 10=100.00%
  cpu          : usr=1.05%, sys=11.19%, ctx=271919, majf=0, minf=8202
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.1%, 32=0.1%, >=64=100.0%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.1%
     issued rwts: total=271934,0,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=128

Run status group 0 (all jobs):
   READ: bw=6795MiB/s (7125MB/s), 6795MiB/s-6795MiB/s (7125MB/s-7125MB/s), io=66.4GiB (71.3GB), run=10005-10005msec

Disk stats (read/write):
  nvme1n1: ios=269008/0, merge=0/0, ticks=1265202/0, in_queue=1265201, util=99.05%
```

## hdd 256 read

```txt
🤒  sudo fio /home/martins3/core/vn/docs/kernel/code/aio/4k-read.fio
trash: (g=0): rw=read, bs=(R) 256KiB-256KiB, (W) 256KiB-256KiB, (T) 256KiB-256KiB, ioengine=libaio, iodepth=128
fio-3.33
Starting 1 process
Jobs: 1 (f=1): [R(1)][100.0%][r=225MiB/s][r=900 IOPS][eta 00m:00s]
trash: (groupid=0, jobs=1): err= 0: pid=961821: Wed Feb  1 18:02:22 2023
  read: IOPS=854, BW=214MiB/s (224MB/s)(2167MiB/10151msec)
    slat (usec): min=2, max=392528, avg=92.12, stdev=4403.49
    clat (msec): min=114, max=587, avg=148.78, stdev=44.83
     lat (msec): min=114, max=590, avg=148.87, stdev=45.08
    clat percentiles (msec):
     |  1.00th=[  131],  5.00th=[  142], 10.00th=[  142], 20.00th=[  142],
     | 30.00th=[  142], 40.00th=[  142], 50.00th=[  142], 60.00th=[  142],
     | 70.00th=[  142], 80.00th=[  144], 90.00th=[  148], 95.00th=[  153],
     | 99.00th=[  506], 99.50th=[  542], 99.90th=[  575], 99.95th=[  584],
     | 99.99th=[  592]
   bw (  KiB/s): min= 6144, max=232960, per=100.00%, avg=218675.20, stdev=50052.91, samples=20
   iops        : min=   24, max=  910, avg=854.20, stdev=195.52, samples=20
  lat (msec)   : 250=98.34%, 500=0.57%, 750=1.10%
  cpu          : usr=0.02%, sys=0.39%, ctx=7315, majf=0, minf=8202
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.2%, 32=0.4%, >=64=99.3%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.1%
     issued rwts: total=8669,0,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=128

Run status group 0 (all jobs):
   READ: bw=214MiB/s (224MB/s), 214MiB/s-214MiB/s (224MB/s-224MB/s), io=2167MiB (2273MB), run=10151-10151msec

Disk stats (read/write):
  sda: ios=4349/0, merge=4318/0, ticks=641120/0, in_queue=641120, util=99.06%
```

## hdd 4k randread
```txt
🧀  sudo fio /home/martins3/core/vn/docs/kernel/code/aio/4k-read.fio
trash: (g=0): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=128
fio-3.33
Starting 1 process
Jobs: 1 (f=1): [r(1)][100.0%][r=7584KiB/s][r=1896 IOPS][eta 00m:00s]
trash: (groupid=0, jobs=1): err= 0: pid=962514: Wed Feb  1 18:03:41 2023
  read: IOPS=1895, BW=7582KiB/s (7764kB/s)(74.6MiB/10076msec)
    slat (nsec): min=1224, max=37451k, avg=523051.98, stdev=1486974.92
    clat (msec): min=42, max=2079, avg=66.88, stdev=28.96
     lat (msec): min=42, max=2083, avg=67.40, stdev=29.01
    clat percentiles (msec):
     |  1.00th=[   48],  5.00th=[   52], 10.00th=[   53], 20.00th=[   57],
     | 30.00th=[   60], 40.00th=[   62], 50.00th=[   65], 60.00th=[   68],
     | 70.00th=[   71], 80.00th=[   74], 90.00th=[   81], 95.00th=[   88],
     | 99.00th=[  116], 99.50th=[  128], 99.90th=[  176], 99.95th=[  203],
     | 99.99th=[ 2056]
   bw (  KiB/s): min= 6848, max= 7936, per=100.00%, avg=7588.80, stdev=308.70, samples=20
   iops        : min= 1712, max= 1984, avg=1897.20, stdev=77.17, samples=20
  lat (msec)   : 50=1.97%, 100=95.75%, 250=2.24%, 500=0.01%, 750=0.01%
  lat (msec)   : 1000=0.01%, 2000=0.01%, >=2000=0.01%
  cpu          : usr=0.08%, sys=0.26%, ctx=21525, majf=0, minf=138
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.1%, 32=0.2%, >=64=99.7%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.1%
     issued rwts: total=19099,0,0,0 short=0,0,0,0 dropped=0,0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=128

Run status group 0 (all jobs):
   READ: bw=7582KiB/s (7764kB/s), 7582KiB/s-7582KiB/s (7764kB/s-7764kB/s), io=74.6MiB (78.2MB), run=10076-10076msec

Disk stats (read/write):
  sda: ios=18812/0, merge=0/0, ticks=594244/0, in_queue=594244, util=98.86%
```
