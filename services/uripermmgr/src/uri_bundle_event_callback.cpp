/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "uri_bundle_event_callback.h"

namespace OHOS {
namespace AAFwk {
UriBundleEventCallback::UriBundleEventCallback() : eventHandler_(nullptr) {}

UriBundleEventCallback::UriBundleEventCallback(std::shared_ptr<AbilityEventHandler> eventHandler)
{
    eventHandler_ = eventHandler;
}


void UriBundleEventCallback::OnReceiveEvent(const EventFwk::CommonEventData eventData)
{
    // env check
    if (eventHandler_ == nullptr) {
        HILOG_ERROR("OnReceiveEvent failed, eventHandler_ is nullptr");
        return;
    }
    const Want& want = eventData.GetWant();
    // action contains the change type of haps.
    std::string action = want.GetAction();
    std::string bundleName = want.GetElement().GetBundleName();
    int uid = want.GetIntParam(KEY_UID, 0);
    // verify data
    if (action.empty() || bundleName.empty()) {
        HILOG_ERROR("OnReceiveEvent failed, empty action/bundleName");
        return;
    }
    HILOG_DEBUG("OnReceiveEvent, action:%{public}s.", action.c_str());

    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED) {
        HILOG_DEBUG("revoke URI permission when uninstall.");
        auto upmClient = AAFwk::UriPermissionManagerClient::GetInstance();
        upmClient->RevokeAllUriPermissions(bundleName);
    }
}

} // namespace AAFwk
} // namespace OHOS