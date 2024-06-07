/*
 * Copyright 2015-2016 Preetam J. D'Souza
 * Copyright 2016 The Maru OS Project
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

#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>


#include "PerspectiveService.h"
#include "LXCContainerManager.h"

#define DEBUG 1

namespace aidl {
namespace vendor {
namespace lindroid {
namespace perspective {

/**
 * Name of the container under /data/maru/containers/
 */
static const char *CONTAINER = "default";

PerspectiveService::PerspectiveService() : mContainerManager(NULL) {
    ALOGI("perspectived is starting...");

    mContainerManager = new LXCContainerManager();
}

PerspectiveService::~PerspectiveService() {
    mContainerManager = NULL;
}

bool PerspectiveService::start() {
    ALOGD_IF(DEBUG, "running start()...");

    return mContainerManager->start(CONTAINER);
}

bool PerspectiveService::stop() {
    ALOGD_IF(DEBUG, "running stop()...");

    return mContainerManager->stop(CONTAINER);
}

bool PerspectiveService::isRunning() {
    ALOGD_IF(DEBUG, "running isRunning()...");

    return mContainerManager->isRunning(CONTAINER);
}

}; // namespace android
};
};
};

int main(void) {
    using namespace aidl::vendor::lindroid::perspective;

    // start up the Binder IPC thread pool
   // sp<ProcessState> ps(ProcessState::self());
   // ps->setThreadPoolMaxThreadCount(1);
   // ps->startThreadPool();

  //sp<PerspectiveService> perspect = new PerspectiveService();

    // publish PerspectiveService
  //  sp<IServiceManager> sm(defaultServiceManager());
 //   sm->addService(String16(PerspectiveService::getServiceName()), perspect, false);

    // service Binder requests
 //   IPCThreadState::self()->joinThreadPool();

      std::shared_ptr<PerspectiveService> perspect = ndk::SharedRefBase::make<PerspectiveService>();
    ABinderProcess_setThreadPoolMaxThreadCount(0);

        binder_status_t status = AServiceManager_addService(perspect->asBinder().get(), "perspective");
		if (status != STATUS_OK) {
	//		ALOGE("Could not register perspect binder service");
            return EXIT_FAILURE;
        }

        ABinderProcess_joinThreadPool();

    // should never get here
    return -1;
}
