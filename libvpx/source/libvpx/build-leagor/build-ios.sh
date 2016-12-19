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
mkdir build-ios-armv7
cd build-ios-armv7
../configure --target=armv7-darwin-gcc
make

# cd ..
# mkdir build-ios-armv7s
# cd build-ios-armv7s
# ../configure --target=armv7s-darwin-gcc
# make

cd ..
mkdir build-ios-arm64
cd build-ios-arm64
../configure --target=arm64-darwin-gcc
make

cd ..
# lipo -create build-ios-arm64/libvpx.a build-ios-armv7/libvpx.a  build-ios-armv7s/libvpx.a -output build-leagor/libvpx.a
lipo -create build-ios-arm64/libvpx.a build-ios-armv7/libvpx.a -output build-leagor/libvpx.a