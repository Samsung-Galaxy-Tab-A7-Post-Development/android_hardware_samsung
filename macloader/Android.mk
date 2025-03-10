LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifdef WIFI_DRIVER_MODULE_PATH
LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_PATH=\"$(WIFI_DRIVER_MODULE_PATH)\"
endif

ifeq ($(BCM4334B2_POWER_WAR), true)
LOCAL_CFLAGS += -DBCM4334B2_POWER_WAR
endif

ifeq ($(WLAN_CHIP_TYPE), COB)
LOCAL_CFLAGS += -DCOB_TYPE
endif

ifeq ($(SEC_PRODUCT_SHIP), true)
LOCAL_CFLAGS += -DSEC_PRODUCT_SHIP
endif

ifneq ($(WLAN_DRIVER_TYPE), MODULE)
LOCAL_CFLAGS += -DSEC_WLAN_BUILTIN_DRIVER
endif

$(warning #### [WIFI] WLAN_VENDOR = $(WLAN_VENDOR))
$(warning #### [WIFI] WLAN_CHIP = $(WLAN_CHIP))
$(warning #### [WIFI] WLAN_CHIP_TYPE = $(WLAN_CHIP_TYPE))
$(warning #### [WIFI] WIFI_NEED_CID = $(WIFI_NEED_CID))

###############################################
# WLAN_VENDOR is defined in BoardConfig.mk
###############################################
ifeq ($(WLAN_VENDOR), 1)
LOCAL_CFLAGS += -DBROADCOM_DUT

ifneq ($(WLAN_CHIP_TYPE), COB)
LOCAL_CFLAGS += -DNEED_CID
endif

ifdef WIFI_NEED_CID
LOCAL_CFLAGS += -DNEED_CID
endif

ifeq ($(WLAN_PCIE_COB), true)
LOCAL_CFLAGS += -DBCM_OTP_WRITE
endif


endif
###############################################
ifeq ($(WLAN_VENDOR), 2)
LOCAL_CFLAGS += -DATHEROS_DUT
endif
###############################################
ifeq ($(WLAN_VENDOR), 3)
LOCAL_CFLAGS += -DTI_DUT
endif
###############################################
ifeq ($(WLAN_VENDOR), 4)
LOCAL_CFLAGS += -DQUALCOMM_DUT
ifeq ($(WLAN_CHIP), qca9377)
LOCAL_CFLAGS += -DQUALCOMM_SCPC
endif
endif
###############################################
ifeq ($(WLAN_VENDOR), 5)
LOCAL_CFLAGS += -DMARVELL_DUT
endif
###############################################
ifeq ($(WLAN_VENDOR), 6)
LOCAL_CFLAGS += -DSTERICSSON_DUT
endif
###############################################
ifeq ($(WLAN_VENDOR), 7)
LOCAL_CFLAGS += -DSPRD_DUT
endif
###############################################
ifeq ($(WLAN_VENDOR), 8)
LOCAL_CFLAGS += -DSAMSUNGSLSI_DUT
endif
###############################################
ifeq ($(WLAN_VENDOR), 9)
LOCAL_CFLAGS += -DMTK_DUT
endif
###############################################
LOCAL_SRC_FILES:= \
	macloader.c \

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog \
	libutils \

LOCAL_LDLIBS := -llog 

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_MODULE:= macloader

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/..

LOCAL_PRELINK_MODULE := false

include $(BUILD_EXECUTABLE)
