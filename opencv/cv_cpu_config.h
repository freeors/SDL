// OpenCV CPU baseline features
#if defined(_WIN32)
#define CV_CPU_COMPILE_SSE 1
#define CV_CPU_BASELINE_COMPILE_SSE 1

#define CV_CPU_COMPILE_SSE2 1
#define CV_CPU_BASELINE_COMPILE_SSE2 1

#define CV_CPU_BASELINE_FEATURES 0 \
    , CV_CPU_SSE \
    , CV_CPU_SSE2 \


// OpenCV supported CPU dispatched features

#define CV_CPU_DISPATCH_COMPILE_SSE4_1 1
#define CV_CPU_DISPATCH_COMPILE_SSE4_2 1
#define CV_CPU_DISPATCH_COMPILE_FP16 1
#define CV_CPU_DISPATCH_COMPILE_AVX 1

#else

#define CV_CPU_COMPILE_NEON 1
#define CV_CPU_BASELINE_COMPILE_NEON 1

#define CV_CPU_COMPILE_NEON 1
#define CV_CPU_BASELINE_COMPILE_NEON 1

#define CV_CPU_BASELINE_FEATURES 0 \
    , CV_CPU_NEON \
    , CV_CPU_NEON \

#endif