#!/bin/bash
# Run only on ed. Builds in a new directory; installs no system packages.
set -euo pipefail
test "$(uname -s)" = Linux
test -x /root/.kira/toolchains/llvm/23.1.0/x86_64-linux-gnu/bin/clang
test -d /root/Dev/webkit-sysroot/aarch64
here=$(cd "$(dirname "$0")" && pwd)
out=${1:?Pass a NEW absolute directory under /root/Dev/webkit-build}
case "$out" in /root/Dev/webkit-build/*) ;; *) exit 2 ;; esac
test ! -e "$out"
mkdir -p "$out/downloads" "$out/prefix/lib"
cp "$here/aarch64-dependencies.ini" "$out/aarch64.ini"
cd "$out/downloads"
curl --fail --location --connect-timeout 10 --max-time 120 \
    -o at-spi2-core-2.56.2.tar.xz \
    https://download.gnome.org/sources/at-spi2-core/2.56/at-spi2-core-2.56.2.tar.xz
curl --fail --location --connect-timeout 10 --max-time 120 \
    -o gst-plugins-base-1.26.2.tar.xz \
    https://gstreamer.freedesktop.org/src/gst-plugins-base/gst-plugins-base-1.26.2.tar.xz
printf '%s\n' \
    'e1b1c9836a8947852f7440c32e23179234c76bd98cd9cc4001f376405f8b783b  at-spi2-core-2.56.2.tar.xz' \
    'f4b9fc0be852fe5f65401d18ae6218e4aea3ff7a3c9f8d265939b9c4704915f7  gst-plugins-base-1.26.2.tar.xz' | sha256sum --check
cd "$out"
tar -xf downloads/at-spi2-core-2.56.2.tar.xz
tar -xf downloads/gst-plugins-base-1.26.2.tar.xz
meson setup atspi-build at-spi2-core-2.56.2 --cross-file aarch64.ini \
    --prefix=/usr --libdir=lib/aarch64-linux-gnu --buildtype=release \
    -Dx11=disabled -Dgtk2_atk_adaptor=false -Dintrospection=disabled \
    -Ddocs=false -Duse_systemd=false -Ddbus_daemon=/usr/bin/dbus-daemon
ninja -C atspi-build -j2 atspi/libatspi.so.0.0.1 atk-adaptor/libatk-bridge-2.0.so.0.0.0
meson setup gst-base-build gst-plugins-base-1.26.2 --cross-file aarch64.ini \
    --prefix=/usr --libdir=lib/aarch64-linux-gnu --buildtype=release \
    -Dauto_features=disabled -Ddrm=disabled -Dgl=disabled -Dx11=disabled \
    -Dtests=disabled -Dexamples=disabled -Dintrospection=disabled \
    -Dtools=disabled -Ddoc=disabled
ninja -C gst-base-build -j2 gst-libs/gst/allocators/libgstallocators-1.0.so.0.2602.0
cp atspi-build/atspi/libatspi.so.0.0.1 prefix/lib/
cp atspi-build/atk-adaptor/libatk-bridge-2.0.so.0.0.0 prefix/lib/
cp atspi-build/atk/libatk-1.0.so.0.25611.1 prefix/lib/
cp gst-base-build/gst-libs/gst/allocators/libgstallocators-1.0.so.0.2602.0 prefix/lib/
for library in prefix/lib/*.so.*; do
    patchelf --set-rpath '$ORIGIN' "$library"
    soname=$(patchelf --print-soname "$library")
    ln -s "$(basename "$library")" "prefix/lib/$soname"
    ln -s "$(basename "$library")" "prefix/lib/${soname%%.so.*}.so"
done
echo "Private dependency prefix: $out/prefix/lib"
