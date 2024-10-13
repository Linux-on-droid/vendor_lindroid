#!/system/bin/sh
#
# SPDX-License-Identifier: LGPL-2.1+
#
# Adapted and stripped down for Lindroid by nift4;
# originally was: Client script for LXC container images.

set -eu

LXC_HOOK_DIR=/data/lindroid/lxc/hooks
LXC_TEMPLATE_CONFIG=/system_ext/usr/share/lindroid/lxc/config.d

LXC_NAME=
LXC_PATH=
LXC_ROOTFS=
LXC_METADATA=
LXC_FSTREE=
MODE="system"
COMPAT_LEVEL=5

EXCLUDES=""
TEMPLATE_FILES=""


# Make sure the usual locations are in PATH
export PATH="$PATH:/usr/sbin:/usr/bin:/sbin:/bin"
# Welcome to Android where /tmp does not exist
export TMPDIR="/data/local/tmp"

in_userns() {
  [ -e /proc/self/uid_map ] || { echo no; return; }

  while read -r line; do
    fields="$(echo "$line" | awk '{ print $1 " " $2 " " $3 }')"
    if [ "${fields}" = "0 0 4294967295" ]; then
      echo no;
      return;
    fi

    if echo "${fields}" | grep -q " 0 1$"; then
      echo userns-root;
      return;
    fi
  done < /proc/self/uid_map

  if [ -e /proc/1/uid_map ]; then
    if [ "$(cat /proc/self/uid_map)" = "$(cat /proc/1/uid_map)" ]; then
      echo userns-root
      return
    fi
  fi
  echo yes
}

usage() {
  cat <<EOF
LXC local template script.
Creates a container from a local tarball.
The metadata tarball is optional, but if present, it must contain a 'config' file.

Special arguments:
[ -h | --help ]: Print this help message and exit.
[ -m | --metadata ]: Path to the image metadata, should be a tar/tar.gz containing the 'config' file.
[ -f | --fstree ]: Path to the image filesystem tree, should be a tar/tar.gz containing the root filesystem.
[ --no-dev ]: Exclude /dev from the fstree tarball.

LXC internal arguments (do not pass manually!):
[ --name <name> ]: The container name
[ --path <path> ]: The path to the container
[ --rootfs <rootfs> ]: The path to the container's rootfs
[ --mapped-uid <map> ]: A uid map (user namespaces)
[ --mapped-gid <map> ]: A gid map (user namespaces)
EOF
  return 0
}

# here in AOSP we unfortunately cannot afford getopt :)
while [ "x${1:-}" != "x" ]; do
  case "$1" in
    -h|--help)     usage && exit 1;;
    --name=*)      LXC_NAME="${1#--name=}"; shift 1;;
    --path=*)      LXC_PATH="${1#--path=}"; shift 1;;
    --rootfs=*)    LXC_ROOTFS="${1#--rootfs=}"; shift 1;;
    -m)            LXC_METADATA="$2"; shift 2;;
    -f)            LXC_FSTREE="$2"; shift 2;;
    --no-dev)      EXCLUDES="${EXCLUDES} --exclude=./dev/*"; shift 1;;
    --mapped-uid=*)LXC_MAPPED_UID="${1#--mapped-uid=}"; shift 1;;
    --mapped-gid=*)LXC_MAPPED_GID="${1#--mapped-gid=}"; shift 1;;
    *)             echo Invalid usage: "$1"; usage; exit 1;;
  esac
done

# Check for required binaries
for bin in tar; do
  if ! command -V "${bin}" >/dev/null 2>&1; then
    echo "ERROR: Missing required tool: ${bin}" 1>&2
    exit 1
  fi
done

cleanup() {
  if [ -d "${LOCAL_TEMP}" ]; then
    rm -Rf "${LOCAL_TEMP}"
  fi
}

# Trap all exit signals
trap cleanup EXIT HUP INT TERM

USERNS="$(in_userns)"

if [ "${USERNS}" != "no" ]; then
  if [ "${USERNS}" = "yes" ]; then
    if [ -z "${LXC_MAPPED_UID}" ] || [ "${LXC_MAPPED_UID}" = "-1" ]; then
      echo "ERROR: In a user namespace without a map" 1>&2
      exit 1
    fi
    MODE="user"
  else
    MODE="user"
  fi
fi

