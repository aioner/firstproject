LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := xt_router

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../trans_common \
	$(LOCAL_PATH)/../xt_media_client \
	$(LOCAL_PATH)/../boost_1.56 \
	$(LOCAL_PATH)/../xt_media_server \
	$(LOCAL_PATH)/../xt_sdp \

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CPPFLAGS += -fexceptions  -g -frtti \

SRC_FILES := $(wildcard $(LOCAL_PATH)/*.cpp)
SRC_FILES := $(SRC_FILES:$(LOCAL_PATH)/%=%)


LOCAL_SRC_FILES += $(SRC_FILES) \

LOCAL_LDLIBS := -llog

LOCAL_SHARED_LIBRARIES := xt_media_server xt_media_client

LOCAL_STATIC_LIBRARIES := xt_sdp boost_armeabi

include $(BUILD_SHARED_LIBRARY)
