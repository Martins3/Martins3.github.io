#!/usr/bin/env bash
set -E -e -u -o pipefail

PROGDIR=$(readlink -m "$(dirname "$0")")
pushd "$PROGDIR"

# dockerfile=$(find ./*.Dockerfile | fzf)
dockerfile=fedora.Dockerfile
image_name=$(basename "${dockerfile%.*}")

set -x
podman build --network=host -t "martins3:$image_name" -f "$dockerfile" .
# echo "docker run -it martins3:$image_name"
