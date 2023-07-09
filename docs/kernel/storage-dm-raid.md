## dm raid

```sh
sudo mdadm --create /dev/md0 --level=1 --raid-devices=2 /dev/sda1 /dev/sdb1 # 创建一个 raid
sudo dmsetup create myraid --table "0 $(blockdev --getsize /dev/md0) raid1 /dev/md0 0" # 这个错误无法找到啊
```

```c
static struct target_type mirror_target = {
	.name	 = "mirror",
	.version = {1, 14, 0},
	.module	 = THIS_MODULE,
	.ctr	 = mirror_ctr,
	.dtr	 = mirror_dtr,
	.map	 = mirror_map,
	.end_io	 = mirror_end_io,
	.presuspend = mirror_presuspend,
	.postsuspend = mirror_postsuspend,
	.resume	 = mirror_resume,
	.status	 = mirror_status,
	.iterate_devices = mirror_iterate_devices,
};
```

参考:
- https://superuser.com/questions/721795/how-fake-raid-communicates-with-operating-systemlinux : 关键总结
- https://www.jinbuguo.com/storage/raid_types.html
- https://skrypuch.com/raid/ : 三种 raid 的简单总结
- https://datahunter.org/fakeraid : 更加细节的使用

> Fake RAID communicates with Linux through device-mapper. There are several drivers involved in this (dm-stripe, dm-raid) and useful utility dmraid.


看上去，实际上这个东西没有什么人用。
