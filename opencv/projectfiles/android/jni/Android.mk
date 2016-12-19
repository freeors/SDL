LOCAL_PATH := $(call my-dir)/../../..

include $(CLEAR_VARS)

LOCAL_MODULE := opencv

# LOCAL_CFLAGS := -std=c11

LOCAL_CFLAGS := -mfpu=neon -D__OPENCV_BUILD=1 -DWITH_CUDA=OFF
# LOCAL_CPP_EXTENSION := .cxx .cpp .cc


LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/. \
	$(LOCAL_PATH)/3rdparty/carotene\include \
	$(LOCAL_PATH)/3rdparty/zlib \
	$(LOCAL_PATH)/autogen \
	$(LOCAL_PATH)/modules/calib3d/include \
	$(LOCAL_PATH)/modules/core/include \
	$(LOCAL_PATH)/modules/core/src \
	$(LOCAL_PATH)/modules/features2d/include \
	$(LOCAL_PATH)/modules/flann/include \
	$(LOCAL_PATH)/modules/imgproc/include \
	$(LOCAL_PATH)/modules/imgproc/src \
	$(LOCAL_PATH)/modules/ml/include \
	$(LOCAL_PATH)/modules/objdetect/include \
	$(LOCAL_PATH)/modules/photo/include \
	$(LOCAL_PATH)/modules/shape/include \
	$(LOCAL_PATH)/modules/stitching/include \
	$(LOCAL_PATH)/modules/superres/include \
	$(LOCAL_PATH)/modules/video/include \
	$(LOCAL_PATH)/modules/videostab/include

LOCAL_PATH := $(LOCAL_PATH)/modules

CC_ALL_SRCS := \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/../3rdparty/carotene/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/calib3d/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/core/src/opencl/runtime/*.cpp) \
	$(wildcard $(LOCAL_PATH)/core/src/utils/*.cpp) \
	$(wildcard $(LOCAL_PATH)/core/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/features2d/src/kaze/*.cpp) \
	$(wildcard $(LOCAL_PATH)/features2d/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/flann/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/imgproc/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/ml/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/objdetect/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/photo/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/shape/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/stitching/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/superres/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/video/src/*.cpp) \
	$(wildcard $(LOCAL_PATH)/videostab/src/*.cpp))

CC_EXCLUDE_SRCS := \
	core/src/convert.avx2.cpp \
	core/src/convert.fp16.cpp \
	core/src/convert.sse4_1.cpp \
	features2d/src/fast.avx2.cpp \
	imgproc/src/corner.avx.cpp \
	imgproc/src/filter.avx2.cpp \
	imgproc/src/imgwarp.avx2.cpp \
	imgproc/src/imgwarp.sse4_1.cpp \
	imgproc/src/resize.avx2.cpp \
	imgproc/src/resize.sse4_1.cpp \
	imgproc/src/undistort.avx2.cpp \
	objdetect/src/haar.avx.cpp
	
LOCAL_SRC_FILES := $(filter-out $(CC_EXCLUDE_SRCS), $(CC_ALL_SRCS))

	
LOCAL_LDLIBS := -llog -lz


include $(BUILD_SHARED_LIBRARY)
