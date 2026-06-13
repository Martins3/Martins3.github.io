#!/usr/bin/env drgn
"""
展示当前环境 DRM/KMS 关键对象与关系:

    drm_connector -> drm_encoder -> drm_crtc
    drm_plane -> drm_crtc -> drm_framebuffer

用法:
    ./drgn-wrapper.sh -c /proc/kcore ./drm_kms_relationship.py
"""

from drgn import cast
from drgn.helpers.linux import list_for_each_entry
from drgn.helpers.linux.xarray import xa_for_each


DRM_MINOR_PRIMARY = 0

PLANE_TYPE_NAMES = {
    0: "OVERLAY",
    1: "PRIMARY",
    2: "CURSOR",
}

CONNECTOR_STATUS_NAMES = {
    1: "connected",
    2: "disconnected",
    3: "unknown",
}

ENCODER_TYPE_NAMES = {
    0: "NONE",
    1: "DAC",
    2: "TMDS",
    3: "LVDS",
    4: "TVDAC",
    5: "VIRTUAL",
    6: "DSI",
    7: "DPMST",
    8: "DPI",
}


def is_null(obj):
    return obj.value_() == 0


def ptr(obj):
    return f"{obj.value_():#x}"


def read_c_string(obj, default="?"):
    try:
        if is_null(obj):
            return default
        return obj.string_().decode("utf-8", errors="replace")
    except Exception:
        return default


def bool_str(value):
    return "yes" if bool(value) else "no"


def fourcc_to_str(value):
    if value == 0:
        return "0"

    chars = []
    for shift in range(0, 32, 8):
        ch = (value >> shift) & 0xFF
        if 32 <= ch <= 126:
            chars.append(chr(ch))
        else:
            chars.append(".")
    return "".join(chars)


def object_id(obj):
    try:
        return obj.base.id.value_()
    except Exception:
        return -1


def sort_by_id(objs):
    return sorted(objs, key=lambda obj: (object_id(obj), obj.value_()))


def kobject_name(dev):
    try:
        return read_c_string(dev.kobj.name, "?")
    except Exception:
        return "?"


def drm_device_label(dev, minor):
    driver = read_c_string(dev.driver.name, "?")
    bus = "?"

    try:
        if not is_null(dev.dev):
            bus = kobject_name(dev.dev[0])
    except Exception:
        pass

    return f"minor={minor.index.value_()} driver={driver} bus={bus}"


def plane_type_name(plane):
    value = plane.type.value_()
    return PLANE_TYPE_NAMES.get(value, f"UNKNOWN({value})")


def connector_status_name(connector):
    value = connector.status.value_()
    return CONNECTOR_STATUS_NAMES.get(value, f"UNKNOWN({value})")


def encoder_type_name(encoder):
    value = encoder.encoder_type.value_()
    return ENCODER_TYPE_NAMES.get(value, f"UNKNOWN({value})")


def mode_object_label(prefix, obj, name=None, extra=None):
    parts = [f"{prefix}[id={object_id(obj)}]"]
    if name:
        parts.append(name)
    if extra:
        parts.append(extra)
    parts.append(ptr(obj))
    return " ".join(parts)


def plane_name(plane):
    name = read_c_string(plane.name, "")
    if name:
        return name
    return f"plane-{plane.index.value_()}"


def crtc_name(crtc):
    name = read_c_string(crtc.name, "")
    if name:
        return name
    return f"crtc-{crtc.index.value_()}"


def encoder_name(encoder):
    name = read_c_string(encoder.name, "")
    if name:
        return name
    return f"encoder-{encoder.index.value_()}"


def connector_name(connector):
    name = read_c_string(connector.name, "")
    if name:
        return name
    return f"connector-{connector.index.value_()}"


def fb_name(fb):
    return f"fb-{object_id(fb)}"


def plane_label(plane):
    return mode_object_label(
        "plane",
        plane,
        plane_name(plane),
        f"type={plane_type_name(plane)}",
    )


def crtc_label(crtc):
    return mode_object_label("crtc", crtc, crtc_name(crtc), f"index={crtc.index.value_()}")


def encoder_label(encoder):
    return mode_object_label(
        "encoder",
        encoder,
        encoder_name(encoder),
        f"type={encoder_type_name(encoder)}",
    )


def connector_label(connector):
    return mode_object_label(
        "connector",
        connector,
        connector_name(connector),
        f"status={connector_status_name(connector)}",
    )


def fb_label(fb):
    return mode_object_label(
        "fb",
        fb,
        fb_name(fb),
        f"{fb.width.value_()}x{fb.height.value_()} fmt={fb_format_name(fb)}",
    )


def fb_format_name(fb):
    try:
        if is_null(fb.format):
            return "?"
        fmt = fb.format[0].format.value_()
        return fourcc_to_str(fmt)
    except Exception:
        return "?"


def current_plane_crtc(plane):
    try:
        if not is_null(plane.state):
            crtc = plane.state.crtc
            if not is_null(crtc):
                return crtc
    except Exception:
        pass

    try:
        if not is_null(plane.crtc):
            return plane.crtc
    except Exception:
        pass

    return None


