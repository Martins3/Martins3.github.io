# 建议全文背诵

Plug and Play is a technology that offers support for automatically adding and removing devices to your system. This reduces conflicts with the resources they use by automatically configuring them at system startup. In order to achieve these goals, the following features are required:

- Automatic detection of adding and removing devices in the system (the device and its bus must notify the appropriate driver that a configuration change occurred).
- Resource management (addresses, irq lines, DMA channels, memory areas), including resource allocation to devices and solving conflicts that may arise.
- Devices must allow for software configuration (device resources - ports, interrupts, DMA resources - must allow for driver assignment).
- The drivers required for new devices must be loaded automatically by the operating system when needed.
- When the device and its bus allow, the system should be able to add or remove the device from the system while it is running, without having to reboot the system ( hotplug ).

## 笔记
1. `bus_find_device_by_name` : 存在这种函数 !
2. **Other possible operations on a bus are browsing the drivers or devices attached to it**.
Although we can not directly access them (lists of drives and devices being stored in the private data of the driver, the subsys_private * p field ),
these can be scanned using the *bus_for_each_dev* and *bus_for_each_drv* macrodefines .




## 资料

```c
static inline const char *dev_name(const struct device *dev)
{
  /* Use the init name until the kobject becomes available */
  if (dev->init_name)
    return dev->init_name;

  return kobject_name(&dev->kobj);
}

static inline const char *kobject_name(const struct kobject *kobj)
{
  return kobj->name;
}
```

```c
/**
 * dev_set_name - set a device name
 * @dev: device
 * @fmt: format string for the device's name
 */
int dev_set_name(struct device *dev, const char *fmt, ...)
{
  va_list vargs;
  int err;

  va_start(vargs, fmt);
  err = kobject_set_name_vargs(&dev->kobj, fmt, vargs); // 调用kvasprintf_const，然后给 kobj 赋值
  va_end(vargs);
  return err;
}
```

