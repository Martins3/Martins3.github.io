# Linux Device Driver : USB Drivers
It was originally created to replace a wide range of slow
and different buses—the parallel, serial, and keyboard connections—with a single
bus type that all devices could connect to
> 所以为什么机器上还是有串口，并口。还是调试用的串口有usb 做不到的事情。

Topologically, a USB subsystem is not laid out as a bus; it is rather a tree built out of
several point-to-point links. 

The
USB host controller is in charge of asking every USB device if it has any data to send.

The USB protocol specifications define a set of standards that any device of a specific type can follow.
If a device follows that standard, then a special driver for that
device is not necessary. These different types are called classes and consist of things
like storage devices, keyboards, mice, joysticks, network devices, and modems
> 原来这些东西都是不需要驱动的啊，所以到底是如何支持键盘的? 只能说所有制造商的键盘都是采用相同的驱动，但是键盘不会仅仅支持USB而不需要特殊的驱动。

Video devices and USB-to-serial
devices are a good example where there is no defined standard, and a driver is
needed for every different device from different manufacturers.

The Linux kernel supports two main types of USB drivers: drivers on a host system
and drivers on a device. 

## 13.1 USB Device Basics
Fortunately, the Linux kernel provides a subsystem called the USB core to handle most of the complexity.

#### 13.1.1 Endpoints
Endpoints can be thought of as unidirectional pipes.

A USB endpoint can be one of four different types that describe how the data is
transmitted:
1. CONTROL
2. INTERRUPT
3. BULK
4. ISOCHRONOUS

USB endpoints are described in the kernel with the structure struct `usb_host_endpoint`.
This structure contains the real endpoint information in another structure called
struct `usb_endpoint_descriptor`. The latter structure contains all of the USB-specific
data in the exact format that the device itself specified. The fields of this structure that
drivers care about are:
> 具体描述看书吧!

```c
/**
 * struct usb_host_endpoint - host-side endpoint descriptor and queue
 * @desc: descriptor for this endpoint, wMaxPacketSize in native byteorder
 * @ss_ep_comp: SuperSpeed companion descriptor for this endpoint
 * @urb_list: urbs queued to this endpoint; maintained by usbcore
 * @hcpriv: for use by HCD; typically holds hardware dma queue head (QH)
 *	with one or more transfer descriptors (TDs) per urb
 * @ep_dev: ep_device for sysfs info
 * @extra: descriptors following this endpoint in the configuration
 * @extralen: how many bytes of "extra" are valid
 * @enabled: URBs may be submitted to this endpoint
 * @streams: number of USB-3 streams allocated on the endpoint
 *
 * USB requests are always queued to a given endpoint, identified by a
 * descriptor within an active interface in a given USB configuration.
 */
struct usb_host_endpoint {
	struct usb_endpoint_descriptor		desc;
	struct usb_ss_ep_comp_descriptor	ss_ep_comp;
	struct list_head		urb_list;
	void				*hcpriv;
	struct ep_device		*ep_dev;	/* For sysfs info */

	unsigned char *extra;   /* Extra descriptors */
	int extralen;
	int enabled;
	int streams;
};
```


```c
/* USB_DT_ENDPOINT: Endpoint descriptor */
struct usb_endpoint_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;

	__u8  bEndpointAddress;
	__u8  bmAttributes;
	__le16 wMaxPacketSize;
	__u8  bInterval;

	/* NOTE:  these two are _only_ in audio endpoints. */
	/* use USB_DT_ENDPOINT*_SIZE in bLength, not sizeof. */
	__u8  bRefresh;
	__u8  bSynchAddress;
} __attribute__ ((packed));
```

#### 13.1.2 Interface
USB endpoints are bundled up into interfaces. USB interfaces handle only one type of
a USB logical connection, such as a mouse, a keyboard, or a audio stream. 


*Each device with an isochronous endpoint uses alternate settings for the same interface.*

USB interfaces are described in the kernel with the `struct usb_interface` structure.
This structure is what the USB core passes to USB drivers and is what the USB driver
then is in charge of controlling. The important fields in this structure are:


```
/**
 * struct usb_interface - what usb device drivers talk to
 * @altsetting: array of interface structures, one for each alternate
 *	setting that may be selected.  Each one includes a set of
 *	endpoint configurations.  They will be in no particular order.
 * @cur_altsetting: the current altsetting.
 * @num_altsetting: number of altsettings defined.
 * @intf_assoc: interface association descriptor
 * @minor: the minor number assigned to this interface, if this
 *	interface is bound to a driver that uses the USB major number.
 *	If this interface does not use the USB major, this field should
 *	be unused.  The driver should set this value in the probe()
 *	function of the driver, after it has been assigned a minor
 *	number from the USB core by calling usb_register_dev().
 * @condition: binding state of the interface: not bound, binding
 *	(in probe()), bound to a driver, or unbinding (in disconnect())
 * @sysfs_files_created: sysfs attributes exist
 * @ep_devs_created: endpoint child pseudo-devices exist
 * @unregistering: flag set when the interface is being unregistered
 * @needs_remote_wakeup: flag set when the driver requires remote-wakeup
 *	capability during autosuspend.
 * @needs_altsetting0: flag set when a set-interface request for altsetting 0
 *	has been deferred.
 * @needs_binding: flag set when the driver should be re-probed or unbound
 *	following a reset or suspend operation it doesn't support.
 * @authorized: This allows to (de)authorize individual interfaces instead
 *	a whole device in contrast to the device authorization.
 * @dev: driver model's view of this device
 * @usb_dev: if an interface is bound to the USB major, this will point
 *	to the sysfs representation for that device.
 * @pm_usage_cnt: PM usage counter for this interface
 * @reset_ws: Used for scheduling resets from atomic context.
 * @resetting_device: USB core reset the device, so use alt setting 0 as
 *	current; needs bandwidth alloc after reset.
 *
 * USB device drivers attach to interfaces on a physical device.  Each
 * interface encapsulates a single high level function, such as feeding
 * an audio stream to a speaker or reporting a change in a volume control.
 * Many USB devices only have one interface.  The protocol used to talk to
 * an interface's endpoints can be defined in a usb "class" specification,
 * or by a product's vendor.  The (default) control endpoint is part of
 * every interface, but is never listed among the interface's descriptors.
 *
 * The driver that is bound to the interface can use standard driver model
 * calls such as dev_get_drvdata() on the dev member of this structure.
 *
 * Each interface may have alternate settings.  The initial configuration
 * of a device sets altsetting 0, but the device driver can change
 * that setting using usb_set_interface().  Alternate settings are often
 * used to control the use of periodic endpoints, such as by having
 * different endpoints use different amounts of reserved USB bandwidth.
 * All standards-conformant USB devices that use isochronous endpoints
 * will use them in non-default settings.
 *
 * The USB specification says that alternate setting numbers must run from
 * 0 to one less than the total number of alternate settings.  But some
 * devices manage to mess this up, and the structures aren't necessarily
 * stored in numerical order anyhow.  Use usb_altnum_to_altsetting() to
 * look up an alternate setting in the altsetting array based on its number.
 */
struct usb_interface {
	/* array of alternate settings for this interface,
	 * stored in no particular order */
	struct usb_host_interface *altsetting;

	struct usb_host_interface *cur_altsetting;	/* the currently
					 * active alternate setting */
	unsigned num_altsetting;	/* number of alternate settings */

	/* If there is an interface association descriptor then it will list
	 * the associated interfaces */
	struct usb_interface_assoc_descriptor *intf_assoc;

	int minor;			/* minor number this interface is
					 * bound to */
	enum usb_interface_condition condition;		/* state of binding */
	unsigned sysfs_files_created:1;	/* the sysfs attributes exist */
	unsigned ep_devs_created:1;	/* endpoint "devices" exist */
	unsigned unregistering:1;	/* unregistration is in progress */
	unsigned needs_remote_wakeup:1;	/* driver requires remote wakeup */
	unsigned needs_altsetting0:1;	/* switch to altsetting 0 is pending */
	unsigned needs_binding:1;	/* needs delayed unbind/rebind */
	unsigned resetting_device:1;	/* true: bandwidth alloc after reset */
	unsigned authorized:1;		/* used for interface authorization */

	struct device dev;		/* interface specific device info */
	struct device *usb_dev;
	atomic_t pm_usage_cnt;		/* usage counter for autosuspend */
	struct work_struct reset_ws;	/* for resets in atomic context */
};
```

#### 13.1.3 Configurations
USB interfaces are themselves bundled up into configurations. A USB device can have
multiple configurations and might switch between them in order to change the state
of the device. 

Linux does not handle multiple configuration
USB devices very well, but, thankfully, they are rare

Linux describes USB configurations with the structure struct `usb_host_config` and
entire USB devices with the structure `struct usb_device`. 