def current_plane_fb(plane):
    try:
        if not is_null(plane.state):
            fb = plane.state.fb
            if not is_null(fb):
                return fb
    except Exception:
        pass

    try:
        if not is_null(plane.fb):
            return plane.fb
    except Exception:
        pass

    return None


def current_connector_encoder(connector):
    try:
        if not is_null(connector.state):
            encoder = connector.state.best_encoder
            if not is_null(encoder):
                return encoder
    except Exception:
        pass

    try:
        if not is_null(connector.encoder):
            return connector.encoder
    except Exception:
        pass

    return None


def current_connector_crtc(connector):
    try:
        if not is_null(connector.state):
            crtc = connector.state.crtc
            if not is_null(crtc):
                return crtc
    except Exception:
        pass

    encoder = current_connector_encoder(connector)
    if encoder is not None:
        try:
            if not is_null(encoder.crtc):
                return encoder.crtc
        except Exception:
            pass

    return None


def current_encoder_crtc(encoder, connectors):
    try:
        if not is_null(encoder.crtc):
            return encoder.crtc
    except Exception:
        pass

    for connector in connectors:
        current = current_connector_encoder(connector)
        if current is not None and current.value_() == encoder.value_():
            crtc = current_connector_crtc(connector)
            if crtc is not None:
                return crtc

    return None


def list_objects(config, type_name, member, list_field):
    return list(
        list_for_each_entry(
            type_name,
            getattr(config, list_field).address_of_(),
            member,
        )
    )


def possible_crtcs(mask, crtcs):
    names = []
    for crtc in crtcs:
        index = crtc.index.value_()
        if mask & (1 << index):
            names.append(crtc_name(crtc))
    return names


def possible_encoders(mask, encoders):
    names = []
    for encoder in encoders:
        index = encoder.index.value_()
        if mask & (1 << index):
            names.append(encoder_name(encoder))
    return names


def join_names(names):
    return ", ".join(names) if names else "(none)"


def collect_primary_drm_devices():
    devices = []
    seen = set()

    for _, entry in xa_for_each(prog["drm_minors_xa"]):
        minor = cast("struct drm_minor *", entry)
        if minor.type.value_() != DRM_MINOR_PRIMARY:
            continue

        dev = minor.dev
        if dev.value_() in seen:
            continue
        seen.add(dev.value_())
        devices.append((minor, dev))

    devices.sort(key=lambda item: item[0].index.value_())
    return devices


def print_live_pipeline(connectors, plane_by_crtc, fb_users):
    print("  live graph")

    any_live = False
    for connector in sort_by_id(connectors):
        crtc = current_connector_crtc(connector)
        encoder = current_connector_encoder(connector)
        if crtc is None and encoder is None:
            continue

        any_live = True
        print(f"    {connector_name(connector)}")

        if encoder is not None:
            print(f"      -> {encoder_label(encoder)}")
        else:
            print("      -> encoder: (none)")

        if crtc is not None:
            print(f"      -> {crtc_label(crtc)}")

            planes = plane_by_crtc.get(crtc.value_(), [])
            if planes:
                for plane in sort_by_id(planes):
                    fb = current_plane_fb(plane)
                    if fb is not None:
                        print(f"         -> {plane_name(plane)} -> {fb_label(fb)}")
                    else:
                        print(f"         -> {plane_name(plane)} -> fb: (none)")

                    users = fb_users.get(fb.value_(), []) if fb is not None else []
                    if fb is not None and len(users) > 1:
                        print(f"            fb shared by: {join_names(users)}")
        else:
            print("      -> crtc: (none)")

    if not any_live:
        print("    (no active connector/encoder/crtc routing found)")


def print_connectors(connectors, encoders, crtcs):
    print("  connectors")
    if not connectors:
        print("    (none)")
        return

    for connector in sort_by_id(connectors):
        encoder = current_connector_encoder(connector)
        crtc = current_connector_crtc(connector)
        possible = possible_encoders(connector.possible_encoders.value_(), encoders)

        print(f"    {connector_label(connector)}")
        print(f"      possible_encoders: {join_names(possible)}")
        print(f"      current_encoder: {encoder_name(encoder) if encoder is not None else '(none)'}")
        print(f"      current_crtc: {crtc_name(crtc) if crtc is not None else '(none)'}")


def print_encoders(encoders, connectors, crtcs):
    print("  encoders")
    if not encoders:
        print("    (none)")
        return

    for encoder in sort_by_id(encoders):
        crtc = current_encoder_crtc(encoder, connectors)
        possible = possible_crtcs(encoder.possible_crtcs.value_(), crtcs)
        attached = []

        for connector in connectors:
            current = current_connector_encoder(connector)
            if current is not None and current.value_() == encoder.value_():
                attached.append(connector_name(connector))

        print(f"    {encoder_label(encoder)}")
        print(f"      possible_crtcs: {join_names(possible)}")
        print(f"      current_crtc: {crtc_name(crtc) if crtc is not None else '(none)'}")
        print(f"      attached_connectors: {join_names(attached)}")


