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

#include "runtime_extractor.h"

#include "hilog_wrapper.h"

namespace OHOS {
namespace AbilityRuntime {
RuntimeExtractor::RuntimeExtractor(const std::string& source) : BaseExtractor(source)
{}

RuntimeExtractor::RuntimeExtractor(
    const std::string& source, const std::string& hapPath) : BaseExtractor(source)
{
    hapPath_ = hapPath;
}

RuntimeExtractor::~RuntimeExtractor()
{}

bool RuntimeExtractor::isSameHap(const std::string& hapPath) const
{
    return !hapPath_.empty() && !hapPath.empty() && hapPath_ == hapPath;
}
}  // namespace AbilityRuntime
}  // namespace OHOS
