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
mkdir build-win-x86
cd build-win-x86
../configure --target=x86-win32-vs14
make
cp vp8_rtcd.h ../../config/win/ia32/vp8_rtcd.h
cp vp9_rtcd.h ../../config/win/ia32/vp9_rtcd.h
cp vpx_config.asm ../../config/win/ia32/vpx_config.asm
cp vpx_config.c ../../config/win/ia32/vpx_config.c
cp vpx_config.h ../../config/win/ia32/vpx_config.h
cp vpx_dsp_rtcd.h ../../config/win/ia32/vpx_dsp_rtcd.h
cp vpx_scale_rtcd.h ../../config/win/ia32/vpx_scale_rtcd.h