A USB device driver commonly has to convert data from a given `struct usb_interface`
structure into a `struct usb_device` structure that the USB core needs for a wide range of
function calls.

So to summarize, USB devices are quite complex and are made up of lots of different
logical units. The relationships among these units can be simply described as follows:
1. Devices(`struct usb_device`) usually have one or more configurations.
2. Configurations(`struct usb_config`) often have one or more interfaces.
3. Interfaces(`struct usb_device`) usually have one or more settings.
4. Interfaces(`struct usb_host_endpoint`) have zero or more endpoints.


## 13.2 USB and Sysfs
Both the physical USB device (as represented by
a `struct usb_device`) and the individual USB interfaces (as represented by a struct
`usb_interface`) are shown in sysfs as individual devices


```
➜  Vn git:(master) ✗ tree  /sys/devices/pci0000:00/0000:00:14.0/usb2/2-0:1.0/ 
/sys/devices/pci0000:00/0000:00:14.0/usb2/2-0:1.0/
├── authorized
├── bAlternateSetting
├── bInterfaceClass
├── bInterfaceNumber
├── bInterfaceProtocol
├── bInterfaceSubClass
├── bNumEndpoints
├── driver -> ../../../../../bus/usb/drivers/hub
├── ep_81
│   ├── bEndpointAddress
│   ├── bInterval
│   ├── bLength
│   ├── bmAttributes
│   ├── direction
│   ├── interval
│   ├── power
│   │   ├── async
│   │   ├── autosuspend_delay_ms
│   │   ├── control
│   │   ├── runtime_active_kids
│   │   ├── runtime_active_time
│   │   ├── runtime_enabled
│   │   ├── runtime_status
│   │   ├── runtime_suspended_time
│   │   └── runtime_usage
│   ├── type
│   ├── uevent
│   └── wMaxPacketSize
> 后面还有，放不下了
```
The *first USB* device is a root hub. This is the *USB controller*, usually contained in a
PCI device. The controller is so named because it controls the whole USB bus connected to it. The controller is a bridge between the PCI bus and the USB bus, as well
as being the first USB device on that bus.
> 1. first ?
> 2. root hub ?
> 3. controller ?
> 4. bridge between PCI bus and USB bus ?

All root hubs are assigned a unique number by the USB core. In our example, the
root hub is called usb2, as it is the second root hub that was registered with the USB
core. There is no limit on the number of root hubs that can be contained in a single
system at any time.
> 1. unique number ?
> 2. second root hub ?

Every device that is on a USB bus takes the number of the root hub as the first number in its name. That is followed by a - character and then the number of the port
that the device is plugged into.
As the device in our example is plugged into the first
port, a 1 is added to the name. So the device name for the main USB mouse device is
2-1. Because this USB device contains one interface, that causes another device in the
tree to be added to the sysfs path. The namingscheme for USB interfaces is the
device name up to this point: in our example, it’s 2-1 followed by a colon and the
USB configuration number, then a period and the interface number. So for this
example, the device name is 2-1:1.0 because it is the first configuration and has
interface number zero.

o to summarize, the USB sysfs device naming scheme is:
`root_hub-hub_port:config.interface`


As the devices go further down in the USB tree, and as more and more USB hubs are
used, the hub port number is added to the stringfollowingthe previous hub port
number in the chain. For a two-deep tree, the device name looks like:
`root_hub-hub_port-hub_port:config.interface`

> 后面还有一点点东西，但是受不了
## 13.3 USB Urbs


The USB code in the Linux kernel communicates with all USB devices usingsomethingcalled a urb (USB request block). 

Every endpoint in a device can handle a queue of urbs, so that multiple urbs can be sent to the
same endpoint before the queue is `empty`. 
> excuse me ?　为什么是 empty ,　难道不是full 吗 ?


The typical lifecycle of a urb is as follows:
1. Created by a USB device driver.
1. Assigned to a specific endpoint of a specific USB device.
1. Submitted to the USB core, by the USB device driver.
1. Submitted to the specific USB host controller driver for the specified device by the USB core.
1. Processed by the USB host controller driver that makes a USB transfer to the device.
1. When the urb is completed, the USB host controller driver notifies the USB device driver.

The procedure described in this chapter for handlingurbs is useful, because it permits streamingand other complex, overlappingcommunications that allow drivers
to achieve the highest possible data transfer speeds. 


#### 13.3.1 struct urb
> 后面含有超级长的内容介绍 struct urb 以及 操纵的方法

## 13.4 Writing a USB Driver
> 依旧是和urb 相关


