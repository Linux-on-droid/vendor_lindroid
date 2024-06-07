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

#include <utils/Log.h>

#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "LXCContainerManager.h"

using aidl::vendor::lindroid::perspective::LXCContainerManager;

#define SERVICE_NAME "perspective"

int main(void) {
    auto perspective = ndk::SharedRefBase::make<LXCContainerManager>();

    binder_status_t status = AServiceManager_addService(perspective->asBinder().get(), SERVICE_NAME);
    if (status != STATUS_OK) {
        ALOGE("Could not register perspective binder service");
        return EXIT_FAILURE;
    }

    ABinderProcess_joinThreadPool();

    // should never get here
    return EXIT_FAILURE;
}