relevant_file() {
  # Reads the supplied file name from LOCAL_TEMP..
  # If the file exists in $LOCAL_TEMP/$FILENAME.$COMPAT_LEVEL, it will be used.
  #   The default COMPAT_LEVEL is 5.
  # If the file exists in $LOCAL_TEMP/$FILENAME-$MODE, it will be used.
  #   The default MODE is "system".
  # If a mode or compatibility level specific file does not exist, the
  # passed file name will be used.
  FILE_PATH="${LOCAL_TEMP}/$1"

  if [ -e "${FILE_PATH}-${MODE}" ]; then
    FILE_PATH="${FILE_PATH}-${MODE}"
  fi

  if [ -e "${FILE_PATH}.${COMPAT_LEVEL}" ]; then
    FILE_PATH="${FILE_PATH}.${COMPAT_LEVEL}"
  fi

  echo "${FILE_PATH}"
}

create_build_dir() {
  # Create temporary build directory.
  # If mktemp is not available, use /tmp/lxc-local.$$
  if ! command -V mktemp >/dev/null 2>&1; then
    LOCAL_TEMP=$TMPDIR/lxc-local.$$
    mkdir -p "${LOCAL_TEMP}"
  else
    LOCAL_TEMP=$(mktemp -d)
  fi

  echo "Using local temporary directory ${LOCAL_TEMP}"
}

process_excludes() {
  # If the file ${LXC_PATH}/excludes exists, it will be used as a list of
  # files to exclude from the tarball.
  excludelist=$(relevant_file excludes)
  if [ -f "${excludelist}" ]; then
    while read -r line; do
      EXCLUDES="${EXCLUDES} --exclude=${line}"
    done < "${excludelist}"
  fi
}

record_template_file() {
  if [ -z "${TEMPLATE_FILES}" ]; then
    TEMPLATE_FILES="$1"
  else
    TEMPLATE_FILES="${TEMPLATE_FILES};$1"
  fi
}

extract_config() {
  # lxc-create will automatically create a config file at ${LXC_PATH}/config.
  # This function extracts the network config, and any remaining lxc config
  # The extracted config is stored in ${LXC_PATH}/config-network and ${LXC_PATH}/config-extra
  # The config will be merged later.

  # Extract all the network config entries
  sed -i -e "/lxc.net.0/{w ${LXC_PATH}/config-network" -e "d}" "${LXC_PATH}/config"

  if [ -e "${LXC_PATH}/config-network" ]; then
    echo "Extracted network config to: ${LXC_PATH}/config-network"
    cat "${LXC_PATH}/config-network"
  fi

  # Extract any other config entry
  sed -i -e "/lxc./{w ${LXC_PATH}/config-extra" -e "d}" "${LXC_PATH}/config"

  if [ -e "${LXC_PATH}/config-extra" ]; then
    echo "Extracted additional config to: ${LXC_PATH}/config-extra"
    cat "${LXC_PATH}/config-extra"
  fi
}

add_container_config() {
  # Adds the contents of the supplied metadata config to the container config
  {
    echo ""
    echo "# Distribution configuration"
    cat "$configfile"
  } >> "${LXC_PATH}/config"
}

add_extra_config() {
  # Add the container-specific config
  {
    echo ""
    echo "# Container specific configuration"
    if [ -e "${LXC_PATH}/config-extra" ]; then
      cat "${LXC_PATH}/config-extra"
      rm "${LXC_PATH}/config-extra"
    fi
  } >> "${LXC_PATH}/config"
}

add_network_config() {
  # Re-add the previously removed network config
  if [ -e "${LXC_PATH}/config-network" ]; then
    {
      echo ""
      echo "# Network configuration"
      cat "${LXC_PATH}/config-network"
      rm "${LXC_PATH}/config-network"
    } >> "${LXC_PATH}/config"
  fi
}

process_config() {
  # Process the supplied config file if it exists.
  configfile="$(relevant_file config)"
  if [ ! -e "${configfile}" ]; then
    echo "ERROR: metadata tarball is missing the configuration file" 1>&2
    return
  fi

  extract_config
  add_container_config
  add_extra_config
  add_network_config
  record_template_file "${LXC_PATH}/config"
}

process_fstab() {
  # Process the supplied fstab file if it exists.
  # Add the fstab file to template files so substitions can be made.
  fstab="$(relevant_file fstab)"
  if [ -e "${fstab}" ]; then
    echo "lxc.mount.fstab = ${LXC_PATH}/fstab" >> "${LXC_PATH}/config"
    cp "${fstab}" "${LXC_PATH}/fstab"
    record_template_file "${LXC_PATH}/fstab"
  fi
}

