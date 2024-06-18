/*
 * Copyright 2017 The Maru OS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <binder/ParcelFileDescriptor.h>
#include <lxc/attach_options.h>
#include <lxc/lxccontainer.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <utils/Log.h>

#include "LXCContainerManager.h"

#define DEBUG 1

namespace aidl {
namespace vendor {
namespace lindroid {
namespace perspective {

/*
 * We use a purely stateless approach to speaking with liblxc since container
 * state is determined at runtime via liblxc, similar to the lxc cmdline
 * utilities lxc-start, lxc-stop, etc. In other words, we re-create the
 * lxc_container object for each operation, rather than caching it. This has
 * the advantage of using an up-to-date container object for all container
 * operations.
 */

static struct lxc_container* initContainer(const char* id) {
    struct lxc_container *c = lxc_container_new(id, NULL);
    if (!c) {
        ALOGW("can't initialize desktop container, container is NULL");
    } else if (!c->is_defined(c)) {
        ALOGW("can't initialize desktop container, container is undefined");
        lxc_container_put(c);
        c = NULL;
    }

    return c;
}

static bool startContainerWithCheck(struct lxc_container *const c) {
    if (!c->start(c, 0, NULL)) {
        ALOGD_IF(DEBUG, "liblxc returned false for start, possible false positive...");
        if (!c->is_running(c)) {
            return false;
        }
    }

    return true;
}


static bool startContainer(const char* id) {
    struct lxc_container *c = initContainer(id);
    bool ret = false;

    if (!c) {
        ALOGE("failed to start container, can't init container");
        return false;
    }

    if (!c->is_running(c)) {
        if (!startContainerWithCheck(c)) {
            ALOGE("failed to start container, lxc said start failed");
            goto out;
        }

        ALOGD_IF(DEBUG, "desktop GO!");
        ALOGD_IF(DEBUG, "container state: %s", c->state(c));
        ALOGD_IF(DEBUG, "container PID: %d", c->init_pid(c));
        ret = true;
    } else {
        ALOGD("desktop already running, ignoring event...");
        ret = true;
    }

out:
    lxc_container_put(c);
    return ret;
}

static bool stopContainer(const char* id) {
    struct lxc_container *c = initContainer(id);
    if (!c) {
        ALOGE("failed to stop container, can't init container");
        return false;
    }

    bool ret = !c->is_running(c) || c->stop(c);

    lxc_container_put(c);
    return ret;
}

static bool containerIsRunning(const char* id) {
    struct lxc_container *c = initContainer(id);
    if (!c) {
        ALOGE("failed to find if container running, can't init container");
        return false;
    }

    bool ret = c->is_running(c);

    lxc_container_put(c);
    return ret;
}

static bool containerAdd(const char* id, int tarball) {
    int len = strlen(id);
    if (strspn(id, "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") != len || len > 50) {
        ALOGE("invalid id");
        return false;
    }
    std::string dir = "/data/lindroid/lxc/container/" + std::string(id);
    if (mkdir(dir.c_str(), 077)) {
        ALOGE("mkdir %s failed", dir.c_str());
        return false;
    }
    std::string cfg = dir + "/config";
    /*dir = dir + "/rootfs";
    if (mkdir(dir.c_str(), 077)) {
        ALOGE("mkdir %s failed", dir.c_str());
        return false;
    }enable this when rootfs pack fixed*/
    std::string cmd = "sed \"s/REPLACEME/" + std::string(id) + "\"/g /system/lindroid/lxc/container/default/config > " + cfg;
    system(cmd.c_str());
    pid_t pid = fork();
    if (pid == -1) {
        ALOGE("failed to fork()");
        return false;
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        if (status) {
            ALOGE("failed to create container, tar returned %d", status);
            return false;
        }
        ALOGI("while creating container, done with extract!");
        // TODO this should probably use lxc templates
        //struct lxc_container *c = lxc_container_new(id, cfg.c_str());
        struct lxc_container *c = initContainer(id);
        if (!c) {
            ALOGE("failed to create container, lxc said new failed");
            return false;
        }
        /*if (!c->create(c, NULL, NULL, NULL, 0, NULL)) {
            ALOGE("failed to create container, lxc said create failed");
            lxc_container_put(c);
            return false;
        }*/
        // we did it!
        lxc_container_put(c);
        return true;
    } else {
        dup2(tarball, STDIN_FILENO);
        close(tarball);
        ALOGI("while creating container, going to extract");
        char binTar[] = { "/system/bin/tar" };
        char x[] = { "x" };
        char dashC[] = { "-C" };
        char* dirCstr = strdup(dir.c_str());
        char* args[] = { binTar, x, dashC, dirCstr, NULL };
        execv(binTar, args);
        exit(1);
    }
}

static bool containerDelete(const char* id) {
    struct lxc_container *c = initContainer(id);
    if (!c) {
        ALOGE("failed to delete container, can't init container");
        lxc_container_put(c);
        return false;
    }

    bool running = c->is_running(c);
    if (running) {
        ALOGE("failed to delete container, can't delete running container");
        lxc_container_put(c);
        return false;
    }

    bool ret = c->destroy(c);
    if (!ret) {
        ALOGE("failed to delete container, lxc said destroy failed");
    }

    lxc_container_put(c);
    return ret;
}

static std::vector<std::string> containerList() {
    char** names = NULL;

    int len = list_defined_containers(NULL, &names, NULL);
    if (len < 0) {
        ALOGE("failed to list containers, lxc said list failed");
        return {};
    }
    std::vector<std::string> ret;
    ret.reserve(len);
    for (int i = 0; i < len; i++) {
        char* name = names[i];
        if (name == NULL) {
            ALOGE("while listing containers, got null name, continuing anyway...");
            continue;
        }
        ret.push_back(name);
        free(name);
    }
    free(names);
    return ret;
}

ndk::ScopedAStatus LXCContainerManager::start(const std::string &id, bool *_aidl_return) {
    *_aidl_return = startContainer(id.c_str());
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus LXCContainerManager::stop(const std::string &id, bool *_aidl_return) {
    *_aidl_return = stopContainer(id.c_str());
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus LXCContainerManager::isRunning(const std::string &id, bool *_aidl_return) {
    *_aidl_return = containerIsRunning(id.c_str());
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus LXCContainerManager::listContainers(std::vector<std::string> *_aidl_return) {
    *_aidl_return = containerList();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus LXCContainerManager::addContainer(const std::string &id, const ndk::ScopedFileDescriptor& fd, bool *_aidl_return) {
    *_aidl_return = containerAdd(id.c_str(), fd.get());
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus LXCContainerManager::deleteContainer(const std::string &id, bool *_aidl_return) {
    *_aidl_return = containerDelete(id.c_str());
    return ndk::ScopedAStatus::ok();
}

} // namespace perspective
} // namespace lindroid
} // namespace vendor
} // namespace aidl