def print_crtcs(crtcs, connectors, encoders, plane_by_crtc):
    print("  crtcs")
    if not crtcs:
        print("    (none)")
        return

    for crtc in sort_by_id(crtcs):
        connector_names = []
        encoder_names = []

        for connector in connectors:
            current_crtc = current_connector_crtc(connector)
            if current_crtc is None or current_crtc.value_() != crtc.value_():
                continue

            connector_names.append(connector_name(connector))
            current_encoder = current_connector_encoder(connector)
            if current_encoder is not None:
                encoder_names.append(encoder_name(current_encoder))

        active_planes = plane_by_crtc.get(crtc.value_(), [])
        primary_name = plane_name(crtc.primary) if not is_null(crtc.primary) else "(none)"
        cursor_name = plane_name(crtc.cursor) if not is_null(crtc.cursor) else "(none)"

        print(f"    {crtc_label(crtc)}")
        print(f"      primary_plane: {primary_name}")
        print(f"      cursor_plane: {cursor_name}")
        print(f"      connectors: {join_names(connector_names)}")
        print(f"      encoders: {join_names(sorted(set(encoder_names)))}")
        print(f"      active_planes: {join_names([plane_name(plane) for plane in sort_by_id(active_planes)])}")


def print_planes(planes, crtcs):
    print("  planes")
    if not planes:
        print("    (none)")
        return

    for plane in sort_by_id(planes):
        crtc = current_plane_crtc(plane)
        fb = current_plane_fb(plane)
        possible = possible_crtcs(plane.possible_crtcs.value_(), crtcs)

        print(f"    {plane_label(plane)}")
        print(f"      possible_crtcs: {join_names(possible)}")
        print(f"      current_crtc: {crtc_name(crtc) if crtc is not None else '(none)'}")
        print(f"      current_fb: {fb_label(fb) if fb is not None else '(none)'}")

        try:
            if not is_null(plane.state):
                print(
                    "      state:"
                    f" crtc_xy=({plane.state.crtc_x.value_()}, {plane.state.crtc_y.value_()})"
                    f" crtc_wh=({plane.state.crtc_w.value_()}x{plane.state.crtc_h.value_()})"
                    f" zpos={plane.state.normalized_zpos.value_()}"
                )
        except Exception:
            pass


def print_fbs(fbs, fb_users):
    print("  framebuffers")
    if not fbs:
        print("    (none)")
        return

    for fb in sort_by_id(fbs):
        users = fb_users.get(fb.value_(), [])
        print(f"    {fb_label(fb)}")
        print(f"      modifier: {fb.modifier.value_():#x}")
        print(f"      pitches: {', '.join(str(fb.pitches[i].value_()) for i in range(4))}")
        print(f"      users: {join_names(users)}")


def build_plane_by_crtc(planes):
    result = {}

    for plane in planes:
        crtc = current_plane_crtc(plane)
        if crtc is None:
            continue
        result.setdefault(crtc.value_(), []).append(plane)

    return result


def build_fb_users(planes):
    result = {}

    for plane in planes:
        fb = current_plane_fb(plane)
        if fb is None:
            continue
        result.setdefault(fb.value_(), []).append(plane_name(plane))

    return result


def print_device(minor, dev):
    config = dev.mode_config

    planes = list_objects(config, "struct drm_plane", "head", "plane_list")
    crtcs = list_objects(config, "struct drm_crtc", "head", "crtc_list")
    encoders = list_objects(config, "struct drm_encoder", "head", "encoder_list")
    connectors = list_objects(config, "struct drm_connector", "head", "connector_list")
    fbs = list_objects(config, "struct drm_framebuffer", "head", "fb_list")

    plane_by_crtc = build_plane_by_crtc(planes)
    fb_users = build_fb_users(planes)

    print(f"[Device] {drm_device_label(dev, minor)}")
    print(f"  drm_device: {ptr(dev)} registered={bool_str(dev.registered.value_())}")
    print(
        "  counts:"
        f" planes={len(planes)}"
        f" crtcs={len(crtcs)}"
        f" encoders={len(encoders)}"
        f" connectors={len(connectors)}"
        f" framebuffers={len(fbs)}"
    )

    print_live_pipeline(connectors, plane_by_crtc, fb_users)
    print_connectors(connectors, encoders, crtcs)
    print_encoders(encoders, connectors, crtcs)
    print_crtcs(crtcs, connectors, encoders, plane_by_crtc)
    print_planes(planes, crtcs)
    print_fbs(fbs, fb_users)
    print("")


def main():
    devices = collect_primary_drm_devices()

    print("=== DRM/KMS Relationships ===")
    print("")
    print("关系主线:")
    print("  connector -> encoder -> crtc")
    print("  plane -> crtc -> framebuffer")
    print("")

    if not devices:
        print("未找到 DRM primary device")
        return

    for minor, dev in devices:
        print_device(minor, dev)


main()
