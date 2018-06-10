LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := rvcommon_rtspclient

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include/common \

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CFLAGS += -fexceptions -fvisibility=hidden -Wl,-Bsymbolic -g\
	-DLINUX -D_GNU_SOURCE -DRV_CFLAG_EPOLL -D_RV_LINUX_API_DEFAULT -DRV_ANDROID \

SRC_FILES1 := $(wildcard $(LOCAL_PATH)/adapters/windows/*.c)
SRC_FILES1 := $(SRC_FILES1:$(LOCAL_PATH)/%=%)

SRC_FILES2 := $(wildcard $(LOCAL_PATH)/ares/*.c)
SRC_FILES2 := $(SRC_FILES2:$(LOCAL_PATH)/%=%)

SRC_FILES3 := $(wildcard $(LOCAL_PATH)/cbase/*.c)
SRC_FILES3 := $(SRC_FILES3:$(LOCAL_PATH)/%=%)

SRC_FILES4 := $(wildcard $(LOCAL_PATH)/ccore/*.c)
SRC_FILES4 := $(SRC_FILES4:$(LOCAL_PATH)/%=%)

SRC_FILES5 := $(wildcard $(LOCAL_PATH)/ccore/memdrivers/*.c)
SRC_FILES5 := $(SRC_FILES5:$(LOCAL_PATH)/%=%)

SRC_FILES6 := $(wildcard $(LOCAL_PATH)/ccore/netdrivers/*.c)
SRC_FILES6 := $(SRC_FILES6:$(LOCAL_PATH)/%=%)

SRC_FILES7 := $(wildcard $(LOCAL_PATH)/ceutils/*.c)
SRC_FILES7 := $(SRC_FILES7:$(LOCAL_PATH)/%=%)

SRC_FILES8 := $(wildcard $(LOCAL_PATH)/cutils/*.c)
SRC_FILES8 := $(SRC_FILES8:$(LOCAL_PATH)/%=%)


LOCAL_SRC_FILES +=  $(SRC_FILES1) $(SRC_FILES2) $(SRC_FILES3) \
		$(SRC_FILES4) $(SRC_FILES5) $(SRC_FILES6) \
		$(SRC_FILES7) $(SRC_FILES8) \

LOCAL_LDLIBS += -L$(SYSROOT)/usr/lib -llog -landroid \


include $(BUILD_STATIC_LIBRARY)
