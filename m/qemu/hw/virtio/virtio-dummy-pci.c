#include "qemu/osdep.h"

#include "hw/virtio/virtio-pci.h"
#include "hw/core/qdev-properties.h"
#include "qapi/error.h"
#include "qemu/module.h"
#include "qom/object.h"
#include "virtio-dummy.h"

typedef struct VirtIODummyPCI VirtIODummyPCI;

/*
 * virtio-dummy-pci: This extends VirtioPCIProxy.
 */
#define TYPE_VIRTIO_DUMMY_PCI "virtio-dummy-pci-base"
DECLARE_INSTANCE_CHECKER(VirtIODummyPCI, VIRTIO_DUMMY_PCI,
                         TYPE_VIRTIO_DUMMY_PCI)

struct VirtIODummyPCI {
  VirtIOPCIProxy parent_obj;
  VirtIODummy vdev;
};

// TODO eventfd 也是需要测试一下的
static const Property virtio_dummy_pci_properties[] = {
    DEFINE_PROP_UINT32("class", VirtIOPCIProxy, class_code, 0),
    DEFINE_PROP_UINT32("vectors", VirtIOPCIProxy, nvectors,
                       DEV_NVECTORS_UNSPECIFIED),
};

static void virtio_dummy_pci_realize(VirtIOPCIProxy *vpci_dev, Error **errp) {
  VirtIODummyPCI *dev = VIRTIO_DUMMY_PCI(vpci_dev);
  DeviceState *vdev = DEVICE(&dev->vdev);

  // TODO 这里必须设置为 2 之后，virtio 才会变为 msix 的中断
  // 换言之，我们可以通过这个来调整测试 msi 和非 msi 中断
  // 这个到底是什么东西来着?
  if (vpci_dev->nvectors == DEV_NVECTORS_UNSPECIFIED) {
      vpci_dev->nvectors = 2;
  }

  qdev_realize(vdev, BUS(&vpci_dev->bus), errp);
}

#define PCI_DEVICE_ID_VIRTIO_DUMMY 0x1015
static void virtio_dummy_pci_class_init(ObjectClass *klass, const void *data) {
  DeviceClass *dc = DEVICE_CLASS(klass);
  VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
  PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);

  set_bit(DEVICE_CATEGORY_STORAGE, dc->categories);
  device_class_set_props(dc, virtio_dummy_pci_properties);
  k->realize = virtio_dummy_pci_realize;

  pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
  pcidev_k->device_id = PCI_DEVICE_ID_VIRTIO_DUMMY;
  pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
  pcidev_k->class_id = PCI_CLASS_STORAGE_SCSI;
}

static void virtio_dummy_pci_instance_init(Object *obj) {
  VirtIODummyPCI *dev = VIRTIO_DUMMY_PCI(obj);

  virtio_instance_init_common(obj, &dev->vdev, sizeof(dev->vdev),
                              TYPE_VIRTIO_DUMMY);
}

static const VirtioPCIDeviceTypeInfo virtio_dummy_pci_info = {
    .base_name = TYPE_VIRTIO_DUMMY_PCI,
    .generic_name = "virtio-dummy-pci",
    .transitional_name = "virtio-dummy-pci-transitional",
    .non_transitional_name = "virtio-dummy-pci-non-transitional",
    .instance_size = sizeof(VirtIODummyPCI),
    .instance_init = virtio_dummy_pci_instance_init,
    .class_init = virtio_dummy_pci_class_init,
};

static void virtio_dummy_pci_register(void) {
  virtio_pci_types_register(&virtio_dummy_pci_info);
}

type_init(virtio_dummy_pci_register)
