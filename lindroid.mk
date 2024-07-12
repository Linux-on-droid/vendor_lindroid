# LXC Tools
PRODUCT_PACKAGES += \
    lxc_attach \
    lxc_autostart \
    lxc_cgroup \
    lxc_checkpoint \
    lxc_config \
    lxc_console \
    lxc_copy \
    lxc_create \
    lxc_destroy \
    lxc_device \
    lxc_execute \
    lxc_freeze \
    lxc_info \
    lxc_ls \
    lxc_monitor \
    lxc_multicall \
    lxc_snapshot \
    lxc_start \
    lxc_stop \
    lxc_top \
    lxc_unfreeze \
    lxc_unshare

# Hacked libc for lxc container
ifneq ($(filter %x86 %x86_64,$(TARGET_PRODUCT)),)
PRODUCT_COPY_FILES += $(LOCAL_PATH)/prebuilt/x86_64/libc.so:$(TARGET_COPY_OUT_SYSTEM_EXT)/usr/share/lindroid/libc.so
else
PRODUCT_COPY_FILES += $(LOCAL_PATH)/prebuilt/arm64/libc.so:$(TARGET_COPY_OUT_SYSTEM_EXT)/usr/share/lindroid/libc.so
endif

# Misc lindroid stuff
PRODUCT_PACKAGES += \
    libhwc2_compat_layer \
    libui_compat_layer \
    LindroidUI \
    perspectived \
    init.lindroid.rc \
    lxc-lindroid \
    lindroid-default.conf

# IDC to ignore lindroid inputs on android side
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/disabled.idc:$(TARGET_COPY_OUT_SYSTEM)/usr/idc/Vendor_000a_Product_000a.idc \
    $(LOCAL_PATH)/configs/disabled.idc:$(TARGET_COPY_OUT_SYSTEM)/usr/idc/Vendor_000a_Product_000b.idc \
    $(LOCAL_PATH)/configs/disabled.idc:$(TARGET_COPY_OUT_SYSTEM)/usr/idc/Vendor_000a_Product_000c.idc \
    $(LOCAL_PATH)/configs/disabled.idc:$(TARGET_COPY_OUT_SYSTEM)/usr/idc/Vendor_000a_Product_000d.idc
