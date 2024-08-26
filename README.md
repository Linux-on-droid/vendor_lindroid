# vendor_lindroid

## What is it?

Lindroid is an app (and patchset for AOSP) that allows you to run Linux containers using lxc with hardware acceleration powered by [libhybris](https://github.com/libhybris/libhybris).

Lindroid consists of a native daemon responsible for overseeing the creation, execution and management of lxc containers. The app on the one hand is responsible for allowing the user to manage containers, and on the other hand it includes a very minimal implementation/emulation of the Android HWComposer HAL which is used by the container to display images by handing the buffers to the actual Android display stack. This design allows the container (or more specifically, the Wayland compositor like KWin inside the container) to use the same HWComposer API it uses for Halium-based mobile Linux distributions, but without interfering with the Android side of the graphics stack.

This allows you to use an OpenGL ES accelerated desktop environment running on a mostly standard Linux distribution anywhere. Lindroid also supports multiple displays and input types, so you can attach your phone to a display, mouse and keyboard and enjoy the full convergence experience. Integrations between Android and the container such as network access, audio, touch/mouse/keyboard input, internal storage access and running the container in the background are working - and many more integration features are planned. You can create/start/stop/delete containers at runtime and seperate your work into multiple containers. In short: it's Linux, as an app. Without awkward restrictions like broken programs due to missing kernel support, performance limitations due to no hardware acceleration or poor GPU support (e.g. Mali or PowerVR not working) due to not being able to use Android's graphic drivers under Linux.

Of course, this being an AOSP patchset also means you have to install an alternative operating system on your phone. While some proot or PC emulation-based solutions don't need this, we don't believe these are able to really provide an enjoyable desktop PC experience.

## How do I use it?

Note: more detailed instructions will be added here in the future. You can also join our [Telegram chat](https://t.me/linux_on_droid) if you need help :)

Patch your device kernel to enable these defconfigs:

    CONFIG_SYSVIPC=y
    CONFIG_UTS_NS=y
    CONFIG_PID_NS=y
    CONFIG_IPC_NS=y
    CONFIG_USER_NS=y
    CONFIG_NET_NS=y
    CONFIG_CGROUP_DEVICE=y
    CONFIG_CGROUP_FREEZER=y
    CONFIG_VT=y

Clone vendor_lindroid, vendor_extra, libhybris and external_lxc repositories into an LMODroid (or LineageOS, if you pick [this patch](https://gerrit.libremobileos.com/c/LMODroid/platform_frameworks_native/+/12936)) tree, set your SELinux to permissive (not recommended for production use!) and build it! You then will have a Lindroid app you can use in your app drawer.

## Community

Join our [Telegram chat](https://t.me/linux_on_droid)!

## Credits

The [libhybris](https://github.com/libhybris) maintainers for making this possible<br>
[Maru OS](https://github.com/maruos) for the idea, groundwork, and perspectived<br>
[Droidian](https://github.com/droidian)<br>
[UBports](https://ubports.com)<br>
[KDE](https://kde.org) for Plasma and KWin<br>


