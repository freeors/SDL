LOCAL_PATH := $(call my-dir)/../../..

include $(CLEAR_VARS)

LOCAL_MODULE := protobuf

# LOCAL_CFLAGS := -std=c11
LOCAL_CFLAGS := -mfpu=neon -DHAVE_PTHREAD -DGOOGLE_PROTOBUF_DONT_USE_UNALIGNED

LOCAL_CPP_EXTENSION := .cxx .cpp .cc


LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../apps/external/protobuf/src

LOCAL_PATH := $(LOCAL_PATH)/../../apps/external/protobuf/src/google

LOCAL_SRC_FILES := \
	protobuf/arena.cc \
	protobuf/arenastring.cc \
	protobuf/extension_set.cc \
	protobuf/generated_message_util.cc \
	protobuf/io/coded_stream.cc \
	protobuf/io/zero_copy_stream.cc \
	protobuf/io/zero_copy_stream_impl_lite.cc \
	protobuf/message_lite.cc \
	protobuf/repeated_field.cc \
	protobuf/stubs/atomicops_internals_x86_gcc.cc \
	protobuf/stubs/atomicops_internals_x86_msvc.cc \
	protobuf/stubs/bytestream.cc \
	protobuf/stubs/common.cc \
	protobuf/stubs/int128.cc \
	protobuf/stubs/once.cc \
	protobuf/stubs/status.cc \
	protobuf/stubs/statusor.cc \
	protobuf/stubs/stringpiece.cc \
	protobuf/stubs/stringprintf.cc \
	protobuf/stubs/structurally_valid.cc \
	protobuf/stubs/strutil.cc \
	protobuf/stubs/time.cc \
	protobuf/wire_format_lite.cc \
	protobuf/any.cc \
    protobuf/any.pb.cc \
    protobuf/api.pb.cc \
    protobuf/compiler/importer.cc \
    protobuf/compiler/parser.cc \
    protobuf/descriptor.cc \
    protobuf/descriptor.pb.cc \
    protobuf/descriptor_database.cc \
    protobuf/duration.pb.cc \
    protobuf/dynamic_message.cc \
    protobuf/empty.pb.cc \
    protobuf/extension_set_heavy.cc \
    protobuf/field_mask.pb.cc \
    protobuf/generated_message_reflection.cc \
    protobuf/io/gzip_stream.cc \
    protobuf/io/printer.cc \
    protobuf/io/strtod.cc \
    protobuf/io/tokenizer.cc \
    protobuf/io/zero_copy_stream_impl.cc \
    protobuf/map_field.cc \
    protobuf/message.cc \
    protobuf/reflection_ops.cc \
    protobuf/service.cc \
    protobuf/source_context.pb.cc \
    protobuf/struct.pb.cc \
    protobuf/stubs/mathlimits.cc \
    protobuf/stubs/substitute.cc \
    protobuf/text_format.cc \
    protobuf/timestamp.pb.cc \
    protobuf/type.pb.cc \
    protobuf/unknown_field_set.cc \
    protobuf/util/delimited_message_util.cc \
    protobuf/util/field_comparator.cc \
    protobuf/util/field_mask_util.cc \
    protobuf/util/internal/datapiece.cc \
    protobuf/util/internal/default_value_objectwriter.cc \
    protobuf/util/internal/error_listener.cc \
    protobuf/util/internal/field_mask_utility.cc \
    protobuf/util/internal/json_escaping.cc \
    protobuf/util/internal/json_objectwriter.cc \
    protobuf/util/internal/json_stream_parser.cc \
    protobuf/util/internal/object_writer.cc \
    protobuf/util/internal/proto_writer.cc \
    protobuf/util/internal/protostream_objectsource.cc \
    protobuf/util/internal/protostream_objectwriter.cc \
    protobuf/util/internal/type_info.cc \
    protobuf/util/internal/type_info_test_helper.cc \
    protobuf/util/internal/utility.cc \
    protobuf/util/json_util.cc \
    protobuf/util/message_differencer.cc \
    protobuf/util/time_util.cc \
    protobuf/util/type_resolver_util.cc \
    protobuf/wire_format.cc \
    protobuf/wrappers.pb.cc
	
LOCAL_LDLIBS := -llog


include $(BUILD_SHARED_LIBRARY)
