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

#include <gtest/gtest.h>

#include <unistd.h>

#define private public
#include "mix_stack_dumper.h"
#undef private
#include "mock_runtime.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::AppExecFwk;

namespace OHOS {
namespace AppExecFwk {
class MixStackDumperTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void MixStackDumperTest::SetUpTestCase()
{}

void MixStackDumperTest::TearDownTestCase()
{}

void MixStackDumperTest::SetUp()
{}

void MixStackDumperTest::TearDown()
{}

static std::string GetCmdResultFromPopen(const std::string& cmd)
{
    if (cmd.empty()) {
        return "";
    }
    FILE* fp = popen(cmd.c_str(), "r");
    if (fp == nullptr) {
        return "";
    }
    const int bufSize = 128; // 128 : cmd result buf size
    char buffer[bufSize];
    std::string result = "";
    while (!feof(fp)) {
        if (fgets(buffer, bufSize - 1, fp) != nullptr) {
            result += buffer;
        }
    }
    pclose(fp);
    return result;
}

static int GetServicePid(const std::string& serviceName)
{
    std::string cmd = "pidof " + serviceName;
    std::string pidStr = GetCmdResultFromPopen(cmd);
    int32_t pid = 0;
    std::stringstream pidStream(pidStr);
    pidStream >> pid;
    return pid;
}

/**
 * @tc.number: MixStackDumperTest001
 * @tc.name: dump com.ohos.systemui process
 * @tc.desc: try to dump com.ohos.systemui process, must be failed.
 */
HWTEST_F(MixStackDumperTest, MixStackDumperTest001, Function | MediumTest | Level3)
{
    MixStackDumper mixDumper;
    pid_t pid = GetServicePid("com.ohos.systemui");
    mixDumper.Init(pid);
    bool ret = mixDumper.DumpMixFrame(1, pid);
    mixDumper.Destroy();
    EXPECT_FALSE(ret);
}

/**
 * @tc.number: MixStackDumperTest002
 * @tc.name: dump current process
 * @tc.desc: dump current process which is not a applicaiton process
 */
HWTEST_F(MixStackDumperTest, MixStackDumperTest002, Function | MediumTest | Level3)
{
    MixStackDumper mixDumper;
    mixDumper.Init(getpid());
    bool ret = mixDumper.DumpMixFrame(1, getpid());
    mixDumper.Destroy();
    EXPECT_TRUE(ret);
}

/**
 * @tc.number: MixStackDumperTest003
 * @tc.name: Call BuildJsStackInfoList Func
 * @tc.desc: test JsRuntime BuildJsStackInfoList Func
 */
HWTEST_F(MixStackDumperTest, MixStackDumperTest003, Function | MediumTest | Level3)
{
    AbilityRuntime::MockRuntime runtime;
    std::vector<JsFrames> frames;
    bool ret = runtime.BuildJsStackInfoList(gettid(), frames);
    EXPECT_TRUE(ret);
}
}  // namespace AppExecFwk
}  // namespace OHOS
