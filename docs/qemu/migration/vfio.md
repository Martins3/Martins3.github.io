# vfio migration

## 感觉提交的很随意啊
文件中出现的两个是什么意思哇:
```c
/* ---------------------------------------------------------------------- */
```

2. 这个名称的来源:
```c
VFIO_REGION_TYPE_MIGRATION_DEPRECATED
```

```diff
History:        #0
Commit:         e4082063e47e9731dbeb1c26174c17f6038f577f
Author:         Alex Williamson <alex.williamson@redhat.com>
Author Date:    Fri 13 May 2022 10:20:08 PM CST
Committer Date: Fri 13 May 2022 10:20:11 PM CST

linux-headers: Update to v5.18-rc6

Update to c5eb0a61238d ("Linux 5.18-rc6").  Mechanical search and
replace of vfio defines with white space massaging.

Signed-off-by: Alex Williamson <alex.williamson@redhat.com>
```

3. `vfio_device_migration_info` 在内核中，完全没有引用的位置啊

## 两个外围的关系

- `vfio_migration_probe`
    - `vfio_get_dev_region_info` : 到底得到是什么东西？
    - `vfio_migration_init`
        - `vfio_region_setup`
        - `register_savevm_live`

在 linux-headers/linux/vfio.h 中详细的描述了 VFIO migration 的过程中，内核的升级过程。


## 分析主要 Hook

- `vfio_save_setup`
    - `vfio_region_mmap`
    - `vfio_migration_set_state`
        - `vfio_mig_read`
            - `vfio_mig_access` ：就是对于 fd 进行读写的
        - `vfio_mig_write`


## 内核中似乎出现了一个巨大的改动

```diff
History:        #0
Commit:         115dcec65f61d53e25e1bed5e380468b30f98b14
Author:         Jason Gunthorpe <jgg@ziepe.ca>
Committer:      Leon Romanovsky <leon@kernel.org>
Author Date:    Thu 24 Feb 2022 10:20:18 PM CST
Committer Date: Thu 03 Mar 2022 06:57:39 PM CST

vfio: Define device migration protocol v2

Replace the existing region based migration protocol with an ioctl based
protocol. The two protocols have the same general semantic behaviors, but
the way the data is transported is changed.

This is the STOP_COPY portion of the new protocol, it defines the 5 states
for basic stop and copy migration and the protocol to move the migration
data in/out of the kernel.

Compared to the clarification of the v1 protocol Alex proposed:

https://lore.kernel.org/r/163909282574.728533.7460416142511440919.stgit@omen

This has a few deliberate functional differences:

 - ERROR arcs allow the device function to remain unchanged.

 - The protocol is not required to return to the original state on
   transition failure. Instead userspace can execute an unwind back to
   the original state, reset, or do something else without needing kernel
   support. This simplifies the kernel design and should userspace choose
   a policy like always reset, avoids doing useless work in the kernel
   on error handling paths.

 - PRE_COPY is made optional, userspace must discover it before using it.
   This reflects the fact that the majority of drivers we are aware of
   right now will not implement PRE_COPY.

 - segmentation is not part of the data stream protocol, the receiver
   does not have to reproduce the framing boundaries.

The hybrid FSM for the device_state is described as a Mealy machine by
documenting each of the arcs the driver is required to implement. Defining
the remaining set of old/new device_state transitions as 'combination
transitions' which are naturally defined as taking multiple FSM arcs along
the shortest path within the FSM's digraph allows a complete matrix of
transitions.

A new VFIO_DEVICE_FEATURE of VFIO_DEVICE_FEATURE_MIG_DEVICE_STATE is
defined to replace writing to the device_state field in the region. This
allows returning a brand new FD whenever the requested transition opens
a data transfer session.

The VFIO core code implements the new feature and provides a helper
function to the driver. Using the helper the driver only has to
implement 6 of the FSM arcs and the other combination transitions are
elaborated consistently from those arcs.

A new VFIO_DEVICE_FEATURE of VFIO_DEVICE_FEATURE_MIGRATION is defined to
report the capability for migration and indicate which set of states and
arcs are supported by the device. The FSM provides a lot of flexibility to
make backwards compatible extensions but the VFIO_DEVICE_FEATURE also
allows for future breaking extensions for scenarios that cannot support
even the basic STOP_COPY requirements.

The VFIO_DEVICE_FEATURE_MIG_DEVICE_STATE with the GET option (i.e.
VFIO_DEVICE_FEATURE_GET) can be used to read the current migration state
of the VFIO device.

Data transfer sessions are now carried over a file descriptor, instead of
the region. The FD functions for the lifetime of the data transfer
session. read() and write() transfer the data with normal Linux stream FD
semantics. This design allows future expansion to support poll(),
io_uring, and other performance optimizations.

The complicated mmap mode for data transfer is discarded as current qemu
doesn't take meaningful advantage of it, and the new qemu implementation
avoids substantially all the performance penalty of using a read() on the
region.

Link: https://lore.kernel.org/all/20220224142024.147653-10-yishaih@nvidia.com
Signed-off-by: Jason Gunthorpe <jgg@nvidia.com>
Tested-by: Shameer Kolothum <shameerali.kolothum.thodi@huawei.com>
Reviewed-by: Kevin Tian <kevin.tian@intel.com>
Reviewed-by: Alex Williamson <alex.williamson@redhat.com>
Reviewed-by: Cornelia Huck <cohuck@redhat.com>
Signed-off-by: Yishai Hadas <yishaih@nvidia.com>
Signed-off-by: Leon Romanovsky <leonro@nvidia.com>
```
