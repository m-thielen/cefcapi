#!/bin/bash
#
# bash script that sets the build environment to build the windows based
# loader application on Mac.
# Uses the Mingw-w64 toolchain installed via Homebrew, see
# https://github.com/cosmo0920/homebrew-mingw_w64
#
# Author: Markus Thielen, mt@thiguten.de
#
MINGW_BASE_DIR=/usr/local/Cellar/mingw-w64
MINGW_VERSION=5.0.3
MINGW_ARCH_32BIT=i686-w64-mingw32
MINGW_ARCH_64BIT=x86_64-w64-mingw32

CEF32=cef/win32
CEF64=

TARGET=win32  # or win64

if [ $TARGET = "win32" ]; then
   TARGET_ARCH=$MINGW_ARCH_32BIT
   CEF=$CEF32
else
   TARGET_ARCH=$MINGW_ARCH_64BIT
   CEF=$CEF64
fi

export TARGET_ARCH
export CEF
export PATH=$MINGW_BASE_DIR/$MINGW_VERSION/bin:$PATH
export WINDOWS_TOOLDIR=$MINGW_BASE_DIR/$MINGW_VERSION

echo "Set Mac cross compile environment to Mingw-w64 in $MINGW_BASE_DIR"
