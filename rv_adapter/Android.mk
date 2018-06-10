LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := rv_adapter

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../RvRtp_android/inc/common \
	$(LOCAL_PATH)/../RvRtp_android/inc/rtprtcp \
	$(LOCAL_PATH)/../boost_1.56 \
	$(LOCAL_PATH)/.. \

	
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CPPFLAGS += -fexceptions -fvisibility=hidden \
	-DLINUX -D_GNU_SOURCE -DRV_ANDROID -D_ANDROID -g\

SRC_FILES := $(wildcard $(LOCAL_PATH)/*.cpp)
SRC_FILES := $(SRC_FILES:$(LOCAL_PATH)/%=%)
 
LOCAL_SRC_FILES += $(SRC_FILES) \

LOCAL_LDLIBS := -llog

LOCAL_STATIC_LIBRARIES := rv32rtp rvcommon tghelper boost_armeabi

include $(BUILD_SHARED_LIBRARY)
