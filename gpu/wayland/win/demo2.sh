#!/usr/bin/env bash
set -E -e -u -o pipefail

wget https://wayland.freedesktop.org/wayland.png
file wayland.png
magick convert wayland.png wayland.ppm
file wayland.ppm
xxd -s +15 -i wayland.ppm  > wayland-logo.h
sed -i 's/wayland_ppm/wayland_logo/g' wayland-logo.h
