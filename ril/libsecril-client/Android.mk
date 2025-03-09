# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    secril-client.cpp

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libbinder \
    libcutils \
    libhardware_legacy \
    liblog \
    librilutils

LOCAL_CFLAGS := 

LOCAL_MODULE:= libsecril-client
LOCAL_VENDOR_MODULE := true

include $(BUILD_SHARED_LIBRARY)