process_templates() {
  # Look for extra templates
  templates="$(relevant_file templates)"
  if [ -e "${templates}" ]; then
    while read -r line; do
      fullpath="${LXC_ROOTFS}/${line}"
      record_template_file "${fullpath}"
    done < "${templates}"
  fi
}

unpack_metadata() {
  # Unpack file that contains the container metadata
  # If the file does not exist, just warn and continue.
  if [ -z "${LXC_METADATA}" ]; then
    echo "Metadata file was not passed" 2>&1
    return
  fi

  if [ ! -f "${LXC_METADATA}" ]; then
    echo "Metadata file does not exist: ${LXC_METADATA}" 2>&1
    return
  fi

  echo "Using metadata file: ${LXC_METADATA}"

  if ! tar xf "${LXC_METADATA}" -C "${LOCAL_TEMP}"; then
    echo "Unable to unpack metadata file: ${LXC_METADATA}" 2>&1
    exit 1
  fi

  echo "Unpacked metadata file: ${LXC_METADATA}"

  process_excludes
  process_config
  process_fstab
  process_templates
}

set_utsname() {
  # Set the container's hostname
  echo "lxc.uts.name = ${LXC_NAME}" >> "${LXC_PATH}/config"
}

prepare_rootfs() {
  # Additional configuration that must be done on the rootfs.
  # Always add /dev/pts as /dev may be excluded.
  mkdir -p "${LXC_ROOTFS}/dev/pts/"
}

unpack_rootfs() {
  # Unpack the rootfs
  if [ -z "${LXC_FSTREE}" ]; then
    echo "ERROR: Rootfs file was not passed" 2>&1
    exit 1
  fi
  if [ ! -f "${LXC_FSTREE}" ]; then
    echo "ERROR: Rootfs file does not exist: ${LXC_FSTREE}" 2>&1
    exit 1
  fi
  echo "Using rootfs file: ${LXC_FSTREE}"

  # Do not surround ${EXCLUDES} by quotes. This does not work. The solution could
  # use array but this is not POSIX compliant. The only POSIX compliant solution
  # is to use a function wrapper, but the latter can't be used here as the args
  # are dynamic. We thus need to ignore the warning brought by shellcheck.
  # shellcheck disable=SC2086
  if [ -n "${EXCLUDES}" ]; then
    echo "Excludes: ${EXCLUDES}"
  fi

  tar ${EXCLUDES} --numeric-owner --selinux -xpf "${LXC_FSTREE}" -C "${LXC_ROOTFS}"

  prepare_rootfs
}

replace_template_vars() {
  # Replace variables in all templates
  OLD_IFS=${IFS}
  IFS=";"
  for file in ${TEMPLATE_FILES}; do
    [ ! -f "${file}" ] && continue

    sed -i "s#LXC_NAME#${LXC_NAME}#g" "${file}"
    sed -i "s#LXC_PATH#${LXC_PATH}#g" "${file}"
    sed -i "s#LXC_ROOTFS#${LXC_ROOTFS}#g" "${file}"
    sed -i "s#LXC_TEMPLATE_CONFIG#${LXC_TEMPLATE_CONFIG}#g" "${file}"
    sed -i "s#LXC_HOOK_DIR#${LXC_HOOK_DIR}#g" "${file}"
  done
  IFS=${OLD_IFS}
}

overlayfs_quirk() {
  # Lindroid quirk: we need fake overlayfs to mask nosuid
  sed -i 's|lxc.rootfs.path = dir:|lxc.rootfs.path = overlayfs:/data/lindroid/lxc/rootfs_ro:|' "${LXC_PATH}/config"
}

