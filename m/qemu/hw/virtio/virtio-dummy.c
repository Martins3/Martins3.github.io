#include "qemu/osdep.h"
#include "qemu/cutils.h"
#include "qemu/iov.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "qemu/madvise.h"
#include "hw/virtio/virtio.h"
#include "hw/mem/pc-dimm.h"
#include "qapi/error.h"
#include "qapi/qapi-events-machine.h"
#include "qapi/visitor.h"
#include "trace.h"
#include "qemu/error-report.h"
#include "migration/misc.h"
#include "system/reset.h"
#include "hw/core/qdev-properties.h"
#include "hw/virtio/virtio-bus.h"

#include "virtio-dummy.h"

#include <stdio.h>

OBJECT_DECLARE_SIMPLE_TYPE(VirtIODummy, VIRTIO_DUMMY)

static const char virtio_dummy_id[] = "virtio-dummy";
static const Property virtio_dummy_properties[] = {
  DEFINE_PROP_SIZE("size", VirtIODummy, size, VIRTIO_DUMMY_DEFAULT_SIZE),
};

static void virtio_dummy_update_config(VirtIODummy *s) {
  s->config.capacity_sectors =
      cpu_to_le64(s->storage_size / VIRTIO_DUMMY_BLOCK_SIZE);
  s->config.blk_size = cpu_to_le32(VIRTIO_DUMMY_BLOCK_SIZE);
  s->config.max_segments = cpu_to_le32(VIRTIO_DUMMY_MAX_SEGS);
}

static uint64_t virtio_dummy_get_features(VirtIODevice *vdev, uint64_t f,
                                          Error **errp) {
  // TODO 不是太确定这里的实现，也不知道这个东西的作用是什么
  /* VirtIODummy *dev = VIRTIO_DUMMY(vdev); */
  // f |= VIRTIO_DUMMY_F_INJECT_INTERRUPT;
  // virtio_add_feature(&f, VIRTIO_DUMMY_F_INJECT_INTERRUPT);

  return f;
}

static uint32_t virtio_dummy_in_len(uint32_t type, uint32_t data_len) {
  switch (type) {
  case VIRTIO_DUMMY_T_IN:
  case VIRTIO_DUMMY_T_GET_ID:
    return data_len + 1;
  case VIRTIO_DUMMY_T_OUT:
  case VIRTIO_DUMMY_T_FLUSH:
  case VIRTIO_DUMMY_T_WRITE_ZEROES:
    return 1;
  default:
    return 1;
  }
}

static void virtio_dummy_complete(VirtIODevice *vdev, VirtQueue *vq,
                                  VirtQueueElement *elem, uint32_t type,
                                  uint32_t data_len, uint8_t status) {
  uint64_t status_offset = type == VIRTIO_DUMMY_T_IN ||
                           type == VIRTIO_DUMMY_T_GET_ID ? data_len : 0;
  uint32_t in_len = virtio_dummy_in_len(type, data_len);

  iov_from_buf(elem->in_sg, elem->in_num, status_offset, &status,
               sizeof(status));
  virtqueue_push(vq, elem, in_len);
  virtio_notify(vdev, vq);
  g_free(elem);
}

static void virtio_dummy_handle_output(VirtIODevice *vdev, VirtQueue *vq) {
  VirtIODummy *s = VIRTIO_DUMMY(vdev);
  VirtQueueElement *elem;

  while ((elem = virtqueue_pop(vq, sizeof(*elem))) != NULL) {
    struct virtio_dummy_req_hdr hdr;
    uint64_t offset;
    uint32_t type;
    uint32_t data_len;
    uint8_t status = VIRTIO_DUMMY_S_IOERR;
    size_t out_size = iov_size(elem->out_sg, elem->out_num);
    size_t in_size = iov_size(elem->in_sg, elem->in_num);

    if (out_size < sizeof(hdr)) {
      virtio_dummy_complete(vdev, vq, elem, VIRTIO_DUMMY_T_OUT, 0, status);
      continue;
    }

    iov_to_buf(elem->out_sg, elem->out_num, 0, &hdr, sizeof(hdr));
    type = le32_to_cpu(hdr.type);
    data_len = le32_to_cpu(hdr.data_len);
    offset = le64_to_cpu(hdr.sector) * VIRTIO_DUMMY_BLOCK_SIZE;

    if (offset > s->storage_size || data_len > s->storage_size - offset) {
      virtio_dummy_complete(vdev, vq, elem, type, data_len, status);
      continue;
    }

    switch (type) {
    case VIRTIO_DUMMY_T_OUT:
      if (out_size < sizeof(hdr) + data_len || in_size < 1) {
        break;
      }
      iov_to_buf(elem->out_sg, elem->out_num, sizeof(hdr), s->storage + offset,
                 data_len);
      status = VIRTIO_DUMMY_S_OK;
      break;
    case VIRTIO_DUMMY_T_IN:
      if (in_size < data_len + 1) {
        break;
      }
      iov_from_buf(elem->in_sg, elem->in_num, 0, s->storage + offset,
                   data_len);
      status = VIRTIO_DUMMY_S_OK;
      break;
    case VIRTIO_DUMMY_T_FLUSH:
      if (in_size < 1) {
        break;
      }
      status = VIRTIO_DUMMY_S_OK;
      break;
    case VIRTIO_DUMMY_T_WRITE_ZEROES:
      if (in_size < 1) {
        break;
      }
      memset(s->storage + offset, 0, data_len);
      status = VIRTIO_DUMMY_S_OK;
      break;
    case VIRTIO_DUMMY_T_GET_ID: {
      char serial[32] = {0};

      if (in_size < data_len + 1) {
        break;
      }
      pstrcpy(serial, sizeof(serial), virtio_dummy_id);
      iov_from_buf(elem->in_sg, elem->in_num, 0, serial,
                   MIN(data_len, sizeof(serial)));
      status = VIRTIO_DUMMY_S_OK;
      break;
    }
    default:
      status = VIRTIO_DUMMY_S_UNSUPP;
      break;
    }

    virtio_dummy_complete(vdev, vq, elem, type, data_len, status);
  }
}

