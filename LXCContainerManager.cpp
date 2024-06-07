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

#include <cutils/log.h>
#include <lxc/attach_options.h>
#include <lxc/lxccontainer.h>
#include <sys/wait.h>

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

static inline void freeContainer(struct lxc_container *c) {
    lxc_container_put(c);
    c = NULL;
}

static struct lxc_container* initContainer(const char* id) {
    struct lxc_container *c = lxc_container_new(id, NULL);
    if (!c) {
        ALOGW("can't initialize desktop container, container is NULL");
    } else if (!c->is_defined(c)) {
        ALOGW("can't initialize desktop container, container is undefined");
        freeContainer(c);
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
            ALOGE("failed to start desktop");
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
    freeContainer(c);
    return ret;
}

static bool stopContainer(const char* id) {
    struct lxc_container *c = initContainer(id);
    if (!c) {
        ALOGE("failed to stop container, can't init container");
        return false;
    }

    bool ret = !c->is_running(c) || c->stop(c);

    freeContainer(c);
    return ret;
}

static bool containerIsRunning(const char* id) {
    struct lxc_container *c = initContainer(id);
    if (!c) {
        ALOGE("failed to stop container, can't init container");
        return false;
    }

    bool ret = c->is_running(c);

    freeContainer(c);
    return ret;
}

LXCContainerManager::LXCContainerManager() {
    // empty
}

LXCContainerManager::~LXCContainerManager() {
    // empty
}

bool LXCContainerManager::start(const char* id) {
    return startContainer(id);
}

bool LXCContainerManager::stop(const char* id) {
    return stopContainer(id);
}

bool LXCContainerManager::isRunning(const char *id) {
    return containerIsRunning(id);
}

} // namespace android
}
}
}
