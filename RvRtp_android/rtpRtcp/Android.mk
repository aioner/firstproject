LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)

LOCAL_MODULE    := rv32rtp

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../inc/common \
	$(LOCAL_PATH)/../inc/rtprtcp \

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CFLAGS += -fexceptions -fvisibility=hidden -g\
	-DLINUX  -D_RV_LINUX_API_DEFAULT -DRV_ANDROID \

SRC_FILES := $(wildcard $(LOCAL_PATH)/*.c)
SRC_FILES := $(SRC_FILES:$(LOCAL_PATH)/%=%)

LOCAL_SRC_FILES += $(SRC_FILES) \

include $(BUILD_STATIC_LIBRARY)
