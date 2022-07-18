/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_RUNTIME_MOCK_APP_SPAWN_CLIENT_H
#define OHOS_ABILITY_RUNTIME_MOCK_APP_SPAWN_CLIENT_H

#include "gmock/gmock.h"

namespace OHOS {
namespace AppExecFwk {
class MockAppSpawnClient : public AppSpawnClient {
public:
    MockAppSpawnClient()
    {}
    virtual ~MockAppSpawnClient()
    {}
    MOCK_METHOD2(StartProcess, ErrCode(const AppSpawnStartMsg &startMsg, pid_t &pid));
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_MOCK_APP_SPAWN_CLIENT_H
