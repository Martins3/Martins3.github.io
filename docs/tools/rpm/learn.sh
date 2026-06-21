#!/usr/bin/env bash
set -E -e -u -o pipefail

cd "$(dirname "$0")"
mkdir -p rpmbuild/SOURCES
PROGDIR=$(readlink -m "$(dirname "$0")")

pushd "$PROGDIR/rpmbuild/SOURCES"
if [[ ! -f hello-2.10.tar.gz ]]; then
	wget http://ftp.gnu.org/gnu/hello/hello-2.10.tar.gz
fi
popd

rpmbuild --define="_topdir $PROGDIR/rpmbuild" -ba "$PROGDIR/hello.spec"