add_base_config() {
  cat >> "${LXC_PATH}/config" <<EOF
# -- static Lindroid configuration begins here --
# Network configuration
lxc.net.0.type = none

# Android mounts
lxc.mount.entry = /sdcard mnt/sdcard none bind,optional,rw,create=dir

# Allow for 1024 pseudo terminals
lxc.pty.max = 1024

# CGroup allowlist
#lxc.cgroup.devices.deny = a TODO do not allow all device operations by default
## Allow any mknod (but not reading/writing the node)
lxc.cgroup.devices.allow = c *:* m
lxc.cgroup.devices.allow = b *:* m
## Allow specific devices
### /dev/null
lxc.cgroup.devices.allow = c 1:3 rwm
### /dev/zero
lxc.cgroup.devices.allow = c 1:5 rwm
### /dev/full
lxc.cgroup.devices.allow = c 1:7 rwm
### /dev/tty
lxc.cgroup.devices.allow = c 5:0 rwm
### /dev/console
lxc.cgroup.devices.allow = c 5:1 rwm
### /dev/ptmx
lxc.cgroup.devices.allow = c 5:2 rwm
### /dev/random
lxc.cgroup.devices.allow = c 1:8 rwm
### /dev/urandom
lxc.cgroup.devices.allow = c 1:9 rwm
### /dev/pts/*
lxc.cgroup.devices.allow = c 136:* rwm
### fuse
lxc.cgroup.devices.allow = c 10:229 rwm

lxc.environment = CGROUP_MANAGER=systemd

lxc.mount.entry = cgroup2 /sys/fs/cgroup cgroup2 defaults 0 0

lxc.mount.entry = /proc proc none bind,optional,rw,create=dir
lxc.mount.entry = /tmp tmp none bind,optional,rw,create=dir
lxc.mount.entry = /vendor vendor none bind,optional,rw,create=dir
lxc.mount.entry = /vendor_dlkm vendor_dlkm none bind,optional,rw,create=dir
lxc.mount.entry = /system system none bind,optional,rw,create=dir
lxc.mount.entry = /system_dlkm system_dlkm none bind,optional,rw,create=dir
lxc.mount.entry = /system_ext system_ext none bind,optional,rw,create=dir
lxc.mount.entry = /product product none bind,optional,rw,create=dir
lxc.mount.entry = /odm odm none bind,optional,rw,create=dir
lxc.mount.entry = /odm_dlkm odm_dlkm none bind,optional,rw,create=dir
lxc.mount.entry = /apex apex none rbind,optional,rw,create=dir
#lxc.mount.entry = /linkerconfig linkerconfig none rbind,optional,rw,create=dir TODO enabling this breaks finding system libs, disabling this breaks finding apex libs...

lxc.mount.entry = /dev/socket dev/socket none bind,optional,rw,create=dir
lxc.mount.entry = /dev/__properties__ dev/__properties__ none bind,optional,rw,create=dir
lxc.mount.entry = /dev/binder dev/binder none bind,optional,rw,create=file
lxc.mount.entry = /dev/hwbinder dev/hwbinder none bind,optional,rw,create=file
lxc.mount.entry = /dev/vndbinder dev/vndbinder none bind,optional,rw,create=file
lxc.mount.entry = /dev/pmsg0 dev/pmsg0 none bind,optional,rw,create=file
lxc.mount.entry = /dev/ashmem dev/ashmem none bind,optional,rw,create=file
lxc.mount.entry = /dev/ion dev/ion none bind,optional,create=file
lxc.mount.entry = /dev/input dev/input none rbind,optional,rw,create=dir
lxc.mount.entry = selinuxfs sys/fs/selinux selinuxfs optional 0 0
lxc.mount.entry = /dev/binderfs dev/binderfs bind bind,create=dir,optional 0 0

# Hack, bind mount patched libc inside container to avoid errors
lxc.mount.entry = /system_ext/usr/share/lindroid/libc.so apex/com.android.runtime/lib64/bionic/libc.so none bind,optional,ro,create=file

# Sockets dir
lxc.mount.entry = /data/lindroid/mnt lindroid bind bind,create=dir,optional 0 0

# GPU devices
lxc.mount.entry = /dev/kgsl-3d0 dev/kgsl-3d0 none bind,optional,create=file
lxc.mount.entry = /dev/mali0 dev/mali0 none bind,optional,create=file
# -- static Lindroid configuration ends here --
EOF
}

fix_tty() {
  # prevent mingetty from calling vhangup(2) since it fails with userns on CentOS / Oracle
  if [ -f "${LXC_ROOTFS}/etc/init/tty.conf" ]; then
    echo "Patching ${LXC_ROOTFS}/etc/init/tty.conf to prevent mingetty from calling vhangup."
    sed -i 's|mingetty|mingetty --nohangup|' "${LXC_ROOTFS}/etc/init/tty.conf"
  fi
}

display_creation_message() {
  if [ -e "$(relevant_file create-message)" ]; then
    echo ""
    echo "---"
    cat "$(relevant_file create-message)"
  fi
}


create_build_dir
unpack_metadata
unpack_rootfs

overlayfs_quirk
set_utsname
add_base_config
replace_template_vars
fix_tty

display_creation_message

exit 0
