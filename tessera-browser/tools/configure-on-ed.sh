#!/bin/bash
# Configure patched /root/Dev/webkit-nogl in a new aarch64 build directory.
set -euo pipefail
test "$(uname -s)" = Linux
test -d /root/Dev/webkit-nogl/Source
here=$(cd "$(dirname "$0")" && pwd)
out=${1:?Pass a NEW absolute directory under /root/Dev/webkit-build}
prefix=${2:?Pass the private dependency prefix containing lib/}
case "$out" in /root/Dev/webkit-build/*) ;; *) exit 2 ;; esac
test ! -e "$out"
for library in libatk-1.0.so libatk-bridge-2.0.so libatspi.so.0 libgstallocators-1.0.so; do
    test -f "$prefix/lib/$library"
done
cmake -S /root/Dev/webkit-nogl -B "$out" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$here/aarch64-tessera.cmake" \
    -DCMAKE_C_COMPILER_LAUNCHER="$here/distcc-launcher-on-ed.sh" \
    -DCMAKE_CXX_COMPILER_LAUNCHER="$here/distcc-launcher-on-ed.sh" \
    -DATK_LIBRARY="$prefix/lib/libatk-1.0.so" \
    -DATKBridge_LIBRARY="$prefix/lib/libatk-bridge-2.0.so" \
    -DGSTREAMER_ALLOCATORS_LIBRARIES="$prefix/lib/libgstallocators-1.0.so" \
    -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld -Wl,-z,max-page-size=65536 -Wl,-rpath-link,$prefix/lib" \
    -DPORT=WPE -DCMAKE_BUILD_TYPE=Release -DDEVELOPER_MODE=ON \
    -DENABLE_DOCUMENTATION=OFF -DENABLE_INTROSPECTION=OFF -DENABLE_API_TESTS=OFF \
    -DENABLE_MINIBROWSER=OFF -DENABLE_WPE_LEGACY_API=OFF -DUSE_LIBBACKTRACE=OFF \
    -DENABLE_WPE_QT_API=OFF -DENABLE_BUBBLEWRAP_SANDBOX=OFF -DENABLE_WEBDRIVER=OFF \
    -DUSE_VULKAN=ON -DTESSERA_NO_GL=ON -DENABLE_WEBGL=OFF \
    -DENABLE_GPU_PROCESS=OFF -DUSE_GBM=OFF -DUSE_LIBDRM=OFF \
    -DENABLE_WPE_PLATFORM_DRM=OFF -DENABLE_WPE_PLATFORM_WAYLAND=OFF \
    -DENABLE_WPE_PLATFORM_HEADLESS=OFF -DUSE_GSTREAMER_GL=OFF
