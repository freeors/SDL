#!/bin/sh
##
##  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
##
##  Use of this source code is governed by a BSD-style license
##  that can be found in the LICENSE file in the root of the source
##  tree. An additional intellectual property rights grant can be found
##  in the file PATENTS.  All contributing project authors may
##  be found in the AUTHORS file in the root of the source tree.
##


cd ..
mkdir build-android-armv7
cd build-android-armv7
../configure --target=armv7-android-gcc --sdk-path=../../../../../../android-ndk-r14b --extra-cflags="-mfloat-abi=softfp -mfpu=neon" --disable-runtime-cpu-detect --disable-pic --disable-realtime-only

## ../configure --target=armv7-android-gcc --sdk-path=../android-ndk-r12b --extra-cflags="-mfloat-abi=softfp -mfpu=neon" --disable-webm-io --disable-runtime-cpu-detect --disable-libyuv --disable-examples

## --enable-webm-io
## --enable-libyuv
## --enable-shared
## --disable-static

make
cp vp8_rtcd.h ../../config/android/armv7/vp8_rtcd.h
cp vp9_rtcd.h ../../config/android/armv7/vp9_rtcd.h
cp vpx_config.asm ../../config/android/armv7/vpx_config.asm
cp vpx_config.c ../../config/android/armv7/vpx_config.c
cp vpx_config.h ../../config/android/armv7/vpx_config.h
cp vpx_dsp_rtcd.h ../../config/android/armv7/vpx_dsp_rtcd.h
cp vpx_scale_rtcd.h ../../config/android/armv7/vpx_scale_rtcd.h
