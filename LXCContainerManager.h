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

#pragma once

#include <aidl/vendor/lindroid/perspective/BnPerspective.h>

namespace aidl {
namespace vendor {
namespace lindroid {
namespace perspective {

class LXCContainerManager : public BnPerspective {
public:
    // BnPerspective interface
    // ------------------------------------------------------------------------

    virtual ndk::ScopedAStatus start(const std::string &id, bool *_aidl_return);
    virtual ndk::ScopedAStatus stop(const std::string &id, bool *_aidl_return);
    virtual ndk::ScopedAStatus isRunning(const std::string &id, bool *_aidl_return);

    // ------------------------------------------------------------------------
};

}; // namespace perspective
}; // namespace lindroid
}; // namespace vendor
}; // namespace aidl
