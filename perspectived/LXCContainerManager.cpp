/*
 * Copyright 2017 The Maru OS Project
 * Copyright 2024-2025 Lindroid
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
#include <log/log.h>

#include <map>
#include <string>
#include <poll.h>

#include "LXCContainerManager.h"

#if !defined(LOG_NDEBUG) || (LOG_NDEBUG == 0)
#define DEBUG 1
#else
#define DEBUG 0
#endif

namespace aidl {
namespace vendor {
namespace lindroid {
namespace perspective {

std::map<std::string, int> containerLogFds;
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


static bool startContainer(const char* id, bool capture_output) {
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

        int ttynum = 0;
        int ptxfd = -1;

        if (capture_output) {
            int console_fd = c->console_getfd(c, &ttynum, &ptxfd);
            if (console_fd < 0) {
                ALOGD_IF(DEBUG, "Failed to allocate console for container.\n");
                goto out;
            }
            containerLogFds[std::string(id)] = ptxfd;
        }
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

    // Close and remove PTY
    auto it = containerLogFds.find(std::string(id));
    if (it != containerLogFds.end()) {
        close(it->second);
        containerLogFds.erase(it);
    }

    lxc_container_put(c);
    return ret;
}

static std::string fetchContainerLogs(const char* id) {
    auto it = containerLogFds.find(std::string(id));
    if (it == containerLogFds.end()) {
        ALOGE("No PTY available for container: %s", id);
        return {};
    }

    int ptxfd = it->second;
    char buffer[256];
    std::string logs;

    struct pollfd fds = { .fd = ptxfd, .events = POLLIN };

    while (poll(&fds, 1, 100) > 0) {
        if (fds.revents & POLLIN) {
            ssize_t bytes = read(ptxfd, buffer, sizeof(buffer) - 1);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                logs += buffer;
            } else if (bytes == 0) {
                break;
            } else {
                ALOGE("Error reading from PTY master");
                break;
            }
        }
    }

    return logs;
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
    struct lxc_container *c = lxc_container_new(id, NULL);
    if (!c) {
        ALOGE("failed to create container, lxc said new failed");
        return false;
    }
    if (c->is_running(c)) {
        ALOGE("failed to create container, can't create running container");
        lxc_container_put(c);
        return false;
    }
    if (c->is_defined(c)) {
        ALOGE("failed to create container, can't create defined container");
        lxc_container_put(c);
        return false;
    }
    c->load_config(c, lxc_get_global_config_item("lxc.default_config"));
    int fd = dup(tarball); // avoid binder closing fd on us
    char fdBuf[13] = { '\0' };
    snprintf(fdBuf, sizeof(fdBuf), "/dev/fd/%d", fd);
    ALOGI("going to extract from %s", fdBuf);
    char f[] = { "-f" };
    char* args[] = { f, fdBuf, NULL };
    bool ret = true;
    if (!c->create(c, "lindroid", NULL, NULL, 0, args)) {
        ALOGE("failed to create container, lxc said create failed (check stdout/stderr for more info)");
        ret = false;
    }
    close(fd);
    lxc_container_put(c);
    return ret;
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

LXCContainerManager::LXCContainerManager() {
    struct lxc_log log;
    log.prefix = "perspectived_lxc"; // max 31 chars excl null terminator
    log.file = "none"; // special value to not log to file
    log.level = "DEBUG"; // see lxc_log_priority_to_int
    lxc_log_init(&log);
    struct lxc_container *c = lxc_container_new("", NULL);
    if (!c) {
        ALOGE("failed to enable syslog with dummy container, creation failure");
    } else {
        // There's no proper API except this config flag to enable logging to syslog (-> logcat on Android).
        // Unlike what it seems like, this changes a process-wide setting!
        if (!c->set_config_item(c, "lxc.log.syslog", "local0"))
            ALOGE("failed to enable syslog with dummy container, set config failure");
        lxc_container_put(c);
    }
}

ndk::ScopedAStatus LXCContainerManager::start(const std::string &id, bool capture_output, bool *_aidl_return) {
    *_aidl_return = startContainer(id.c_str(), capture_output);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus LXCContainerManager::stop(const std::string &id, bool *_aidl_return) {
    *_aidl_return = stopContainer(id.c_str());
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus LXCContainerManager::fetchLogs(const std::string &id, std::string *_aidl_return) {
    *_aidl_return = fetchContainerLogs(id.c_str());
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
