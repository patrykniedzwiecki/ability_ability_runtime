/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "js_module_searcher.h"

#include "hilog_wrapper.h"
#include "js_runtime_utils.h"

namespace OHOS {
namespace AbilityRuntime {
std::string JsModuleSearcher::operator()(const std::string& curJsModulePath, const std::string& newJsModuleUri) const
{
    HILOG_INFO("Search JS module (%{public}s, %{public}s) begin",
        curJsModulePath.c_str(), newJsModuleUri.c_str());

    std::string newJsModulePath;

    if (curJsModulePath.empty() || newJsModuleUri.empty()) {
        return newJsModulePath;
    }

    std::string normalizeUri = newJsModuleUri;
    std::replace(normalizeUri.begin(), normalizeUri.end(), '\\', '/');

    switch (normalizeUri[0]) {
        case '.': {
            newJsModulePath = MakeNewJsModulePath(curJsModulePath, normalizeUri);
            break;
        }
        case '@': {
            newJsModulePath = ParseOhmUri(bundleName_, curJsModulePath, normalizeUri);
            if (newJsModulePath.empty()) {
                newJsModulePath = FindNpmPackage(curJsModulePath, normalizeUri);
            }
            break;
        }
        default: {
            newJsModulePath = FindNpmPackage(curJsModulePath, normalizeUri);
            break;
        }
    }

    FixExtName(newJsModulePath);

    HILOG_INFO("Search JS module (%{public}s, %{public}s) => %{public}s end",
        curJsModulePath.c_str(), normalizeUri.c_str(), newJsModulePath.c_str());

    return newJsModulePath;
}
} // namespace AbilityRuntime
} // namespace OHOS