LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := rvsdp_rtspclient

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include/common \
	$(LOCAL_PATH)/../include/rtsp \

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CFLAGS += -fexceptions -fvisibility=hidden -g\
	-DLINUX -D_GNU_SOURCE -D_RV_LINUX_API_DEFAULT -DRV_ANDROID\


SRC_FILES := $(wildcard $(LOCAL_PATH)/*.c)
SRC_FILES := $(SRC_FILES:$(LOCAL_PATH)/%=%)

LOCAL_SRC_FILES +=  $(SRC_FILES) \

LOCAL_LDLIBS += -L$(SYSROOT)/usr/lib -llog -landroid \

include $(BUILD_STATIC_LIBRARY)
