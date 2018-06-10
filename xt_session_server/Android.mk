LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := xt_session_server

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../boost_1.56 \
	$(LOCAL_PATH)/../xt_udp_session/udp_session_server \
	$(LOCAL_PATH)/../trans_common

	
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CPPFLAGS += -fexceptions -g -D_ANDROID

SRC_FILES := $(wildcard $(LOCAL_PATH)/*.cpp)
SRC_FILES := $(SRC_FILES:$(LOCAL_PATH)/%=%)


LOCAL_SRC_FILES += $(SRC_FILES) \

include $(BUILD_STATIC_LIBRARY)