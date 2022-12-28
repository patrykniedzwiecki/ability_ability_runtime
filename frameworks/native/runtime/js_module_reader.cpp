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

#include "js_module_reader.h"

#include "file_path_utils.h"
#include "hilog_wrapper.h"
#include "js_runtime_utils.h"

using namespace OHOS::AbilityBase;

namespace OHOS {
namespace AbilityRuntime {
std::vector<uint8_t> JsModuleReader::operator()(const std::string& hapPath) const
{
    std::vector<uint8_t> buffer;
    if (hapPath.empty()) {
        HILOG_ERROR("hapPath is empty");
        return buffer;
    }
    std::string realHapPath = std::string(ABS_CODE_PATH) + hapPath + std::string(HAP_SUFFIX);
    bool newCreate = false;
    std::string loadPath = ExtractorUtil::GetLoadFilePath(realHapPath);
    std::shared_ptr<Extractor> extractor = ExtractorUtil::GetExtractor(loadPath, newCreate);
    if (extractor == nullptr) {
        HILOG_ERROR("realHapPath %{private}s GetExtractor failed", realHapPath.c_str());
        return buffer;
    }
    std::unique_ptr<uint8_t[]> dataPtr = nullptr;
    size_t len = 0;
    if (!extractor->ExtractToBufByName(MERGE_ABC_PATH, dataPtr, len)) {
        HILOG_ERROR("get mergeAbc fileBuffer failed");
    }
    buffer.assign(dataPtr.get(), dataPtr.get() + len);
    return buffer;
}
} // namespace AbilityRuntime
} // namespace OHOS