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

#include "app_mgr.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "if_system_ability_manager.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

using namespace OHOS;
using namespace OHOS::AAFwk;
using namespace OHOS::AppExecFwk;

static constexpr uint8_t NUMBER_OF_PARAMETERS_TWO = 2;

static napi_value ParseBundleName(napi_env env, std::string &bundleName, napi_value args)
{
    napi_status status;
    napi_valuetype valuetype;
    NAPI_CALL(env, napi_typeof(env, args, &valuetype));
    NAPI_ASSERT(env, valuetype == napi_string, "Wrong argument type. String expected.");
    char buf[BUFFER_LENGTH_MAX] = {0};
    size_t len = 0;
    napi_get_value_string_utf8(env, args, buf, BUFFER_LENGTH_MAX, &len);
    HILOG_INFO("bundleName= [%{public}s].", buf);
    bundleName = std::string {buf};
    // create result code
    napi_value result;
    status = napi_create_int32(env, 1, &result);
    NAPI_ASSERT(env, status == napi_ok, "napi_create_int32 error!");
    return result;
}

static int32_t AbilityMgrKillProcess(const std::string &bundleName)
{
    OHOS::sptr<OHOS::ISystemAbilityManager> systemAbilityManager =
        OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    OHOS::sptr<OHOS::IRemoteObject> abilityObject =
        systemAbilityManager->GetSystemAbility(OHOS::ABILITY_MGR_SERVICE_ID);
    OHOS::sptr<OHOS::AAFwk::IAbilityManager> abilityManager =
        OHOS::iface_cast<OHOS::AAFwk::IAbilityManager>(abilityObject);

    return abilityManager->KillProcess(bundleName);
}

AsyncCallbackInfo::AsyncCallbackInfo(napi_env env)
{
    this->env = env;
}

AsyncCallbackInfo::~AsyncCallbackInfo()
{
    if (asyncWork != nullptr) {
        HILOG_INFO("AsyncCallbackInfo::~AsyncCallbackInfo delete asyncWork.");
        napi_delete_async_work(env, asyncWork);
        asyncWork = nullptr;
    }

    if (callback != nullptr) {
        HILOG_INFO("AsyncCallbackInfo::~AsyncCallbackInfo delete callback.");
        napi_delete_reference(env, callback);
        callback = nullptr;
    }
}

static napi_value NapiGetNull(napi_env env)
{
    napi_value result = nullptr;
    napi_get_null(env, &result);
    return result;
}

napi_value NAPI_KillProcessesByBundleName(napi_env env, napi_callback_info info)
{
    HILOG_INFO("NAPI_KillProcessesByBundleName called...");
    size_t argc = 2;
    napi_value argv[NUMBER_OF_PARAMETERS_TWO];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    HILOG_INFO("argc = [%{public}zu]", argc);

    size_t argcNum = 2;
    if (argc >= argcNum) {
        napi_valuetype valuetype;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valuetype));
        if (valuetype != napi_function) {
            HILOG_ERROR("%{public}s, Wrong argument type. Function expected.", __func__);
            return NapiGetNull(env);
        }
    }

    AsyncCallbackInfo *asyncCallbackInfoPtr = new (std::nothrow) AsyncCallbackInfo(env);
    if (asyncCallbackInfoPtr == nullptr) {
        HILOG_ERROR("%{public}s, asyncCallbackInfoPtr == nullptr", __func__);
        return NapiGetNull(env);
    }

    std::unique_ptr<AsyncCallbackInfo> asyncCallbackInfoUPtr(asyncCallbackInfoPtr);
    std::string bundleName;
    ParseBundleName(env, bundleName, argv[0]);

    if (argc >= argcNum) {
        asyncCallbackInfoUPtr->bundleName = bundleName;
        NAPI_CALL(env, napi_create_reference(env, argv[1], 1, &asyncCallbackInfoUPtr->callback));

        napi_value resourceName;
        NAPI_CALL(env, napi_create_string_latin1(env, "NAPI_KillProcessesByBundleNameCallBack",
            NAPI_AUTO_LENGTH, &resourceName));

        NAPI_CALL(env, napi_create_async_work(env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("killProcessesByBundleName called(CallBack Mode)...");
                AsyncCallbackInfo *asyncCallbackInfoPtr = (AsyncCallbackInfo *)data;
                asyncCallbackInfoPtr->result = AbilityMgrKillProcess(asyncCallbackInfoPtr->bundleName);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("killProcessesByBundleName compeleted(CallBack Mode)...");
                AsyncCallbackInfo *asyncCallbackInfoPtr = (AsyncCallbackInfo *)data;
                std::unique_ptr<AsyncCallbackInfo> asyncCallbackInfoUPtr {asyncCallbackInfoPtr};

                napi_value result;
                napi_value callback;
                napi_value undefined;

                napi_create_int32(asyncCallbackInfoUPtr->env, asyncCallbackInfoUPtr->result, &result);
                napi_get_undefined(env, &undefined);

                napi_get_reference_value(env, asyncCallbackInfoUPtr->callback, &callback);
                napi_call_function(env, undefined, callback, 1, &result, nullptr);
            },
            (void *)asyncCallbackInfoUPtr.get(),
            &asyncCallbackInfoUPtr->asyncWork));

        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfoUPtr->asyncWork));
        asyncCallbackInfoUPtr.release();
        return NapiGetNull(env);
    } else {
        napi_value resourceName;
        NAPI_CALL(env, napi_create_string_latin1(env, "NAPI_KillProcessesByBundleNamePromise",
            NAPI_AUTO_LENGTH, &resourceName));

        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfoUPtr->deferred = deferred;
        asyncCallbackInfoUPtr->bundleName = bundleName;

        NAPI_CALL(env, napi_create_async_work(env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("killProcessesByBundleName called(Promise Mode)...");
                AsyncCallbackInfo *asyncCallbackInfoPtr = (AsyncCallbackInfo *)data;
                asyncCallbackInfoPtr->result = AbilityMgrKillProcess(asyncCallbackInfoPtr->bundleName);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("killProcessesByBundleName compeleted(Promise Mode)...");
                AsyncCallbackInfo *asyncCallbackInfoPtr = (AsyncCallbackInfo *)data;
                std::unique_ptr<AsyncCallbackInfo> asyncCallbackInfoUPtr {asyncCallbackInfoPtr};

                napi_value result;
                napi_create_int32(asyncCallbackInfoUPtr->env, asyncCallbackInfoUPtr->result, &result);
                if (asyncCallbackInfoUPtr->result == ERR_OK) {
                    napi_resolve_deferred(asyncCallbackInfoUPtr->env, asyncCallbackInfoUPtr->deferred, result);
                } else {
                    napi_reject_deferred(asyncCallbackInfoUPtr->env, asyncCallbackInfoUPtr->deferred, result);
                }
            },
            (void *)asyncCallbackInfoUPtr.get(),
            &asyncCallbackInfoUPtr->asyncWork));
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfoUPtr->asyncWork));
        asyncCallbackInfoUPtr.release();
        return promise;
    }
}