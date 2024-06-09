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

# Debug libc for lxc container
# PRODUCT_COPY_FILES += $(TARGET_COPY_OUT_SYSTEM)/../symbols/recovery/root/system/lib64/libc.so:$(TARGET_COPY_OUT_SYSTEM)/lindroid/libc.so \

# Misc lindroid stuff
PRODUCT_PACKAGES += \
    libhwc2_compat_layer \
    libui_compat_layer \
    LindroidUI \
    perspectived

# IDC to ignore lindroid inputs on android side
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/idc/disabled.idc:$(TARGET_COPY_OUT_SYSTEM)/usr/idc/Vendor_000a_Product_000a.idc \
    $(LOCAL_PATH)/configs/idc/disabled.idc:$(TARGET_COPY_OUT_SYSTEM)/usr/idc/Vendor_000a_Product_000b.idc \
    $(LOCAL_PATH)/configs/idc/disabled.idc:$(TARGET_COPY_OUT_SYSTEM)/usr/idc/Vendor_000a_Product_000c.idc

# LXC configs and default container
$(call inherit-product, $(LOCAL_PATH)/lxc/lxc.mk)
