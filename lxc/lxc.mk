#
# Copyright (C) 2024 The LindroidOS Project
#
# SPDX-License-Identifier: GPL-2.0
#


# Default LXC config
PRODUCT_COPY_FILES += $(LOCAL_PATH)/default.conf:$(TARGET_COPY_OUT_SYSTEM)/lindroid/lxc/default.conf

# Default container
PRODUCT_COPY_FILES += $(LOCAL_PATH)/container/default/config:$(TARGET_COPY_OUT_SYSTEM)/lindroid/lxc/container/default/config

# Lindroid Init
PRODUCT_COPY_FILES += $(LOCAL_PATH)/init.lindroid.rc:$(TARGET_COPY_OUT_SYSTEM)/etc/init/init.lindroid.rc
