#!/system/bin/sh

CONTAINER_DIR="/data/lindroid/lxc/container/default"
CONFIG_SRC="/system/lindroid/lxc/container/default/config"
ROOTFS_SRC="/system/lindroid/lxc/container/default/rootfs.tar.gz"

if [ ! -d "$CONTAINER_DIR" ]; then
    mkdir -p "$CONTAINER_DIR/"
    cp "$CONFIG_SRC" "$CONTAINER_DIR/"
    tar -xzf "$ROOTFS_SRC" -C "$CONTAINER_DIR/"
fi
