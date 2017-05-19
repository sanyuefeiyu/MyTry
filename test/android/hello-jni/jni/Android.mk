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

LOCAL_MODULE    := hello-jni

LOCAL_SRC_FILES := hello-jni.c
LOCAL_SRC_FILES += ../../../src/test2.c
LOCAL_SRC_FILES += ../../../../src/utils/ffmpeg/android/DFFmpegAndroid.c
LOCAL_SRC_FILES += ../../../../src/utils/file/DFile.cpp
LOCAL_SRC_FILES += ../../../../src/utils/lib/android/DLoadAndroid.c
LOCAL_SRC_FILES += ../../../../src/utils/log/DLog.cpp
LOCAL_SRC_FILES += ../../../../src/utils/log/android/DLogAndroid.cpp
LOCAL_SRC_FILES += ../../../../src/utils/misc/android/DMiscAndroid.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../3rd/FFmpeg_3_2_4/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../src/base
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../src/utils/FFmpeg
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../src/utils/file
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../src/utils/lib
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../src/utils/log
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../src/utils/misc

# for logging
LOCAL_LDLIBS    += -L$(LOCAL_PATH)/libs/ -llog

include $(BUILD_SHARED_LIBRARY)