[IOMMU](https://en.wikipedia.org/wiki/Input%E2%80%93output_memory_management_unit)

```c

/**
 * A bus is a channel between the processor and one or more devices. For the
 * purposes of the device model, all devices are connected via a bus, even if
 * it is an internal, virtual, "platform" bus. Buses can plug into each other.
 * A USB controller is usually a PCI device, for example. The device model
 * represents the actual connections between buses and the devices they control.
 * A bus is represented by the bus_type structure. It contains the name, the
 * default attributes, the bus' methods, PM operations, and the driver core's
 * private data.
 */
struct bus_type {
  const char    *name;
  const char    *dev_name;
  struct device   *dev_root;
  const struct attribute_group **bus_groups;
  const struct attribute_group **dev_groups;
  const struct attribute_group **drv_groups;

  int (*match)(struct device *dev, struct device_driver *drv);
  int (*uevent)(struct device *dev, struct kobj_uevent_env *env);
  int (*probe)(struct device *dev);
  int (*remove)(struct device *dev);
  void (*shutdown)(struct device *dev);

  int (*online)(struct device *dev);
  int (*offline)(struct device *dev);

  int (*suspend)(struct device *dev, pm_message_t state);
  int (*resume)(struct device *dev);

  int (*num_vf)(struct device *dev);

  int (*dma_configure)(struct device *dev);

  const struct dev_pm_ops *pm; // 电池管理

  const struct iommu_ops *iommu_ops;

  struct subsys_private *p;
  struct lock_class_key lock_key;

  bool need_parent_lock;
};
```


[](http://www.staroceans.org/kernel-and-driver/The%20Linux%20Kernel%20Driver%20Model.pdf)


```c
struct attribute {
  const char    *name;
  umode_t     mode;
};

/**
 * struct attribute_group - data structure used to declare an attribute group.
 * @name: Optional: Attribute group name
 *    If specified, the attribute group will be created in
 *    a new subdirectory with this name.
 * @is_visible: Optional: Function to return permissions associated with an
 *    attribute of the group. Will be called repeatedly for each
 *    non-binary attribute in the group. Only read/write
 *    permissions as well as SYSFS_PREALLOC are accepted. Must
 *    return 0 if an attribute is not visible. The returned value
 *    will replace static permissions defined in struct attribute.
 * @is_bin_visible:
 *    Optional: Function to return permissions associated with a
 *    binary attribute of the group. Will be called repeatedly
 *    for each binary attribute in the group. Only read/write
 *    permissions as well as SYSFS_PREALLOC are accepted. Must
 *    return 0 if a binary attribute is not visible. The returned
 *    value will replace static permissions defined in
 *    struct bin_attribute.
 * @attrs:  Pointer to NULL terminated list of attributes.
 * @bin_attrs:  Pointer to NULL terminated list of binary attributes.
 *    Either attrs or bin_attrs or both must be provided.
 */
struct attribute_group {
  const char    *name;
  umode_t     (*is_visible)(struct kobject *,
                struct attribute *, int);
  umode_t     (*is_bin_visible)(struct kobject *,
              struct bin_attribute *, int);
  struct attribute  **attrs;
  struct bin_attribute  **bin_attrs;
};
```

```c
/*
 * The type of device, "struct device" is embedded in. A class
 * or bus can contain devices of different types
 * like "partitions" and "disks", "mouse" and "event".
 * This identifies the device type and carries type-specific
 * information, equivalent to the kobj_type of a kobject.
 * If "name" is specified, the uevent will contain it in
 * the DEVTYPE variable.
 */
struct device_type {
  const char *name;
  const struct attribute_group **groups;
  int (*uevent)(struct device *dev, struct kobj_uevent_env *env);
  char *(*devnode)(struct device *dev, umode_t *mode,
       kuid_t *uid, kgid_t *gid);
  void (*release)(struct device *dev);

  const struct dev_pm_ops *pm;
};
```

```c
/**
 * The device driver-model tracks all of the drivers known to the system.
 * The main reason for this tracking is to enable the driver core to match
 * up drivers with new devices. Once drivers are known objects within the
 * system, however, a number of other things become possible. Device drivers
 * can export information and configuration variables that are independent
 * of any specific device.
 */
struct device_driver {
  const char    *name;
  struct bus_type   *bus;

  struct module   *owner;
  const char    *mod_name;  /* used for built-in modules */

  bool suppress_bind_attrs; /* disables bind/unbind via sysfs */
  enum probe_type probe_type;

  const struct of_device_id *of_match_table;
  const struct acpi_device_id *acpi_match_table;

  int (*probe) (struct device *dev);
  int (*remove) (struct device *dev);
  void (*shutdown) (struct device *dev);
  int (*suspend) (struct device *dev, pm_message_t state);
  int (*resume) (struct device *dev);
  const struct attribute_group **groups;

  const struct dev_pm_ops *pm;
  void (*coredump) (struct device *dev);

  struct driver_private *p;
};
```
> @todo 所以，是不是说，device_driver 让 device 和 driver 分离了 ?


## 问题
1. hotplug 和 plug and play 的关系是什么 ?
2. uevent ?
3. device 和 device driver 的区别是什么 ?
4. 居然存在两个 init_module，那么到底如何调用 ?
   1. 居然就是生成两个 ko 文件
5. 初始化分别注册了 : device_driver 和 device 关系是什么 ?
6. 之前的 char 以及 block 设备 没有使用现在的内容但是依旧非常的正常呀 ?

## Plans for next
1. 设备驱动的书籍必然会存在关于 bus usb 等等的内容，找到另一本 device driver 的书籍


## bug

```c
? device_create_file+0x34/0xa0
device_add+0x3aa/0x630
? bex_unregister_driver+0x10/0x10 [bex]
? bex_unregister_driver+0x10/0x10 [bex]
device_register+0x16/0x20


EIP: bus_add_device+0x5d/0x150
```

```c
  /*
   * device_add does this:
   *    bus_add_device(dev)
   *    ->device_attach(dev)
   *      ->for each driver drv registered on the bus that dev is on
   *          if (dev.drv)  **  device already has a driver **
   *            ** not sure we could ever get here... **
   *          else
   *            if (bus.match(dev,drv)) [visorbus_match]
   *              dev.drv = drv
   *              if (!drv.probe(dev))  [visordriver_probe_device]
   *                dev.drv = NULL
   *
   * Note that device_add does NOT fail if no driver failed to claim the
   * device.  The device will be linked onto bus_type.klist_devices
   * regardless (use bus_for_each_dev).
   */
```

```c
/**
 * device_add - add device to device hierarchy.
 * @dev: device.
 *
 * This is part 2 of device_register(), though may be called
 * separately _iff_ device_initialize() has been called separately.
 *
 * This adds @dev to the kobject hierarchy via kobject_add(), adds it
 * to the global and sibling lists for the device, then
 * adds it to the other relevant subsystems of the driver model.
 *
 * Do not call this routine or device_register() more than once for
 * any device structure.  The driver model core is not designed to work
 * with devices that get unregistered and then spring back to life.
 * (Among other things, it's very hard to guarantee that all references
 * to the previous incarnation of @dev have been dropped.)  Allocate
 * and register a fresh new struct device instead.
 *
 * NOTE: _Never_ directly free @dev after calling this function, even
 * if it returned an error! Always use put_device() to give up your
 * reference instead.
 */
int device_add(struct device *dev)
```
