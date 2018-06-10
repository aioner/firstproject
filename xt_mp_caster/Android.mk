LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := xt_mp_caster

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../RvRtp_android/inc/common \
	$(LOCAL_PATH)/../RvRtp_android/inc/rtprtcp \
	$(LOCAL_PATH)/../boost_1.56 \
	$(LOCAL_PATH)/../tghelper \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/../trans_common	\
	$(LOCAL_PATH)/../xt_config \


LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CPPFLAGS += -fexceptions -fvisibility=hidden -g -D_ANDROID -DBOOST_THREAD_USES_MOVE -D_USE_RTP_TRAFFIC_SHAPING 
#-D_USE_RTP_SEND_CONTROLLER

SRC_FILES := $(wildcard $(LOCAL_PATH)/*.cpp)
SRC_FILES := $(SRC_FILES:$(LOCAL_PATH)/%=%)

LOCAL_SRC_FILES += $(SRC_FILES) \

include $(BUILD_STATIC_LIBRARY)
