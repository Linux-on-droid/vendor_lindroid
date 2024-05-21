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

# Hybris hwc compat
PRODUCT_PACKAGES += \
    libhwc2_compat_layer \
    libui_compat_layer

# LXC configs and default container
$(call inherit-product, $(LOCAL_PATH)/lxc/lxc.mk)
