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

#ifndef MARU_CONTAINER_MANAGER_H
#define MARU_CONTAINER_MANAGER_H

#include <utils/RefBase.h>

namespace android {

class ContainerManager : public RefBase {
public:
    virtual bool start(const char* id) = 0;
    virtual bool stop(const char* id) = 0;
    virtual bool isRunning(const char* id) = 0;
    virtual bool enableInput(const char* id, const bool enable) = 0;

protected:
    // we are reference counted
    virtual ~ContainerManager(){};
};

}; // namespace android

#endif // MARU_CONTAINER_MANAGER_H