static void virtio_dummy_device_realize(DeviceState *dev, Error **errp) {
  VirtIODevice *vdev = VIRTIO_DEVICE(dev);
  VirtIODummy *s = VIRTIO_DUMMY(dev);

  if (!s->size) {
    s->size = VIRTIO_DUMMY_DEFAULT_SIZE;
  }
  if (!QEMU_IS_ALIGNED(s->size, VIRTIO_DUMMY_BLOCK_SIZE)) {
    error_setg(errp, "virtio-dummy size must be aligned to %u bytes",
               VIRTIO_DUMMY_BLOCK_SIZE);
    return;
  }

  s->storage_size = s->size;
  s->storage = g_malloc0(s->storage_size);
  virtio_dummy_update_config(s);

  virtio_init(vdev, VIRTIO_ID_DUMMY, sizeof(s->config));
  s->vq = virtio_add_queue(vdev, 128, virtio_dummy_handle_output);
}

static void virtio_dummy_device_unrealize(DeviceState *dev) {
  VirtIODevice *vdev = VIRTIO_DEVICE(dev);
  VirtIODummy *s = VIRTIO_DUMMY(dev);

  virtio_delete_queue(s->vq);
  virtio_cleanup(vdev);
  g_free(s->storage);
  s->storage = NULL;
}

static void virtio_dummy_device_reset(VirtIODevice *vdev) {
  VirtIODummy *s = VIRTIO_DUMMY(vdev);

  // TODO 什么时候调用 reset ?
  if (s->storage) {
    memset(s->storage, 0, s->storage_size);
  }
}

static void virtio_dummy_get_config(VirtIODevice *vdev, uint8_t *config_data) {
  VirtIODummy *s = VIRTIO_DUMMY(vdev);

  memcpy(config_data, &s->config, sizeof(s->config));
}

static void virtio_dummy_set_config(VirtIODevice *vdev,
                                    const uint8_t *config_data) {
  VirtIODummy *s = VIRTIO_DUMMY(vdev);

  memcpy(&s->config, config_data, sizeof(s->config));
}

static int virtio_dummy_set_status(VirtIODevice *vdev, uint8_t status) {
  /* VirtIODummy *s = VIRTIO_DUMMY(vdev); */
  // TODO 这个什么时候调用这个函数
  return 0;
}

static void virtio_dummy_class_init(ObjectClass *klass, const void *data) {
  DeviceClass *dc = DEVICE_CLASS(klass);
  VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);

  set_bit(DEVICE_CATEGORY_STORAGE, dc->categories);
  device_class_set_props(dc, virtio_dummy_properties);
  vdc->realize = virtio_dummy_device_realize;
  vdc->unrealize = virtio_dummy_device_unrealize;
  vdc->reset = virtio_dummy_device_reset;
  vdc->get_config = virtio_dummy_get_config;
  vdc->set_config = virtio_dummy_set_config;
  vdc->get_features = virtio_dummy_get_features;
  vdc->set_status = virtio_dummy_set_status;

  // TODO 还有热迁移和 reset 相关的配置，到时候研究下
}

static void virtio_dummy_instance_init(Object *obj) {
  VirtIODummy *s = VIRTIO_DUMMY(obj);
  printf("[martins3:%s:%d] %p\n", __FUNCTION__, __LINE__, s);
}

static const TypeInfo virtio_dummy_info = {
    .name = TYPE_VIRTIO_DUMMY,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIODummy),
    .instance_init = virtio_dummy_instance_init,
    .class_init = virtio_dummy_class_init,
};

static void virtio_register_types(void) {
  type_register_static(&virtio_dummy_info);
}

type_init(virtio_register_types)

