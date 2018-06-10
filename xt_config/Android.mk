LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := xt_config

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../boost_1.56 \

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CPPFLAGS += -fPIC -fexceptions -frtti -fvisibility=hidden -g -O0 -DXT_CONFIG_EXPORTS

SRC_FILES := $(wildcard $(LOCAL_PATH)/*.cpp)
SRC_FILES := $(SRC_FILES:$(LOCAL_PATH)/%=%)

LOCAL_SRC_FILES += $(SRC_FILES)  \

LOCAL_STATIC_LIBRARIES := boost_armeabi \

LOCAL_SHARED_LIBRARIES := xt_config \

include $(BUILD_SHARED_LIBRARY)
