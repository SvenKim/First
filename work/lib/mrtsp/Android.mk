# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := mrtsp
LOCAL_SRC_FILES := rtp__data.c rtp__mod.c rtp__packet.c rtsp_on_msg.c rtsp_session.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../platforms/universal/include
LOCAL_SHARED_LIBRARIES := mcore mhashmap

LOCAL_CFLAGS := $(LOCAL_CFLAGS) $(MY_CFLAGS)
LOCAL_ARM_NEON  := $(MY_LOCAL_ARM_NEON)
include $(MY_LIBRARY_TYPE)
