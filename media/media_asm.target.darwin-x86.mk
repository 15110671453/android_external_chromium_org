# This file is generated by gyp; do not edit.

include $(CLEAR_VARS)

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := media_media_asm_gyp
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_TARGET_ARCH := $(TARGET_$(GYP_VAR_PREFIX)ARCH)
gyp_intermediate_dir := $(call local-intermediates-dir,,$(GYP_VAR_PREFIX))
gyp_shared_intermediate_dir := $(call intermediates-dir-for,GYP,shared,,,$(GYP_VAR_PREFIX))

# Make sure our deps are built first.
GYP_TARGET_DEPENDENCIES := \
	$(gyp_shared_intermediate_dir)/yasm


### Generated for rule "media_media_gyp_media_asm_target_assemble":
# "{'inputs': ['$(gyp_shared_intermediate_dir)/yasm', '../third_party/x86inc/x86inc.asm', 'base/simd/convert_rgb_to_yuv_ssse3.inc', 'base/simd/convert_yuv_to_rgb_mmx.inc', 'base/simd/convert_yuva_to_argb_mmx.inc', 'base/simd/linear_scale_yuv_to_rgb_mmx.inc', 'base/simd/media_export.asm', 'base/simd/scale_yuv_to_rgb_mmx.inc'], 'extension': 'asm', 'process_outputs_as_sources': '1', 'outputs': ['$(gyp_shared_intermediate_dir)/media/%(INPUT_ROOT)s.o'], 'rule_name': 'assemble', 'rule_sources': ['base/simd/convert_rgb_to_yuv_ssse3.asm', 'base/simd/convert_yuv_to_rgb_mmx.asm', 'base/simd/convert_yuv_to_rgb_sse.asm', 'base/simd/convert_yuva_to_argb_mmx.asm', 'base/simd/empty_register_state_mmx.asm', 'base/simd/linear_scale_yuv_to_rgb_mmx.asm', 'base/simd/linear_scale_yuv_to_rgb_sse.asm', 'base/simd/scale_yuv_to_rgb_mmx.asm', 'base/simd/scale_yuv_to_rgb_sse.asm'], 'action': ['$(gyp_shared_intermediate_dir)/yasm', '-DCHROMIUM', '-I..', '-felf32', '-m', 'x86', '-DARCH_X86_32', '-DELF', '-o', '$(gyp_shared_intermediate_dir)/media/%(INPUT_ROOT)s.o', '$(RULE_SOURCES)'], 'message': 'Compile assembly $(RULE_SOURCES)'}":
$(gyp_shared_intermediate_dir)/media/convert_rgb_to_yuv_ssse3.o: gyp_local_path := $(LOCAL_PATH)
$(gyp_shared_intermediate_dir)/media/convert_rgb_to_yuv_ssse3.o: gyp_var_prefix := $(GYP_VAR_PREFIX)
$(gyp_shared_intermediate_dir)/media/convert_rgb_to_yuv_ssse3.o: gyp_intermediate_dir := $(abspath $(gyp_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/convert_rgb_to_yuv_ssse3.o: gyp_shared_intermediate_dir := $(abspath $(gyp_shared_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/convert_rgb_to_yuv_ssse3.o: export PATH := $(subst $(ANDROID_BUILD_PATHS),,$(PATH))
$(gyp_shared_intermediate_dir)/media/convert_rgb_to_yuv_ssse3.o: $(LOCAL_PATH)/media/base/simd/convert_rgb_to_yuv_ssse3.asm $(gyp_shared_intermediate_dir)/yasm $(LOCAL_PATH)/third_party/x86inc/x86inc.asm $(LOCAL_PATH)/media/base/simd/convert_rgb_to_yuv_ssse3.inc $(LOCAL_PATH)/media/base/simd/convert_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/convert_yuva_to_argb_mmx.inc $(LOCAL_PATH)/media/base/simd/linear_scale_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/media_export.asm $(LOCAL_PATH)/media/base/simd/scale_yuv_to_rgb_mmx.inc $(GYP_TARGET_DEPENDENCIES)
	mkdir -p $(gyp_shared_intermediate_dir)/media; cd $(gyp_local_path)/media; "$(gyp_shared_intermediate_dir)/yasm" -DCHROMIUM -I.. -felf32 -m x86 -DARCH_X86_32 -DELF -o "$(gyp_shared_intermediate_dir)/media/convert_rgb_to_yuv_ssse3.o" base/simd/convert_rgb_to_yuv_ssse3.asm


$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_mmx.o: gyp_local_path := $(LOCAL_PATH)
$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_mmx.o: gyp_var_prefix := $(GYP_VAR_PREFIX)
$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_mmx.o: gyp_intermediate_dir := $(abspath $(gyp_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_mmx.o: gyp_shared_intermediate_dir := $(abspath $(gyp_shared_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_mmx.o: export PATH := $(subst $(ANDROID_BUILD_PATHS),,$(PATH))
$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_mmx.o: $(LOCAL_PATH)/media/base/simd/convert_yuv_to_rgb_mmx.asm $(gyp_shared_intermediate_dir)/yasm $(LOCAL_PATH)/third_party/x86inc/x86inc.asm $(LOCAL_PATH)/media/base/simd/convert_rgb_to_yuv_ssse3.inc $(LOCAL_PATH)/media/base/simd/convert_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/convert_yuva_to_argb_mmx.inc $(LOCAL_PATH)/media/base/simd/linear_scale_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/media_export.asm $(LOCAL_PATH)/media/base/simd/scale_yuv_to_rgb_mmx.inc $(GYP_TARGET_DEPENDENCIES)
	mkdir -p $(gyp_shared_intermediate_dir)/media; cd $(gyp_local_path)/media; "$(gyp_shared_intermediate_dir)/yasm" -DCHROMIUM -I.. -felf32 -m x86 -DARCH_X86_32 -DELF -o "$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_mmx.o" base/simd/convert_yuv_to_rgb_mmx.asm


$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_sse.o: gyp_local_path := $(LOCAL_PATH)
$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_sse.o: gyp_var_prefix := $(GYP_VAR_PREFIX)
$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_sse.o: gyp_intermediate_dir := $(abspath $(gyp_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_sse.o: gyp_shared_intermediate_dir := $(abspath $(gyp_shared_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_sse.o: export PATH := $(subst $(ANDROID_BUILD_PATHS),,$(PATH))
$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_sse.o: $(LOCAL_PATH)/media/base/simd/convert_yuv_to_rgb_sse.asm $(gyp_shared_intermediate_dir)/yasm $(LOCAL_PATH)/third_party/x86inc/x86inc.asm $(LOCAL_PATH)/media/base/simd/convert_rgb_to_yuv_ssse3.inc $(LOCAL_PATH)/media/base/simd/convert_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/convert_yuva_to_argb_mmx.inc $(LOCAL_PATH)/media/base/simd/linear_scale_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/media_export.asm $(LOCAL_PATH)/media/base/simd/scale_yuv_to_rgb_mmx.inc $(GYP_TARGET_DEPENDENCIES)
	mkdir -p $(gyp_shared_intermediate_dir)/media; cd $(gyp_local_path)/media; "$(gyp_shared_intermediate_dir)/yasm" -DCHROMIUM -I.. -felf32 -m x86 -DARCH_X86_32 -DELF -o "$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_sse.o" base/simd/convert_yuv_to_rgb_sse.asm


$(gyp_shared_intermediate_dir)/media/convert_yuva_to_argb_mmx.o: gyp_local_path := $(LOCAL_PATH)
$(gyp_shared_intermediate_dir)/media/convert_yuva_to_argb_mmx.o: gyp_var_prefix := $(GYP_VAR_PREFIX)
$(gyp_shared_intermediate_dir)/media/convert_yuva_to_argb_mmx.o: gyp_intermediate_dir := $(abspath $(gyp_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/convert_yuva_to_argb_mmx.o: gyp_shared_intermediate_dir := $(abspath $(gyp_shared_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/convert_yuva_to_argb_mmx.o: export PATH := $(subst $(ANDROID_BUILD_PATHS),,$(PATH))
$(gyp_shared_intermediate_dir)/media/convert_yuva_to_argb_mmx.o: $(LOCAL_PATH)/media/base/simd/convert_yuva_to_argb_mmx.asm $(gyp_shared_intermediate_dir)/yasm $(LOCAL_PATH)/third_party/x86inc/x86inc.asm $(LOCAL_PATH)/media/base/simd/convert_rgb_to_yuv_ssse3.inc $(LOCAL_PATH)/media/base/simd/convert_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/convert_yuva_to_argb_mmx.inc $(LOCAL_PATH)/media/base/simd/linear_scale_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/media_export.asm $(LOCAL_PATH)/media/base/simd/scale_yuv_to_rgb_mmx.inc $(GYP_TARGET_DEPENDENCIES)
	mkdir -p $(gyp_shared_intermediate_dir)/media; cd $(gyp_local_path)/media; "$(gyp_shared_intermediate_dir)/yasm" -DCHROMIUM -I.. -felf32 -m x86 -DARCH_X86_32 -DELF -o "$(gyp_shared_intermediate_dir)/media/convert_yuva_to_argb_mmx.o" base/simd/convert_yuva_to_argb_mmx.asm


$(gyp_shared_intermediate_dir)/media/empty_register_state_mmx.o: gyp_local_path := $(LOCAL_PATH)
$(gyp_shared_intermediate_dir)/media/empty_register_state_mmx.o: gyp_var_prefix := $(GYP_VAR_PREFIX)
$(gyp_shared_intermediate_dir)/media/empty_register_state_mmx.o: gyp_intermediate_dir := $(abspath $(gyp_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/empty_register_state_mmx.o: gyp_shared_intermediate_dir := $(abspath $(gyp_shared_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/empty_register_state_mmx.o: export PATH := $(subst $(ANDROID_BUILD_PATHS),,$(PATH))
$(gyp_shared_intermediate_dir)/media/empty_register_state_mmx.o: $(LOCAL_PATH)/media/base/simd/empty_register_state_mmx.asm $(gyp_shared_intermediate_dir)/yasm $(LOCAL_PATH)/third_party/x86inc/x86inc.asm $(LOCAL_PATH)/media/base/simd/convert_rgb_to_yuv_ssse3.inc $(LOCAL_PATH)/media/base/simd/convert_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/convert_yuva_to_argb_mmx.inc $(LOCAL_PATH)/media/base/simd/linear_scale_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/media_export.asm $(LOCAL_PATH)/media/base/simd/scale_yuv_to_rgb_mmx.inc $(GYP_TARGET_DEPENDENCIES)
	mkdir -p $(gyp_shared_intermediate_dir)/media; cd $(gyp_local_path)/media; "$(gyp_shared_intermediate_dir)/yasm" -DCHROMIUM -I.. -felf32 -m x86 -DARCH_X86_32 -DELF -o "$(gyp_shared_intermediate_dir)/media/empty_register_state_mmx.o" base/simd/empty_register_state_mmx.asm


$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_mmx.o: gyp_local_path := $(LOCAL_PATH)
$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_mmx.o: gyp_var_prefix := $(GYP_VAR_PREFIX)
$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_mmx.o: gyp_intermediate_dir := $(abspath $(gyp_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_mmx.o: gyp_shared_intermediate_dir := $(abspath $(gyp_shared_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_mmx.o: export PATH := $(subst $(ANDROID_BUILD_PATHS),,$(PATH))
$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_mmx.o: $(LOCAL_PATH)/media/base/simd/linear_scale_yuv_to_rgb_mmx.asm $(gyp_shared_intermediate_dir)/yasm $(LOCAL_PATH)/third_party/x86inc/x86inc.asm $(LOCAL_PATH)/media/base/simd/convert_rgb_to_yuv_ssse3.inc $(LOCAL_PATH)/media/base/simd/convert_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/convert_yuva_to_argb_mmx.inc $(LOCAL_PATH)/media/base/simd/linear_scale_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/media_export.asm $(LOCAL_PATH)/media/base/simd/scale_yuv_to_rgb_mmx.inc $(GYP_TARGET_DEPENDENCIES)
	mkdir -p $(gyp_shared_intermediate_dir)/media; cd $(gyp_local_path)/media; "$(gyp_shared_intermediate_dir)/yasm" -DCHROMIUM -I.. -felf32 -m x86 -DARCH_X86_32 -DELF -o "$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_mmx.o" base/simd/linear_scale_yuv_to_rgb_mmx.asm


$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_sse.o: gyp_local_path := $(LOCAL_PATH)
$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_sse.o: gyp_var_prefix := $(GYP_VAR_PREFIX)
$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_sse.o: gyp_intermediate_dir := $(abspath $(gyp_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_sse.o: gyp_shared_intermediate_dir := $(abspath $(gyp_shared_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_sse.o: export PATH := $(subst $(ANDROID_BUILD_PATHS),,$(PATH))
$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_sse.o: $(LOCAL_PATH)/media/base/simd/linear_scale_yuv_to_rgb_sse.asm $(gyp_shared_intermediate_dir)/yasm $(LOCAL_PATH)/third_party/x86inc/x86inc.asm $(LOCAL_PATH)/media/base/simd/convert_rgb_to_yuv_ssse3.inc $(LOCAL_PATH)/media/base/simd/convert_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/convert_yuva_to_argb_mmx.inc $(LOCAL_PATH)/media/base/simd/linear_scale_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/media_export.asm $(LOCAL_PATH)/media/base/simd/scale_yuv_to_rgb_mmx.inc $(GYP_TARGET_DEPENDENCIES)
	mkdir -p $(gyp_shared_intermediate_dir)/media; cd $(gyp_local_path)/media; "$(gyp_shared_intermediate_dir)/yasm" -DCHROMIUM -I.. -felf32 -m x86 -DARCH_X86_32 -DELF -o "$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_sse.o" base/simd/linear_scale_yuv_to_rgb_sse.asm


$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_mmx.o: gyp_local_path := $(LOCAL_PATH)
$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_mmx.o: gyp_var_prefix := $(GYP_VAR_PREFIX)
$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_mmx.o: gyp_intermediate_dir := $(abspath $(gyp_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_mmx.o: gyp_shared_intermediate_dir := $(abspath $(gyp_shared_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_mmx.o: export PATH := $(subst $(ANDROID_BUILD_PATHS),,$(PATH))
$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_mmx.o: $(LOCAL_PATH)/media/base/simd/scale_yuv_to_rgb_mmx.asm $(gyp_shared_intermediate_dir)/yasm $(LOCAL_PATH)/third_party/x86inc/x86inc.asm $(LOCAL_PATH)/media/base/simd/convert_rgb_to_yuv_ssse3.inc $(LOCAL_PATH)/media/base/simd/convert_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/convert_yuva_to_argb_mmx.inc $(LOCAL_PATH)/media/base/simd/linear_scale_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/media_export.asm $(LOCAL_PATH)/media/base/simd/scale_yuv_to_rgb_mmx.inc $(GYP_TARGET_DEPENDENCIES)
	mkdir -p $(gyp_shared_intermediate_dir)/media; cd $(gyp_local_path)/media; "$(gyp_shared_intermediate_dir)/yasm" -DCHROMIUM -I.. -felf32 -m x86 -DARCH_X86_32 -DELF -o "$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_mmx.o" base/simd/scale_yuv_to_rgb_mmx.asm


$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_sse.o: gyp_local_path := $(LOCAL_PATH)
$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_sse.o: gyp_var_prefix := $(GYP_VAR_PREFIX)
$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_sse.o: gyp_intermediate_dir := $(abspath $(gyp_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_sse.o: gyp_shared_intermediate_dir := $(abspath $(gyp_shared_intermediate_dir))
$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_sse.o: export PATH := $(subst $(ANDROID_BUILD_PATHS),,$(PATH))
$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_sse.o: $(LOCAL_PATH)/media/base/simd/scale_yuv_to_rgb_sse.asm $(gyp_shared_intermediate_dir)/yasm $(LOCAL_PATH)/third_party/x86inc/x86inc.asm $(LOCAL_PATH)/media/base/simd/convert_rgb_to_yuv_ssse3.inc $(LOCAL_PATH)/media/base/simd/convert_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/convert_yuva_to_argb_mmx.inc $(LOCAL_PATH)/media/base/simd/linear_scale_yuv_to_rgb_mmx.inc $(LOCAL_PATH)/media/base/simd/media_export.asm $(LOCAL_PATH)/media/base/simd/scale_yuv_to_rgb_mmx.inc $(GYP_TARGET_DEPENDENCIES)
	mkdir -p $(gyp_shared_intermediate_dir)/media; cd $(gyp_local_path)/media; "$(gyp_shared_intermediate_dir)/yasm" -DCHROMIUM -I.. -felf32 -m x86 -DARCH_X86_32 -DELF -o "$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_sse.o" base/simd/scale_yuv_to_rgb_sse.asm



GYP_GENERATED_OUTPUTS := \
	$(gyp_shared_intermediate_dir)/media/convert_rgb_to_yuv_ssse3.o \
	$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_mmx.o \
	$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_sse.o \
	$(gyp_shared_intermediate_dir)/media/convert_yuva_to_argb_mmx.o \
	$(gyp_shared_intermediate_dir)/media/empty_register_state_mmx.o \
	$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_mmx.o \
	$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_sse.o \
	$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_mmx.o \
	$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_sse.o

# Make sure our deps and generated files are built first.
LOCAL_ADDITIONAL_DEPENDENCIES := $(GYP_TARGET_DEPENDENCIES) $(GYP_GENERATED_OUTPUTS)

LOCAL_GENERATED_SOURCES := \
	$(gyp_shared_intermediate_dir)/media/convert_rgb_to_yuv_ssse3.o \
	$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_mmx.o \
	$(gyp_shared_intermediate_dir)/media/convert_yuv_to_rgb_sse.o \
	$(gyp_shared_intermediate_dir)/media/convert_yuva_to_argb_mmx.o \
	$(gyp_shared_intermediate_dir)/media/empty_register_state_mmx.o \
	$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_mmx.o \
	$(gyp_shared_intermediate_dir)/media/linear_scale_yuv_to_rgb_sse.o \
	$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_mmx.o \
	$(gyp_shared_intermediate_dir)/media/scale_yuv_to_rgb_sse.o

GYP_COPIED_SOURCE_ORIGIN_DIRS :=

LOCAL_SRC_FILES :=


# Flags passed to both C and C++ files.
MY_CFLAGS_Debug := \
	--param=ssp-buffer-size=4 \
	-Werror \
	-fno-exceptions \
	-fno-strict-aliasing \
	-Wall \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-fvisibility=hidden \
	-pipe \
	-fPIC \
	-Wno-unused-local-typedefs \
	-msse2 \
	-mfpmath=sse \
	-mmmx \
	-m32 \
	-ffunction-sections \
	-funwind-tables \
	-g \
	-fno-short-enums \
	-finline-limit=64 \
	-Wa,--noexecstack \
	-U_FORTIFY_SOURCE \
	-Wno-extra \
	-Wno-ignored-qualifiers \
	-Wno-type-limits \
	-Wno-unused-but-set-variable \
	-fno-stack-protector \
	-Os \
	-g \
	-fdata-sections \
	-ffunction-sections \
	-fomit-frame-pointer \
	-funwind-tables

MY_DEFS_Debug := \
	'-DV8_DEPRECATION_WARNINGS' \
	'-DBLINK_SCALE_FILTERS_AT_RECORD_TIME' \
	'-D_FILE_OFFSET_BITS=64' \
	'-DNO_TCMALLOC' \
	'-DDISABLE_NACL' \
	'-DCHROMIUM_BUILD' \
	'-DUSE_LIBJPEG_TURBO=1' \
	'-DENABLE_WEBRTC=1' \
	'-DUSE_PROPRIETARY_CODECS' \
	'-DENABLE_BROWSER_CDMS' \
	'-DENABLE_CONFIGURATION_POLICY' \
	'-DDISCARDABLE_MEMORY_ALWAYS_SUPPORTED_NATIVELY' \
	'-DSYSTEM_NATIVELY_SIGNALS_MEMORY_PRESSURE' \
	'-DENABLE_EGLIMAGE=1' \
	'-DCLD_VERSION=1' \
	'-DENABLE_PRINTING=1' \
	'-DENABLE_MANAGED_USERS=1' \
	'-DDATA_REDUCTION_FALLBACK_HOST="http://compress.googlezip.net:80/"' \
	'-DDATA_REDUCTION_DEV_HOST="http://proxy-dev.googlezip.net:80/"' \
	'-DSPDY_PROXY_AUTH_ORIGIN="https://proxy.googlezip.net:443/"' \
	'-DDATA_REDUCTION_PROXY_PROBE_URL="http://check.googlezip.net/connect"' \
	'-DDATA_REDUCTION_PROXY_WARMUP_URL="http://www.gstatic.com/generate_204"' \
	'-DVIDEO_HOLE=1' \
	'-DUSE_OPENSSL=1' \
	'-DUSE_OPENSSL_CERTS=1' \
	'-D__STDC_CONSTANT_MACROS' \
	'-D__STDC_FORMAT_MACROS' \
	'-DANDROID' \
	'-D__GNU_SOURCE=1' \
	'-DUSE_STLPORT=1' \
	'-D_STLP_USE_PTR_SPECIALIZATIONS=1' \
	'-DCHROME_BUILD_ID=""' \
	'-DDYNAMIC_ANNOTATIONS_ENABLED=1' \
	'-DWTF_USE_DYNAMIC_ANNOTATIONS=1' \
	'-D_DEBUG'


# Include paths placed before CFLAGS/CPPFLAGS
LOCAL_C_INCLUDES_Debug := \
	$(gyp_shared_intermediate_dir) \
	$(PWD)/frameworks/wilhelm/include \
	$(PWD)/bionic \
	$(PWD)/external/stlport/stlport


# Flags passed to only C++ (and not C) files.
LOCAL_CPPFLAGS_Debug := \
	-fno-rtti \
	-fno-threadsafe-statics \
	-fvisibility-inlines-hidden \
	-Wsign-compare \
	-Wno-non-virtual-dtor \
	-Wno-sign-promo


LOCAL_FDO_SUPPORT_Debug := false

# Flags passed to both C and C++ files.
MY_CFLAGS_Release := \
	--param=ssp-buffer-size=4 \
	-Werror \
	-fno-exceptions \
	-fno-strict-aliasing \
	-Wall \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-fvisibility=hidden \
	-pipe \
	-fPIC \
	-Wno-unused-local-typedefs \
	-msse2 \
	-mfpmath=sse \
	-mmmx \
	-m32 \
	-ffunction-sections \
	-funwind-tables \
	-g \
	-fno-short-enums \
	-finline-limit=64 \
	-Wa,--noexecstack \
	-U_FORTIFY_SOURCE \
	-Wno-extra \
	-Wno-ignored-qualifiers \
	-Wno-type-limits \
	-Wno-unused-but-set-variable \
	-fno-stack-protector \
	-Os \
	-fno-ident \
	-fdata-sections \
	-ffunction-sections \
	-fomit-frame-pointer \
	-funwind-tables

MY_DEFS_Release := \
	'-DV8_DEPRECATION_WARNINGS' \
	'-DBLINK_SCALE_FILTERS_AT_RECORD_TIME' \
	'-D_FILE_OFFSET_BITS=64' \
	'-DNO_TCMALLOC' \
	'-DDISABLE_NACL' \
	'-DCHROMIUM_BUILD' \
	'-DUSE_LIBJPEG_TURBO=1' \
	'-DENABLE_WEBRTC=1' \
	'-DUSE_PROPRIETARY_CODECS' \
	'-DENABLE_BROWSER_CDMS' \
	'-DENABLE_CONFIGURATION_POLICY' \
	'-DDISCARDABLE_MEMORY_ALWAYS_SUPPORTED_NATIVELY' \
	'-DSYSTEM_NATIVELY_SIGNALS_MEMORY_PRESSURE' \
	'-DENABLE_EGLIMAGE=1' \
	'-DCLD_VERSION=1' \
	'-DENABLE_PRINTING=1' \
	'-DENABLE_MANAGED_USERS=1' \
	'-DDATA_REDUCTION_FALLBACK_HOST="http://compress.googlezip.net:80/"' \
	'-DDATA_REDUCTION_DEV_HOST="http://proxy-dev.googlezip.net:80/"' \
	'-DSPDY_PROXY_AUTH_ORIGIN="https://proxy.googlezip.net:443/"' \
	'-DDATA_REDUCTION_PROXY_PROBE_URL="http://check.googlezip.net/connect"' \
	'-DDATA_REDUCTION_PROXY_WARMUP_URL="http://www.gstatic.com/generate_204"' \
	'-DVIDEO_HOLE=1' \
	'-DUSE_OPENSSL=1' \
	'-DUSE_OPENSSL_CERTS=1' \
	'-D__STDC_CONSTANT_MACROS' \
	'-D__STDC_FORMAT_MACROS' \
	'-DANDROID' \
	'-D__GNU_SOURCE=1' \
	'-DUSE_STLPORT=1' \
	'-D_STLP_USE_PTR_SPECIALIZATIONS=1' \
	'-DCHROME_BUILD_ID=""' \
	'-DNDEBUG' \
	'-DNVALGRIND' \
	'-DDYNAMIC_ANNOTATIONS_ENABLED=0' \
	'-D_FORTIFY_SOURCE=2'


# Include paths placed before CFLAGS/CPPFLAGS
LOCAL_C_INCLUDES_Release := \
	$(gyp_shared_intermediate_dir) \
	$(PWD)/frameworks/wilhelm/include \
	$(PWD)/bionic \
	$(PWD)/external/stlport/stlport


# Flags passed to only C++ (and not C) files.
LOCAL_CPPFLAGS_Release := \
	-fno-rtti \
	-fno-threadsafe-statics \
	-fvisibility-inlines-hidden \
	-Wsign-compare \
	-Wno-non-virtual-dtor \
	-Wno-sign-promo


LOCAL_FDO_SUPPORT_Release := false

LOCAL_CFLAGS := $(MY_CFLAGS_$(GYP_CONFIGURATION)) $(MY_DEFS_$(GYP_CONFIGURATION))
LOCAL_FDO_SUPPORT := $(LOCAL_FDO_SUPPORT_$(GYP_CONFIGURATION))
LOCAL_C_INCLUDES := $(GYP_COPIED_SOURCE_ORIGIN_DIRS) $(LOCAL_C_INCLUDES_$(GYP_CONFIGURATION))
LOCAL_CPPFLAGS := $(LOCAL_CPPFLAGS_$(GYP_CONFIGURATION))
LOCAL_ASFLAGS := $(LOCAL_CFLAGS)
### Rules for final target.

LOCAL_LDFLAGS_Debug := \
	-Wl,-z,now \
	-Wl,-z,relro \
	-Wl,--fatal-warnings \
	-Wl,-z,noexecstack \
	-fPIC \
	-m32 \
	-fuse-ld=gold \
	-nostdlib \
	-Wl,--no-undefined \
	-Wl,--exclude-libs=ALL \
	-Wl,--warn-shared-textrel \
	-Wl,-O1 \
	-Wl,--as-needed


LOCAL_LDFLAGS_Release := \
	-Wl,-z,now \
	-Wl,-z,relro \
	-Wl,--fatal-warnings \
	-Wl,-z,noexecstack \
	-fPIC \
	-m32 \
	-fuse-ld=gold \
	-nostdlib \
	-Wl,--no-undefined \
	-Wl,--exclude-libs=ALL \
	-Wl,-O1 \
	-Wl,--as-needed \
	-Wl,--gc-sections \
	-Wl,--warn-shared-textrel


LOCAL_LDFLAGS := $(LOCAL_LDFLAGS_$(GYP_CONFIGURATION))

LOCAL_STATIC_LIBRARIES :=

# Enable grouping to fix circular references
LOCAL_GROUP_STATIC_LIBRARIES := true

LOCAL_SHARED_LIBRARIES := \
	libstlport \
	libdl

# Add target alias to "gyp_all_modules" target.
.PHONY: gyp_all_modules
gyp_all_modules: media_media_asm_gyp

# Alias gyp target name.
.PHONY: media_asm
media_asm: media_media_asm_gyp

include $(BUILD_STATIC_LIBRARY)
