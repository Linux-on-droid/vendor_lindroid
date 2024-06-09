/**
 * Copyright 2023, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package vendor.lindroid.composer;

parcelable DisplayConfiguration {
    /**
     * The config id, to be used with IComposerClient.setActiveConfig.
     */
    int configId;

    /**
     * The display id.
     */
    long displayId;

    /**
     * Dimensions in pixels
     */
    int width;
    int height;

    /**
     * Dots per inch.
     * If the DPI for a configuration is unavailable or is
     * considered unreliable, the device may set null instead.
     */
    parcelable Dpi {
        float x;
        float y;
    }
    Dpi dpi;

    /**
     * Vsync period in nanoseconds. This period represents the internal
     * frequency of the display. IComposerCallback.onVsync is expected
     * to be called on each vsync event. For non-VRR configurations, a
     * frame can be presented on each vsync event.
     *
     * A present fence, retrieved from CommandResultPayload.presentFence
     * must be signaled on a vsync boundary.
     */
    int vsyncPeriod;
}
