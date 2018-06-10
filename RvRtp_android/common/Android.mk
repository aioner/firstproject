LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := rvcommon

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../inc/common \

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CFLAGS += -fexceptions -fvisibility=hidden -g \
	-DLINUX -D_GNU_SOURCE -DRV_CFLAG_EPOLL -D_RV_LINUX_API_HIDDEN -DRV_ANDROID \

SRC_FILES := $(wildcard $(LOCAL_PATH)/*.c)
SRC_FILES := $(SRC_FILES:$(LOCAL_PATH)/%=%)

LOCAL_SRC_FILES +=  $(SRC_FILES) \

include $(BUILD_STATIC_LIBRARY)
