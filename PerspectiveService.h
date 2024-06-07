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

#ifndef MARU_PERSPECTIVE_SERVICE_H
#define MARU_PERSPECTIVE_SERVICE_H

#include <utils/RefBase.h>
#include <perspective/IPerspectiveService.h>

#include "ContainerManager.h"

namespace android {

class PerspectiveService : public BnPerspectiveService {
public:
    static const char *getServiceName() {
        return "PerspectiveService";
    }

    PerspectiveService();

private:
    // we are reference counted
    ~PerspectiveService();

    // IPerspectiveService interface
    // ------------------------------------------------------------

    virtual bool start();
    virtual bool stop();
    virtual bool isRunning();
    virtual bool enableInput(bool enable);

    // ------------------------------------------------------------

    sp<ContainerManager> mContainerManager;
};

}; // namespace android

#endif // MARU_PERSPECTIVE_SERVICE_H
