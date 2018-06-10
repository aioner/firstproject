LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := rvrtsp_client

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include/common \
	$(LOCAL_PATH)/../include/rtsp \

	
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CFLAGS += -fexceptions -fvisibility=hidden -g \
	-DLINUX -D_GNU_SOURCE -D_RV_LINUX_API_DEFAULT -DRV_ANDROID \


SRC_FILES := $(wildcard $(LOCAL_PATH)/messages/*.c)
SRC_FILES := $(SRC_FILES:$(LOCAL_PATH)/%=%)

SRC_FILES1 := $(wildcard $(LOCAL_PATH)/transport/*.c)
SRC_FILES1 := $(SRC_FILES1:$(LOCAL_PATH)/%=%)

SRC_FILES2 := $(wildcard $(LOCAL_PATH)/utils/*.c)
SRC_FILES2 := $(SRC_FILES2:$(LOCAL_PATH)/%=%)


LOCAL_SRC_FILES +=  $(SRC_FILES) $(SRC_FILES1) $(SRC_FILES2) \


LOCAL_LDLIBS += -L$(SYSROOT)/usr/lib -llog -landroid \

include $(BUILD_STATIC_LIBRARY)

include $(LOCAL_PATH)/client/Android.mk
