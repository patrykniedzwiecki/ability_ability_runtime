/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "napi_form_host.h"

#include <cinttypes>
#include <regex>
#include <uv.h>
#include <vector>

#include "form_info.h"
#include "hilog_wrapper.h"
#include "napi_form_util.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "runtime.h"

using namespace OHOS;
using namespace OHOS::AAFwk;
using namespace OHOS::AppExecFwk;

namespace {
    constexpr size_t ARGS_SIZE_ZERO = 0;
    constexpr size_t ARGS_SIZE_ONE = 1;
    constexpr size_t ARGS_SIZE_TWO = 2;
    constexpr size_t ARGS_SIZE_THREE = 3;
    constexpr int REF_COUNT = 1;
    constexpr int CALLBACK_FLG = 1;
    constexpr int PROMISE_FLG = 2;
    OHOS::AppExecFwk::Ability* g_ability = nullptr;
}

/**
 * @brief GetGlobalAbility
 *
 * @param[in] env The environment that the Node-API call is invoked under
 *
 * @return OHOS::AppExecFwk::Ability*
 */
static OHOS::AppExecFwk::Ability* GetGlobalAbility(napi_env env)
{
    // get global value
    napi_value global = nullptr;
    napi_get_global(env, &global);

    // get ability
    napi_value abilityObj = nullptr;
    napi_get_named_property(env, global, "ability", &abilityObj);

    // get ability pointer
    OHOS::AppExecFwk::Ability* ability = nullptr;
    napi_get_value_external(env, abilityObj, (void**)&ability);
    HILOG_INFO("%{public}s, ability = [%{public}p]", __func__, ability);
    if (ability == nullptr) {
        if (g_ability == nullptr) {
            std::unique_ptr<AbilityRuntime::Runtime> runtime;
            g_ability = OHOS::AppExecFwk::Ability::Create(runtime);
        }
        ability = g_ability;
        HILOG_INFO("%{public}s, Use Local tmp Ability for Stage Module", __func__);
    }
    return ability;
}

/**
 * @brief Get a C++ string value from Node-API
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[in] value This is an opaque pointer that is used to represent a JavaScript value
 *
 * @return Return a C++ string
 */
static std::string GetStringFromNAPI(napi_env env, napi_value value)
{
    std::string result;
    size_t size = 0;

    if (napi_get_value_string_utf8(env, value, nullptr, 0, &size) != napi_ok) {
        HILOG_ERROR("%{public}s, can not get string size", __func__);
        return "";
    }
    result.reserve(size + 1);
    result.resize(size);
    if (napi_get_value_string_utf8(env, value, result.data(), (size + 1), &size) != napi_ok) {
        HILOG_ERROR("%{public}s, can not get string value", __func__);
        return "";
    }
    return result;
}

/**
 * @brief Parse form info into Node-API
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[in] formInfo it is used for return forminfo to JavaScript
 * @param[out] result This is an opaque pointer that is used to represent a JavaScript value
 *
 * @return void
 */
static void ParseFormInfoIntoNapi(napi_env env, const FormInfo &formInfo, napi_value &result)
{
    // bundleName
    napi_value bundleName;
    napi_create_string_utf8(env, formInfo.bundleName.c_str(), NAPI_AUTO_LENGTH, &bundleName);
    HILOG_DEBUG("%{public}s, bundleName=%{public}s.", __func__, formInfo.bundleName.c_str());
    napi_set_named_property(env, result, "bundleName", bundleName);

    // moduleName
    napi_value moduleName;
    napi_create_string_utf8(env, formInfo.moduleName.c_str(), NAPI_AUTO_LENGTH, &moduleName);
    HILOG_DEBUG("%{public}s, moduleName=%{public}s.", __func__, formInfo.moduleName.c_str());
    napi_set_named_property(env, result, "moduleName", moduleName);

    // abilityName
    napi_value abilityName;
    napi_create_string_utf8(env, formInfo.abilityName.c_str(), NAPI_AUTO_LENGTH, &abilityName);
    HILOG_DEBUG("%{public}s, abilityName=%{public}s.", __func__, formInfo.abilityName.c_str());
    napi_set_named_property(env, result, "abilityName", abilityName);

    // name
    napi_value name;
    napi_create_string_utf8(env, formInfo.name.c_str(), NAPI_AUTO_LENGTH, &name);
    HILOG_DEBUG("%{public}s, name=%{public}s.", __func__, formInfo.name.c_str());
    napi_set_named_property(env, result, "name", name);

    // description
    napi_value description;
    napi_create_string_utf8(env, formInfo.description.c_str(), NAPI_AUTO_LENGTH, &description);
    HILOG_DEBUG("%{public}s, description=%{public}s.", __func__, formInfo.description.c_str());
    napi_set_named_property(env, result, "description", description);

    // descriptionId
    napi_value descriptionId;
    napi_create_int32(env, formInfo.descriptionId, &descriptionId);
    HILOG_DEBUG("%{public}s, descriptionId=%{public}d.", __func__, formInfo.descriptionId);
    napi_set_named_property(env, result, "descriptionId", descriptionId);

    // type
    napi_value type;
    FormType formType = formInfo.type;
    napi_create_int32(env, (int32_t)formType, &type);
    HILOG_DEBUG("%{public}s, formInfo_type=%{public}d.", __func__, (int32_t)formType);
    napi_set_named_property(env, result, "type", type);

    // jsComponentName
    napi_value jsComponentName;
    napi_create_string_utf8(env, formInfo.jsComponentName.c_str(), NAPI_AUTO_LENGTH, &jsComponentName);
    HILOG_DEBUG("%{public}s, jsComponentName=%{public}s.", __func__, formInfo.jsComponentName.c_str());
    napi_set_named_property(env, result, "jsComponentName", jsComponentName);

    // colorMode
    napi_value colorMode;
    FormsColorMode  formsColorMode = formInfo.colorMode;
    napi_create_int32(env, (int32_t)formsColorMode, &colorMode);
    HILOG_DEBUG("%{public}s, formInfo_type=%{public}d.", __func__, (int32_t)formsColorMode);
    napi_set_named_property(env, result, "colorMode", colorMode);

    // defaultFlag
    napi_value defaultFlag;
    napi_create_int32(env, (int32_t)formInfo.defaultFlag, &defaultFlag);
    HILOG_DEBUG("%{public}s, defaultFlag=%{public}d.", __func__, formInfo.defaultFlag);
    napi_set_named_property(env, result, "isDefault", defaultFlag);

    // updateEnabled
    napi_value updateEnabled;
    napi_create_int32(env, (int32_t)formInfo.updateEnabled, &updateEnabled);
    HILOG_DEBUG("%{public}s, updateEnabled=%{public}d.", __func__, formInfo.updateEnabled);
    napi_set_named_property(env, result, "updateEnabled", updateEnabled);

    // formVisibleNotify
    napi_value formVisibleNotify;
    napi_create_int32(env, (int32_t)formInfo.formVisibleNotify, &formVisibleNotify);
    HILOG_DEBUG("%{public}s, formVisibleNotify=%{public}d.", __func__, formInfo.formVisibleNotify);
    napi_set_named_property(env, result, "formVisibleNotify", formVisibleNotify);

    // formConfigAbility
    napi_value formConfigAbility;
    napi_create_string_utf8(env, formInfo.formConfigAbility.c_str(), NAPI_AUTO_LENGTH, &formConfigAbility);
    HILOG_DEBUG("%{public}s, formConfigAbility=%{public}s.", __func__, formInfo.formConfigAbility.c_str());
    napi_set_named_property(env, result, "formConfigAbility", formConfigAbility);

    // updateDuration
    napi_value updateDuration;
    napi_create_int32(env, formInfo.updateDuration, &updateDuration);
    HILOG_DEBUG("%{public}s, updateDuration=%{public}d.", __func__, formInfo.updateDuration);
    napi_set_named_property(env, result, "updateDuration", updateDuration);

    // scheduledUpdateTime
    napi_value scheduledUpdateTime;
    napi_create_string_utf8(env, formInfo.scheduledUpdateTime.c_str(), NAPI_AUTO_LENGTH, &scheduledUpdateTime);
    HILOG_DEBUG("%{public}s, scheduledUpdateTime=%{public}s.", __func__, formInfo.scheduledUpdateTime.c_str());
    napi_set_named_property(env, result, "scheduledUpdateTime", scheduledUpdateTime);

    // defaultDimension
    napi_value defaultDimension;
    napi_create_int32(env, formInfo.defaultDimension, &defaultDimension);
    HILOG_DEBUG("%{public}s, defaultDimension=%{public}d.", __func__, formInfo.defaultDimension);
    napi_set_named_property(env, result, "defaultDimension", defaultDimension);

    // supportDimensions
    napi_value supportDimensions;
    napi_create_array(env, &supportDimensions);
    int iDimensionsCount = 0;
    for (auto  dimension : formInfo.supportDimensions) {
        HILOG_DEBUG("%{public}s, dimension=%{public}d.", __func__, dimension);
        napi_value dimensionInfo;
        napi_create_int32(env, (int32_t)dimension, &dimensionInfo);
        napi_set_element(env, supportDimensions, iDimensionsCount, dimensionInfo);
        ++iDimensionsCount;
    }
    HILOG_DEBUG("%{public}s, supportDimensions size=%{public}zu.", __func__, formInfo.supportDimensions.size());
    napi_set_named_property(env, result, "supportDimensions", supportDimensions);

    // metaData
    napi_value metaData;
    napi_create_object(env, &metaData);

    // metaData: customizeDatas
    napi_value customizeDatas;
    napi_create_array(env, &customizeDatas);
    int iCustomizeDatasCount = 0;
    for (auto  customizeData : formInfo.customizeDatas) {
        napi_value customizeDataOnject = nullptr;
        napi_create_object(env, &customizeDataOnject);

        // customizeData : name
        napi_value customizeDataName;
        napi_create_string_utf8(env, customizeData.name.c_str(), NAPI_AUTO_LENGTH, &customizeDataName);
        HILOG_DEBUG("%{public}s, customizeData.name=%{public}s.", __func__, customizeData.name.c_str());
        napi_set_named_property(env, customizeDataOnject, "name", customizeDataName);

        // customizeData : value
        napi_value customizeDataValue;
        napi_create_string_utf8(env, customizeData.value.c_str(), NAPI_AUTO_LENGTH, &customizeDataValue);
        HILOG_DEBUG("%{public}s, customizeData.value=%{public}s.", __func__, customizeData.value.c_str());
        napi_set_named_property(env, customizeDataOnject, "value", customizeDataValue);

        napi_set_element(env, customizeDatas, iCustomizeDatasCount, customizeDataOnject);
        ++iDimensionsCount;
    }
    HILOG_DEBUG("%{public}s, customizeDatas size=%{public}zu.", __func__, formInfo.customizeDatas.size());
    napi_set_named_property(env, metaData, "customizeData", customizeDatas);
    napi_set_named_property(env, result, "metaData", metaData);

    return;
}

/**
 * @brief  Call native kit function: DeleteForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 *
 * @return void
 */
static void InnerDelForm(napi_env env, AsyncDelFormCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    ErrCode ret = ability->DeleteForm(asyncCallbackInfo->formId);
    asyncCallbackInfo->result = ret;
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: deleteForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 *
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_DeleteForm(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    HILOG_INFO("%{public}s, argc = [%{public}zu]", __func__, argc);

    // Check the value type of the arguments
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    if (valueType != napi_string) {
        AsyncErrMsgCallbackInfo *asyncErrorInfo = new
            AsyncErrMsgCallbackInfo {
                .env = env,
                .asyncWork = nullptr,
                .deferred = nullptr,
                .callback = nullptr,
                .code = ERR_APPEXECFWK_FORM_INVALID_FORM_ID,
                .type = 0,
                .callbackValue = argv[1]
            };

        if (argc == ARGS_SIZE_TWO) {
            asyncErrorInfo->type = CALLBACK_FLG;
        } else {
            asyncErrorInfo->type = PROMISE_FLG;
        }
        return RetErrMsg(asyncErrorInfo);
    }

    std::string strFormId = GetStringFromNAPI(env, argv[0]);
    int64_t formId;
    HILOG_ERROR("%{public}s, form id ", strFormId.c_str());
    if (!ConvertStringToInt64(strFormId, formId)) {
        AsyncErrMsgCallbackInfo *asyncErrorInfo = new
            AsyncErrMsgCallbackInfo {
                .env = env,
                .asyncWork = nullptr,
                .deferred = nullptr,
                .callback = nullptr,
                .code = ERR_APPEXECFWK_FORM_FORM_ID_NUM_ERR,
                .type = 0,
                .callbackValue = argv[1]
            };

        if (argc == ARGS_SIZE_TWO) {
            asyncErrorInfo->type = CALLBACK_FLG;
        } else {
            asyncErrorInfo->type = PROMISE_FLG;
        }
        return RetErrMsg(asyncErrorInfo);
    }

    AsyncDelFormCallbackInfo *asyncCallbackInfo = new
        AsyncDelFormCallbackInfo {
            .env = env,
            .ability = GetGlobalAbility(env),
            .asyncWork = nullptr,
            .deferred = nullptr,
            .callback = nullptr,
            .formId = 0,
            .result = 0,
        };
    asyncCallbackInfo->formId = formId;

    if (argc == ARGS_SIZE_TWO) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of deleteForm is incorrect,\
            expected type is function.");

        napi_create_reference(env, argv[1], REF_COUNT, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);
                AsyncDelFormCallbackInfo *asyncCallbackInfo = (AsyncDelFormCallbackInfo *)data;
                InnerDelForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncDelFormCallbackInfo *asyncCallbackInfo = (AsyncDelFormCallbackInfo *)data;
                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    napi_create_int32(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, ARGS_SIZE_ONE, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
        return NapiGetResut(env, 1);
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise running", __func__);
                AsyncDelFormCallbackInfo *asyncCallbackInfo = (AsyncDelFormCallbackInfo *)data;
                InnerDelForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);
                AsyncDelFormCallbackInfo *asyncCallbackInfo = (AsyncDelFormCallbackInfo *)data;

                napi_value result;
                InnerCreateRetMsg(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: ReleaseForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 *
 * @return void
 */
static void InnerReleaseForm(napi_env env, AsyncReleaseFormCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    ErrCode ret = ability->ReleaseForm(asyncCallbackInfo->formId, asyncCallbackInfo->isReleaseCache);
    asyncCallbackInfo->result = ret;
    HILOG_DEBUG("%{public}s end", __func__);
}

// NAPI_ReleaseForm InnerReleaseForm callback execute
napi_async_execute_callback NAPI_ReleaseFormAsyncExecute = [](napi_env env, void *data) {
    HILOG_INFO("NAPI_ReleaseForm InnerReleaseForm execute callback");
    AsyncReleaseFormCallbackInfo *asyncCallbackInfo =
    (AsyncReleaseFormCallbackInfo *)data;
    InnerReleaseForm(env, asyncCallbackInfo);
};

// NAPI_ReleaseForm callback complete
napi_async_complete_callback NAPI_ReleaseFormAsyncComplete = [](napi_env env, napi_status status, void *data) {
    HILOG_INFO("NAPI_ReleaseForm compeleted callback");
    AsyncReleaseFormCallbackInfo *asyncCallbackInfo =
    (AsyncReleaseFormCallbackInfo *)data;

    if (asyncCallbackInfo->callback != nullptr) {
        napi_value result;
        InnerCreateRetMsg(env, asyncCallbackInfo->result, &result);
        napi_value callback;
        napi_value undefined;
        napi_get_undefined(env, &undefined);
        napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
        napi_value callResult;
        napi_call_function(env, undefined, callback, ARGS_SIZE_ONE, &result, &callResult);
        napi_delete_reference(env, asyncCallbackInfo->callback);
    }
    napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
    delete asyncCallbackInfo;
};

// NAPI_ReleaseForm promise Complete
napi_async_complete_callback NAPI_ReleaseFormPromiseComplete = [](napi_env env, napi_status status, void *data) {
    HILOG_INFO("%{public}s, promise complete", __func__);
    AsyncReleaseFormCallbackInfo *asyncCallbackInfo = (AsyncReleaseFormCallbackInfo *)data;

    napi_value result;
    InnerCreateRetMsg(env, asyncCallbackInfo->result, &result);
    napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
    napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
    delete asyncCallbackInfo;
};

// ReleaseForm callback
napi_value ReleaseFormCallback(napi_env env, AsyncReleaseFormCallbackInfo *asyncCallbackInfo)
{
    HILOG_INFO("%{public}s, asyncCallback.", __func__);

    napi_value resourceName;
    napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
    napi_create_async_work(
        env,
        nullptr,
        resourceName,
        NAPI_ReleaseFormAsyncExecute,
        NAPI_ReleaseFormAsyncComplete,
        (void *)asyncCallbackInfo,
        &asyncCallbackInfo->asyncWork);
    NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
    return NapiGetResut(env, 1);
}

// ReleaseForm promise
napi_value ReleaseFormPromise(napi_env env, AsyncReleaseFormCallbackInfo *asyncCallbackInfo)
{
    HILOG_INFO("%{public}s, promise.", __func__);
    napi_deferred deferred;
    napi_value promise;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    asyncCallbackInfo->deferred = deferred;

    napi_value resourceName;
    napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
    napi_create_async_work(
        env,
        nullptr,
        resourceName,
        NAPI_ReleaseFormAsyncExecute,
        NAPI_ReleaseFormPromiseComplete,
        (void *)asyncCallbackInfo,
        &asyncCallbackInfo->asyncWork);
    napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
    return promise;
}

/**
 * @brief  The implementation of Node-API interface: releaseForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 *
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_ReleaseForm(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_THREE;
    napi_value argv[ARGS_SIZE_THREE] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc < ARGS_SIZE_ONE || argc > ARGS_SIZE_THREE) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    HILOG_INFO("%{public}s, argc = [%{public}zu]", __func__, argc);

    // Check the value type of the arguments
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[ARGS_SIZE_ZERO], &valueType));
    if (valueType != napi_string) {
        AsyncErrMsgCallbackInfo *asyncErrorInfo = new
            AsyncErrMsgCallbackInfo {
                .env = env,
                .asyncWork = nullptr,
                .deferred = nullptr,
                .callback = nullptr,
                .code = ERR_APPEXECFWK_FORM_INVALID_FORM_ID,
                .type = 0,
                .callbackValue = argv[ARGS_SIZE_TWO]
            };

        if (argc == ARGS_SIZE_ONE) {
            asyncErrorInfo->type = PROMISE_FLG;
        } else if (argc == ARGS_SIZE_THREE) {
            asyncErrorInfo->type = CALLBACK_FLG;
            asyncErrorInfo->callbackValue = argv[ARGS_SIZE_TWO];
        } else {
            valueType = napi_undefined;
            NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
            if (valueType == napi_function) {
                asyncErrorInfo->type = CALLBACK_FLG;
                asyncErrorInfo->callbackValue = argv[1];
            } else if (valueType == napi_boolean) {
                asyncErrorInfo->type = PROMISE_FLG;
            } else {
                NAPI_ASSERT(env, valueType == napi_function || valueType == napi_boolean,
                    "The arguments[2] type of releaseForm is incorrect,expected type is function or boolean.");
            }
        }
        return RetErrMsg(asyncErrorInfo);
    }

    std::string strFormId = GetStringFromNAPI(env, argv[ARGS_SIZE_ZERO]);
    int64_t formId;
    if (!ConvertStringToInt64(strFormId, formId)) {
        AsyncErrMsgCallbackInfo *asyncErrorInfo = new
            AsyncErrMsgCallbackInfo {
                .env = env,
                .asyncWork = nullptr,
                .deferred = nullptr,
                .callback = nullptr,
                .code = ERR_APPEXECFWK_FORM_FORM_ID_NUM_ERR,
                .type = 0,
                .callbackValue = argv[ARGS_SIZE_TWO]
            };

        if (argc == ARGS_SIZE_ONE) {
            asyncErrorInfo->type = PROMISE_FLG;
        } else if (argc == ARGS_SIZE_THREE) {
            asyncErrorInfo->type = CALLBACK_FLG;
            asyncErrorInfo->callbackValue = argv[ARGS_SIZE_TWO];
        } else {
            valueType = napi_undefined;
            NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
            if (valueType == napi_function) {
                asyncErrorInfo->type = CALLBACK_FLG;
                asyncErrorInfo->callbackValue = argv[1];
            } else if (valueType == napi_boolean) {
                asyncErrorInfo->type = PROMISE_FLG;
            } else {
                NAPI_ASSERT(env, valueType == napi_function || valueType == napi_boolean,
                    "The arguments[2] type of releaseForm is incorrect,expected type is function or boolean.");
            }
        }
        return RetErrMsg(asyncErrorInfo);
    }

    AsyncReleaseFormCallbackInfo *asyncCallbackInfo = new
    AsyncReleaseFormCallbackInfo {
        .env = env,
        .ability = GetGlobalAbility(env),
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formId = formId,
        .isReleaseCache = false,
        .result = 0,
    };

    if (argc == ARGS_SIZE_ONE) { // release promise, one argv
        return ReleaseFormPromise(env, asyncCallbackInfo);
    } else if (argc == ARGS_SIZE_THREE) { // release callback, three argv
        HILOG_INFO("%{public}s, asyncCallback.", __func__);
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_boolean, "The arguments[1] type of releaseForm is incorrect,\
            expected type is boolean.");

        napi_get_value_bool(env, argv[1], &asyncCallbackInfo->isReleaseCache);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[ARGS_SIZE_TWO], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[2] type of releaseForm is incorrect,\
            expected type is function.");
        napi_create_reference(env, argv[ARGS_SIZE_TWO], REF_COUNT, &asyncCallbackInfo->callback);
        ReleaseFormCallback(env, asyncCallbackInfo);
    } else {
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        if (valueType == napi_function) { // release callback, two argv
            napi_create_reference(env, argv[1], REF_COUNT, &asyncCallbackInfo->callback);
            ReleaseFormCallback(env, asyncCallbackInfo);
        } else if (valueType == napi_boolean) { // release promise, two argv
            return ReleaseFormPromise(env, asyncCallbackInfo);
        } else {
            NAPI_ASSERT(env, valueType == napi_function || valueType == napi_boolean,
                "The arguments[2] type of releaseForm is incorrect,expected type is function or boolean.");
        }
    }
    return NapiGetResut(env, 1);
}

/**
 * @brief  Call native kit function: RequestForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 *
 * @return void
 */
static void InnerRequestForm(napi_env env, AsyncRequestFormCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    asyncCallbackInfo->result = ability->RequestForm(asyncCallbackInfo->formId);
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: requestForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 *
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_RequestForm(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    HILOG_INFO("%{public}s, argc = [%{public}zu]", __func__, argc);

    // Check the value type of the arguments
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    if (valueType != napi_string) {
        AsyncErrMsgCallbackInfo *asyncErrorInfo = new
            AsyncErrMsgCallbackInfo {
                .env = env,
                .asyncWork = nullptr,
                .deferred = nullptr,
                .callback = nullptr,
                .code = ERR_APPEXECFWK_FORM_INVALID_FORM_ID,
                .type = 0,
                .callbackValue = argv[1]
            };

        if (argc == ARGS_SIZE_TWO) {
            asyncErrorInfo->type = CALLBACK_FLG;
        } else {
            asyncErrorInfo->type = PROMISE_FLG;
        }
        return RetErrMsg(asyncErrorInfo);
    }

    std::string strFormId = GetStringFromNAPI(env, argv[0]);
    int64_t formId;
    if (!ConvertStringToInt64(strFormId, formId)) {
        AsyncErrMsgCallbackInfo *asyncErrorInfo = new
            AsyncErrMsgCallbackInfo {
                .env = env,
                .asyncWork = nullptr,
                .deferred = nullptr,
                .callback = nullptr,
                .code = ERR_APPEXECFWK_FORM_FORM_ID_NUM_ERR,
                .type = 0,
                .callbackValue = argv[1]
            };

        if (argc == ARGS_SIZE_TWO) {
            asyncErrorInfo->type = CALLBACK_FLG;
        } else {
            asyncErrorInfo->type = PROMISE_FLG;
        }
        return RetErrMsg(asyncErrorInfo);
    }

    AsyncRequestFormCallbackInfo *asyncCallbackInfo = new
    AsyncRequestFormCallbackInfo {
        .env = env,
        .ability = GetGlobalAbility(env),
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formId = formId,
        .result = 0,
    };

    if (argc == ARGS_SIZE_TWO) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of requestForm is incorrect,\
        expected type is function.");

        napi_create_reference(env, argv[1], REF_COUNT, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);
                AsyncRequestFormCallbackInfo *asyncCallbackInfo = (AsyncRequestFormCallbackInfo *)data;
                InnerRequestForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncRequestFormCallbackInfo *asyncCallbackInfo = (AsyncRequestFormCallbackInfo *)data;
                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    InnerCreateRetMsg(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, ARGS_SIZE_ONE, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
        return NapiGetResut(env, 1);
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise running", __func__);
                AsyncRequestFormCallbackInfo *asyncCallbackInfo = (AsyncRequestFormCallbackInfo *)data;
                InnerRequestForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);
                AsyncRequestFormCallbackInfo *asyncCallbackInfo = (AsyncRequestFormCallbackInfo *)data;

                napi_value result;
                InnerCreateRetMsg(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: CastTempForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 *
 * @return void
 */
static void InnerCastTempForm(napi_env env, AsyncCastTempFormCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    asyncCallbackInfo->result = ability->CastTempForm(asyncCallbackInfo->formId);
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: castTempForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 *
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_CastTempForm(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    HILOG_INFO("%{public}s, argc = [%{public}zu]", __func__, argc);

    // Check the value type of the arguments
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    if (valueType != napi_string) {
        AsyncErrMsgCallbackInfo *asyncErrorInfo = new
            AsyncErrMsgCallbackInfo {
                .env = env,
                .asyncWork = nullptr,
                .deferred = nullptr,
                .callback = nullptr,
                .code = ERR_APPEXECFWK_FORM_INVALID_FORM_ID,
                .type = 0,
                .callbackValue = argv[1]
            };

        if (argc == ARGS_SIZE_TWO) {
            asyncErrorInfo->type = CALLBACK_FLG;
        } else {
            asyncErrorInfo->type = PROMISE_FLG;
        }
        return RetErrMsg(asyncErrorInfo);
    }

    std::string strFormId = GetStringFromNAPI(env, argv[0]);
    int64_t formId;
    if (!ConvertStringToInt64(strFormId, formId)) {
        AsyncErrMsgCallbackInfo *asyncErrorInfo = new
            AsyncErrMsgCallbackInfo {
                .env = env,
                .asyncWork = nullptr,
                .deferred = nullptr,
                .callback = nullptr,
                .code = ERR_APPEXECFWK_FORM_FORM_ID_NUM_ERR,
                .type = 0,
                .callbackValue = argv[1]
            };

        if (argc == ARGS_SIZE_TWO) {
            asyncErrorInfo->type = CALLBACK_FLG;
        } else {
            asyncErrorInfo->type = PROMISE_FLG;
        }
        return RetErrMsg(asyncErrorInfo);
    }

    AsyncCastTempFormCallbackInfo *asyncCallbackInfo = new
    AsyncCastTempFormCallbackInfo {
        .env = env,
        .ability = GetGlobalAbility(env),
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formId = formId,
        .result = 0,
    };

    if (argc == ARGS_SIZE_TWO) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of castTempForm is incorrect,\
        expected type is function.");

        napi_create_reference(env, argv[1], REF_COUNT, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);
                AsyncCastTempFormCallbackInfo *asyncCallbackInfo = (AsyncCastTempFormCallbackInfo *)data;
                InnerCastTempForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncCastTempFormCallbackInfo *asyncCallbackInfo = (AsyncCastTempFormCallbackInfo *)data;
                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    InnerCreateRetMsg(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, ARGS_SIZE_ONE, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
        return NapiGetResut(env, 1);
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise running", __func__);
                AsyncCastTempFormCallbackInfo *asyncCallbackInfo = (AsyncCastTempFormCallbackInfo *)data;
                InnerCastTempForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);
                AsyncCastTempFormCallbackInfo *asyncCallbackInfo = (AsyncCastTempFormCallbackInfo *)data;

                napi_value result;
                InnerCreateRetMsg(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: NotifyVisibleForms
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 *
 * @return void
 */
static void InnerNotifyVisibleForms(napi_env env, AsyncNotifyVisibleFormsCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    asyncCallbackInfo->result = ability->NotifyVisibleForms(asyncCallbackInfo->formIds);
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: notifyVisibleForms
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 *
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_NotifyVisibleForms(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    HILOG_INFO("%{public}s, argc = [%{public}zu]", __func__, argc);

    uint32_t arrayLength = 0;
    NAPI_CALL(env, napi_get_array_length(env, argv[0], &arrayLength));
    NAPI_ASSERT(env, arrayLength > 0, "The arguments[0] value of notifyVisibleForms is incorrect,\
        this array is empty.");

    std::vector<int64_t> formIds;
    formIds.clear();
    napi_valuetype valueType;
    for (size_t i = 0; i < arrayLength; i++) {
        napi_value napiFormId;
        napi_get_element(env, argv[0], i, &napiFormId);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, napiFormId, &valueType));
        if (valueType != napi_string) {
            AsyncErrMsgCallbackInfo *asyncErrorInfo = new
                AsyncErrMsgCallbackInfo {
                    .env = env,
                    .asyncWork = nullptr,
                    .deferred = nullptr,
                    .callback = nullptr,
                    .code = ERR_APPEXECFWK_FORM_INVALID_FORM_ID,
                    .type = 0,
                    .callbackValue = argv[1]
                };

            if (argc == ARGS_SIZE_TWO) {
                asyncErrorInfo->type = CALLBACK_FLG;
            } else {
                asyncErrorInfo->type = PROMISE_FLG;
            }
            return RetErrMsg(asyncErrorInfo);
        }

        std::string strFormId = GetStringFromNAPI(env, napiFormId);
        int64_t formIdValue;
        if (!ConvertStringToInt64(strFormId, formIdValue)) {
            AsyncErrMsgCallbackInfo *asyncErrorInfo = new
                AsyncErrMsgCallbackInfo {
                    .env = env,
                    .asyncWork = nullptr,
                    .deferred = nullptr,
                    .callback = nullptr,
                    .code = ERR_APPEXECFWK_FORM_FORM_ID_NUM_ERR,
                    .type = 0,
                    .callbackValue = argv[1]
                };

            if (argc == ARGS_SIZE_TWO) {
                asyncErrorInfo->type = CALLBACK_FLG;
            } else {
                asyncErrorInfo->type = PROMISE_FLG;
            }
            return RetErrMsg(asyncErrorInfo);
        }

        formIds.push_back(formIdValue);
    }

    AsyncNotifyVisibleFormsCallbackInfo *asyncCallbackInfo = new
        AsyncNotifyVisibleFormsCallbackInfo {
            .env = env,
            .ability = GetGlobalAbility(env),
            .asyncWork = nullptr,
            .deferred = nullptr,
            .callback = nullptr,
            .formIds = formIds,
            .result = 1,
        };

    if (argc == ARGS_SIZE_TWO) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of notifyVisibleForms is incorrect,\
        expected type is function.");

        napi_create_reference(env, argv[1], REF_COUNT, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);

                AsyncNotifyVisibleFormsCallbackInfo *asyncCallbackInfo =
                (AsyncNotifyVisibleFormsCallbackInfo *)data;

                InnerNotifyVisibleForms(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncNotifyVisibleFormsCallbackInfo *asyncCallbackInfo =
                (AsyncNotifyVisibleFormsCallbackInfo *)data;

                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    InnerCreateRetMsg(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, ARGS_SIZE_ONE, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
        return NapiGetResut(env, 1);
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise running", __func__);

                AsyncNotifyVisibleFormsCallbackInfo *asyncCallbackInfo =
                (AsyncNotifyVisibleFormsCallbackInfo *)data;

                InnerNotifyVisibleForms(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);

                AsyncNotifyVisibleFormsCallbackInfo *asyncCallbackInfo =
                (AsyncNotifyVisibleFormsCallbackInfo *)data;

                napi_value result;
                InnerCreateRetMsg(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: NotifyInvisibleForms
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 *
 * @return void
 */
static void InnerNotifyInvisibleForms(napi_env env, AsyncNotifyInvisibleFormsCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    asyncCallbackInfo->result = ability->NotifyInvisibleForms(asyncCallbackInfo->formIds);
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: notifyInvisibleForms
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 *
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_NotifyInvisibleForms(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    HILOG_INFO("%{public}s, argc = [%{public}zu]", __func__, argc);

    uint32_t arrayLength = 0;
    NAPI_CALL(env, napi_get_array_length(env, argv[0], &arrayLength));
    NAPI_ASSERT(env, arrayLength > 0, "The arguments[0] value of notifyInvisibleForms is incorrect,\
    this array is empty.");

    std::vector<int64_t> formIds;
    formIds.clear();
    napi_valuetype valueType;
    for (size_t i = 0; i < arrayLength; i++) {
        napi_value napiFormId;
        napi_get_element(env, argv[0], i, &napiFormId);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, napiFormId, &valueType));
        if (valueType != napi_string) {
            AsyncErrMsgCallbackInfo *asyncErrorInfo = new
                AsyncErrMsgCallbackInfo {
                    .env = env,
                    .asyncWork = nullptr,
                    .deferred = nullptr,
                    .callback = nullptr,
                    .code = ERR_APPEXECFWK_FORM_INVALID_FORM_ID,
                    .type = 0,
                    .callbackValue = argv[1]
                };

            if (argc == ARGS_SIZE_TWO) {
                asyncErrorInfo->type = CALLBACK_FLG;
            } else {
                asyncErrorInfo->type = PROMISE_FLG;
            }
            return RetErrMsg(asyncErrorInfo);
        }

        std::string strFormId = GetStringFromNAPI(env, napiFormId);
        int64_t formIdValue;
        if (!ConvertStringToInt64(strFormId, formIdValue)) {
            AsyncErrMsgCallbackInfo *asyncErrorInfo = new
                AsyncErrMsgCallbackInfo {
                    .env = env,
                    .asyncWork = nullptr,
                    .deferred = nullptr,
                    .callback = nullptr,
                    .code = ERR_APPEXECFWK_FORM_FORM_ID_NUM_ERR,
                    .type = 0,
                    .callbackValue = argv[1]
                };

            if (argc == ARGS_SIZE_TWO) {
                asyncErrorInfo->type = CALLBACK_FLG;
            } else {
                asyncErrorInfo->type = PROMISE_FLG;
            }
            return RetErrMsg(asyncErrorInfo);
        }

        formIds.push_back(formIdValue);
    }

    AsyncNotifyInvisibleFormsCallbackInfo *asyncCallbackInfo = new
        AsyncNotifyInvisibleFormsCallbackInfo {
            .env = env,
            .ability = GetGlobalAbility(env),
            .asyncWork = nullptr,
            .deferred = nullptr,
            .callback = nullptr,
            .formIds = formIds,
            .result = 1,
        };

    if (argc == ARGS_SIZE_TWO) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of notifyInvisibleForms is incorrect,\
        expected type is function.");

        napi_create_reference(env, argv[1], REF_COUNT, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);

                AsyncNotifyInvisibleFormsCallbackInfo *asyncCallbackInfo =
                (AsyncNotifyInvisibleFormsCallbackInfo *)data;

                InnerNotifyInvisibleForms(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncNotifyInvisibleFormsCallbackInfo *asyncCallbackInfo =
                (AsyncNotifyInvisibleFormsCallbackInfo *)data;

                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    InnerCreateRetMsg(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, ARGS_SIZE_ONE, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
        return NapiGetResut(env, 1);
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise running", __func__);

                AsyncNotifyInvisibleFormsCallbackInfo *asyncCallbackInfo =
                (AsyncNotifyInvisibleFormsCallbackInfo *)data;

                InnerNotifyInvisibleForms(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);

                AsyncNotifyInvisibleFormsCallbackInfo *asyncCallbackInfo =
                (AsyncNotifyInvisibleFormsCallbackInfo *)data;

                napi_value result;
                InnerCreateRetMsg(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: EnableUpdateForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 *
 * @return void
 */
static void InnerEnableFormsUpdate(napi_env env, AsyncEnableUpdateFormCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    asyncCallbackInfo->result = ability->EnableUpdateForm(asyncCallbackInfo->formIds);
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: enableFormsUpdate
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 *
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_EnableFormsUpdate(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    HILOG_INFO("%{public}s, argc = [%{public}zu]", __func__, argc);

    uint32_t arrayLength = 0;
    NAPI_CALL(env, napi_get_array_length(env, argv[0], &arrayLength));
    NAPI_ASSERT(env, arrayLength > 0, "The arguments[0] value of enableFormsUpdate \
        is incorrect, this array is empty.");

    std::vector<int64_t> formIds;
    formIds.clear();
    napi_valuetype valueType;
    for (size_t i = 0; i < arrayLength; i++) {
        napi_value napiFormId;
        napi_get_element(env, argv[0], i, &napiFormId);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, napiFormId, &valueType));
        if (valueType != napi_string) {
            AsyncErrMsgCallbackInfo *asyncErrorInfo = new
                AsyncErrMsgCallbackInfo {
                    .env = env,
                    .asyncWork = nullptr,
                    .deferred = nullptr,
                    .callback = nullptr,
                    .code = ERR_APPEXECFWK_FORM_INVALID_FORM_ID,
                    .type = 0,
                    .callbackValue = argv[1]
                };

            if (argc == ARGS_SIZE_TWO) {
                asyncErrorInfo->type = CALLBACK_FLG;
            } else {
                asyncErrorInfo->type = PROMISE_FLG;
            }
            return RetErrMsg(asyncErrorInfo);
        }

        std::string strFormId = GetStringFromNAPI(env, napiFormId);
        int64_t formIdValue;
        if (!ConvertStringToInt64(strFormId, formIdValue)) {
            AsyncErrMsgCallbackInfo *asyncErrorInfo = new
                AsyncErrMsgCallbackInfo {
                    .env = env,
                    .asyncWork = nullptr,
                    .deferred = nullptr,
                    .callback = nullptr,
                    .code = ERR_APPEXECFWK_FORM_FORM_ID_NUM_ERR,
                    .type = 0,
                    .callbackValue = argv[1]
                };

            if (argc == ARGS_SIZE_TWO) {
                asyncErrorInfo->type = CALLBACK_FLG;
            } else {
                asyncErrorInfo->type = PROMISE_FLG;
            }
            return RetErrMsg(asyncErrorInfo);
        }

        formIds.push_back(formIdValue);
    }

    AsyncEnableUpdateFormCallbackInfo *asyncCallbackInfo = new
    AsyncEnableUpdateFormCallbackInfo {
        .env = env,
        .ability = GetGlobalAbility(env),
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formIds = formIds,
        .result = 0,
    };

    if (argc == ARGS_SIZE_TWO) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of enableFormsUpdate \
            is incorrect, expected type is function.");

        napi_create_reference(env, argv[1], REF_COUNT, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);

                AsyncEnableUpdateFormCallbackInfo *asyncCallbackInfo =
                (AsyncEnableUpdateFormCallbackInfo *)data;

                InnerEnableFormsUpdate(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncEnableUpdateFormCallbackInfo *asyncCallbackInfo =
                (AsyncEnableUpdateFormCallbackInfo *)data;

                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    InnerCreateRetMsg(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, ARGS_SIZE_ONE, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
        return NapiGetResut(env, 1);
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise running", __func__);
                AsyncEnableUpdateFormCallbackInfo *asyncCallbackInfo =
                (AsyncEnableUpdateFormCallbackInfo *)data;

                InnerEnableFormsUpdate(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);

                AsyncEnableUpdateFormCallbackInfo *asyncCallbackInfo =
                (AsyncEnableUpdateFormCallbackInfo *)data;

                napi_value result;
                InnerCreateRetMsg(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: DisableUpdateForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 *
 * @return void
 */
static void InnerDisableFormsUpdate(napi_env env, AsyncDisableUpdateFormCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    asyncCallbackInfo->result = ability->DisableUpdateForm(asyncCallbackInfo->formIds);
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: disableFormsUpdate
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 *
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_DisableFormsUpdate(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    HILOG_INFO("%{public}s, argc = [%{public}zu]", __func__, argc);

    uint32_t arrayLength = 0;
    NAPI_CALL(env, napi_get_array_length(env, argv[0], &arrayLength));
    NAPI_ASSERT(env, arrayLength > 0, "The arguments[0] value of disableFormsUpdate \
        is incorrect, this array is empty.");

    std::vector<int64_t> formIds;
    formIds.clear();
    napi_valuetype valueType;
    for (size_t i = 0; i < arrayLength; i++) {
        napi_value napiFormId;
        napi_get_element(env, argv[0], i, &napiFormId);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, napiFormId, &valueType));
        if (valueType != napi_string) {
            AsyncErrMsgCallbackInfo *asyncErrorInfo = new
                AsyncErrMsgCallbackInfo {
                    .env = env,
                    .asyncWork = nullptr,
                    .deferred = nullptr,
                    .callback = nullptr,
                    .code = ERR_APPEXECFWK_FORM_INVALID_FORM_ID,
                    .type = 0,
                    .callbackValue = argv[1]
                };

            if (argc == ARGS_SIZE_TWO) {
                asyncErrorInfo->type = CALLBACK_FLG;
            } else {
                asyncErrorInfo->type = PROMISE_FLG;
            }
            return RetErrMsg(asyncErrorInfo);
        }

        std::string strFormId = GetStringFromNAPI(env, napiFormId);
        int64_t formIdValue;
        if (!ConvertStringToInt64(strFormId, formIdValue)) {
            AsyncErrMsgCallbackInfo *asyncErrorInfo = new
                AsyncErrMsgCallbackInfo {
                    .env = env,
                    .asyncWork = nullptr,
                    .deferred = nullptr,
                    .callback = nullptr,
                    .code = ERR_APPEXECFWK_FORM_FORM_ID_NUM_ERR,
                    .type = 0,
                    .callbackValue = argv[1]
                };

            if (argc == ARGS_SIZE_TWO) {
                asyncErrorInfo->type = CALLBACK_FLG;
            } else {
                asyncErrorInfo->type = PROMISE_FLG;
            }
            return RetErrMsg(asyncErrorInfo);
        }

        formIds.push_back(formIdValue);
    }

    AsyncDisableUpdateFormCallbackInfo *asyncCallbackInfo = new
        AsyncDisableUpdateFormCallbackInfo {
            .env = env,
            .ability = GetGlobalAbility(env),
            .asyncWork = nullptr,
            .deferred = nullptr,
            .callback = nullptr,
            .formIds = formIds,
            .result = 0,
        };

    if (argc == ARGS_SIZE_TWO) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of disableFormsUpdate \
        is incorrect, expected type is function.");

        napi_create_reference(env, argv[1], REF_COUNT, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);

                AsyncDisableUpdateFormCallbackInfo *asyncCallbackInfo =
                (AsyncDisableUpdateFormCallbackInfo *)data;

                InnerDisableFormsUpdate(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncDisableUpdateFormCallbackInfo *asyncCallbackInfo =
                (AsyncDisableUpdateFormCallbackInfo *)data;

                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    InnerCreateRetMsg(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, ARGS_SIZE_ONE, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
        return NapiGetResut(env, 1);
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise running", __func__);

                AsyncDisableUpdateFormCallbackInfo *asyncCallbackInfo =
                (AsyncDisableUpdateFormCallbackInfo *)data;

                InnerDisableFormsUpdate(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);

                AsyncDisableUpdateFormCallbackInfo *asyncCallbackInfo =
                (AsyncDisableUpdateFormCallbackInfo *)data;

                napi_value result;
                InnerCreateRetMsg(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: CheckFMSReady
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 *
 * @return void
 */
static void InnerCheckFMSReady(napi_env env, AsyncCheckFMSReadyCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    asyncCallbackInfo->isFMSReady = ability->CheckFMSReady();
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: checkFMSReady
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 *
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_CheckFMSReady(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_ONE) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    HILOG_INFO("%{public}s, argc = [%{public}zu]", __func__, argc);

    AsyncCheckFMSReadyCallbackInfo *asyncCallbackInfo = new
    AsyncCheckFMSReadyCallbackInfo {
        .env = env,
        .ability = GetGlobalAbility(env),
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .isFMSReady = false,
    };

    if (argc == ARGS_SIZE_ONE) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        napi_valuetype valueType;
        NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[0] type of checkFMSReady is incorrect,\
        expected type is function.");

        napi_create_reference(env, argv[0], REF_COUNT, &asyncCallbackInfo->callback);
        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);

                AsyncCheckFMSReadyCallbackInfo *asyncCallbackInfo =
                (AsyncCheckFMSReadyCallbackInfo *)data;

                InnerCheckFMSReady(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                AsyncCheckFMSReadyCallbackInfo *asyncCallbackInfo =
                (AsyncCheckFMSReadyCallbackInfo *)data;

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value isFMSReadyResult;
                    napi_create_int32(env, asyncCallbackInfo->isFMSReady, &isFMSReadyResult);
                    napi_value callback;
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, nullptr, callback, ARGS_SIZE_ONE, &isFMSReadyResult, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
        return NapiGetResut(env, 1);
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise running", __func__);

                AsyncCheckFMSReadyCallbackInfo *asyncCallbackInfo =
                (AsyncCheckFMSReadyCallbackInfo *)data;

                InnerCheckFMSReady(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);

                AsyncCheckFMSReadyCallbackInfo *asyncCallbackInfo =
                (AsyncCheckFMSReadyCallbackInfo *)data;

                napi_value result;
                napi_create_int32(env, asyncCallbackInfo->isFMSReady, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: GetAllFormsInfo
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 *
 * @return void
 */
static void InnerGetAllFormsInfo(napi_env env, AsyncGetFormsInfoCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    asyncCallbackInfo->result = ability->GetAllFormsInfo(asyncCallbackInfo->formInfos);
    HILOG_DEBUG("%{public}s, end", __func__);
}

// NAPI_GetAllFormsInfo callback execute
napi_async_execute_callback NAPI_GetAllFormsInfoAsyncExecute = [](napi_env env, void *data) {
    HILOG_INFO("NAPI_GetAllFormsInfo execute callback");
    AsyncGetFormsInfoCallbackInfo *asyncCallbackInfo = (AsyncGetFormsInfoCallbackInfo *)data;
    InnerGetAllFormsInfo(env, asyncCallbackInfo);
};

// NAPI_GetFormsInfo callback complete
napi_async_complete_callback NAPI_GetFormsInfoAsyncComplete = [](napi_env env, napi_status status, void *data) {
    HILOG_INFO("NAPI_GetFormsInfo compeleted callback");
    AsyncGetFormsInfoCallbackInfo *asyncCallbackInfo = (AsyncGetFormsInfoCallbackInfo *)data;
    napi_value arrayFormInfos;
    napi_create_array(env, &arrayFormInfos);
    if (asyncCallbackInfo->result == ERR_OK) {
        int iFormInfoCount = 0;
        for (auto formInfo : asyncCallbackInfo->formInfos) {
            napi_value formInfoObject = nullptr;
            napi_create_object(env, &formInfoObject);
            ParseFormInfoIntoNapi(env, formInfo, formInfoObject);
            napi_set_element(env, arrayFormInfos, iFormInfoCount, formInfoObject);
            ++iFormInfoCount;
        }
    }
    if (asyncCallbackInfo->callback != nullptr) {
        napi_value callbackValues[2] = {0};
        napi_value callback;
        napi_value resultCode;
        ErrCode errCode = asyncCallbackInfo->result;
        if (errCode == ERR_OK) {
            napi_create_int32(env, errCode, &resultCode);
            callbackValues[0] = resultCode;
            callbackValues[1] = arrayFormInfos;
        } else {
            InnerCreateRetMsg(env, errCode, callbackValues);
        }

        napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
        napi_value callResult;
        napi_call_function(env, nullptr, callback, ARGS_SIZE_TWO, callbackValues, &callResult);
        napi_delete_reference(env, asyncCallbackInfo->callback);
    }
    napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
    delete asyncCallbackInfo;
};

// NAPI_GetFormsInfo promise Complete
napi_async_complete_callback NAPI_GetFormsInfoPromiseComplete = [](napi_env env, napi_status status, void *data) {
    HILOG_INFO("%{public}s, promise complete", __func__);
    AsyncGetFormsInfoCallbackInfo *asyncCallbackInfo = (AsyncGetFormsInfoCallbackInfo *)data;
    if (asyncCallbackInfo->result == ERR_OK) {
        napi_value arrayFormInfos;
        napi_create_array(env, &arrayFormInfos);
        int iFormInfoCount = 0;
        for (auto formInfo : asyncCallbackInfo->formInfos) {
            napi_value formInfoObject = nullptr;
            napi_create_object(env, &formInfoObject);
            ParseFormInfoIntoNapi(env, formInfo, formInfoObject);
            napi_set_element(env, arrayFormInfos, iFormInfoCount, formInfoObject);
            ++iFormInfoCount;
        }
        napi_resolve_deferred(
            asyncCallbackInfo->env,
            asyncCallbackInfo->deferred,
            arrayFormInfos);
    } else {
        napi_value getFormsInfoResult;
        InnerCreateRetMsg(env, asyncCallbackInfo->result, &getFormsInfoResult);

        napi_resolve_deferred(
            asyncCallbackInfo->env,
            asyncCallbackInfo->deferred,
            getFormsInfoResult);
    }

    napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
    delete asyncCallbackInfo;
};

/**
 * @brief  The implementation of Node-API interface: getAllFormsInfo
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 *
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_GetAllFormsInfo(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_ONE) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    HILOG_INFO("%{public}s, argc = [%{public}zu]", __func__, argc);

    AsyncGetFormsInfoCallbackInfo *asyncCallbackInfo = new
    AsyncGetFormsInfoCallbackInfo {
        .env = env,
        .ability = GetGlobalAbility(env),
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formInfos = std::vector<OHOS::AppExecFwk::FormInfo>(),
        .bundleName = "",
        .moduleName = "",
        .result = 0,
    };

    if (argc == ARGS_SIZE_ONE) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        napi_valuetype valueType;
        NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[0] type of getAllFormsInfo is incorrect,\
            expected type is function.");

        napi_create_reference(env, argv[0], REF_COUNT, &asyncCallbackInfo->callback);
        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            NAPI_GetAllFormsInfoAsyncExecute,
            NAPI_GetFormsInfoAsyncComplete,
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
        return NapiGetResut(env, 1);
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            NAPI_GetAllFormsInfoAsyncExecute,
            NAPI_GetFormsInfoPromiseComplete,
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: GetFormsInfoByApp
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 *
 * @return void
 */
static void InnerGetFormsInfoByApp(napi_env env, AsyncGetFormsInfoCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    asyncCallbackInfo->result = ability->GetFormsInfoByApp(
        asyncCallbackInfo->bundleName,
        asyncCallbackInfo->formInfos);
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  Call native kit function: GetFormsInfoByModule
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 *
 * @return void
 */
static void InnerGetFormsInfoByModule(napi_env env, AsyncGetFormsInfoCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    asyncCallbackInfo->result = ability->GetFormsInfoByModule(
        asyncCallbackInfo->bundleName,
        asyncCallbackInfo->moduleName,
        asyncCallbackInfo->formInfos);
    HILOG_DEBUG("%{public}s, end", __func__);
}

// NAPI_GetFormsInfo byModule callback execute
auto NAPI_GetFormsInfoByModuleAsyncExecute = [](napi_env env, void *data) {
    HILOG_INFO("NAPI_GetFormsInfo byModule execute callback");
    AsyncGetFormsInfoCallbackInfo *asyncCallbackInfo =
        (AsyncGetFormsInfoCallbackInfo *)data;
    InnerGetFormsInfoByModule(env, asyncCallbackInfo);
};

// NAPI_GetFormsInfo byApp callback execute
auto NAPI_GetFormsInfoByAppAsyncExecute = [](napi_env env, void *data) {
    HILOG_INFO("NAPI_GetFormsInfo byApp execute callback");
    AsyncGetFormsInfoCallbackInfo *asyncCallbackInfo =
        (AsyncGetFormsInfoCallbackInfo *)data;
    InnerGetFormsInfoByApp(env, asyncCallbackInfo);
};

// GetFormsInfo callback
napi_value GetFormsInfoCallback(napi_env env, AsyncGetFormsInfoCallbackInfo *asyncCallbackInfo, bool isByApp)
{
    HILOG_INFO("%{public}s, callback.", __func__);
    napi_value resourceName;
    napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
    napi_create_async_work(
        env,
        nullptr,
        resourceName,
        isByApp ? NAPI_GetFormsInfoByAppAsyncExecute :
        NAPI_GetFormsInfoByModuleAsyncExecute,
        NAPI_GetFormsInfoAsyncComplete,
        (void *)asyncCallbackInfo,
        &asyncCallbackInfo->asyncWork);
    NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
    return NapiGetResut(env, 1);
}

// GetFormsInfo promise
napi_value GetFormsInfoPromise(napi_env env, AsyncGetFormsInfoCallbackInfo *asyncCallbackInfo, bool isByApp)
{
    HILOG_INFO("%{public}s, promise.", __func__);
    napi_deferred deferred;
    napi_value promise;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    asyncCallbackInfo->deferred = deferred;
    napi_value resourceName;
    napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
    napi_create_async_work(
        env,
        nullptr,
        resourceName,
        isByApp ? NAPI_GetFormsInfoByAppAsyncExecute : NAPI_GetFormsInfoByModuleAsyncExecute,
        NAPI_GetFormsInfoPromiseComplete,
        (void *)asyncCallbackInfo,
        &asyncCallbackInfo->asyncWork);
    napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
    return promise;
}

// GetFormsInfo THREE ARGV
napi_value GetFormsInfoThreeArgv(napi_env env, napi_value *argv, AsyncGetFormsInfoCallbackInfo* const asyncCallbackInfo)
{
    HILOG_INFO("%{public}s.", __func__);

    // Check the value type of the arguments
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[ARGS_SIZE_ONE], &valueType));
    std::string moduleNameInfo = GetStringFromNAPI(env, argv[ARGS_SIZE_ONE]);
    HILOG_INFO("%{public}s, moduleNameInfo=%{public}s.", __func__, moduleNameInfo.c_str());
    asyncCallbackInfo->moduleName = moduleNameInfo;

    // Check the value type of the arguments
    valueType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[ARGS_SIZE_TWO], &valueType));
    NAPI_ASSERT(env, valueType == napi_function, "The arguments[2] type of getFormsInfo is incorrect,\
        expected type is function.");
    napi_create_reference(env, argv[ARGS_SIZE_TWO], REF_COUNT, &asyncCallbackInfo->callback);
    return GetFormsInfoCallback(env, asyncCallbackInfo, false);
}

// GetFormsInfo TWO ARGV
napi_value GetFormsInfoTwoArgv(napi_env env, napi_value *argv, AsyncGetFormsInfoCallbackInfo* const asyncCallbackInfo)
{
    HILOG_INFO("%{public}s.", __func__);

    // Check the value type of the arguments
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[ARGS_SIZE_ONE], &valueType));

    // GetFormsInfoByModule promise
    if (valueType == napi_string) {
        std::string moduleNameInfo = GetStringFromNAPI(env, argv[ARGS_SIZE_ONE]);
        HILOG_INFO("%{public}s, moduleNameInfo=%{public}s.", __func__, moduleNameInfo.c_str());
        asyncCallbackInfo->moduleName = moduleNameInfo;
        return GetFormsInfoPromise(env, asyncCallbackInfo, false);
    } else if (valueType == napi_function) { // GetFormsInfoByApp callback
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[ARGS_SIZE_ONE], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of getFormsInfo is incorrect,\
            expected type is function.");
        napi_create_reference(env, argv[ARGS_SIZE_ONE], REF_COUNT, &asyncCallbackInfo->callback);
        return GetFormsInfoCallback(env, asyncCallbackInfo, true);
    } else {
        NAPI_ASSERT(env, false, "The arguments[1] type of getFormsInfo is incorrect,\
            expected type is string or function.");
        return NapiGetResut(env, 1);
    }
}

/**
 * @brief  The implementation of Node-API interface: getFormsInfo
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 *
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_GetFormsInfo(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_THREE;
    napi_value argv[ARGS_SIZE_THREE] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_THREE) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    HILOG_INFO("%{public}s, argc = [%{public}zu]", __func__, argc);

    // Check the value type of the arguments
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[ARGS_SIZE_ZERO], &valueType));
    std::string bundleNameInfo = GetStringFromNAPI(env, argv[ARGS_SIZE_ZERO]);
    HILOG_INFO("%{public}s, bundleName=%{public}s.", __func__, bundleNameInfo.c_str());

    AsyncGetFormsInfoCallbackInfo *asyncCallbackInfo = new
    AsyncGetFormsInfoCallbackInfo {
        .env = env,
        .ability = GetGlobalAbility(env),
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formInfos = std::vector<OHOS::AppExecFwk::FormInfo>(),
        .bundleName = bundleNameInfo,
        .moduleName = "",
        .result = 0,
    };

    if (argc == ARGS_SIZE_THREE) { // GetFormsInfoByModule callback
        HILOG_INFO("%{public}s, ARGS_SIZE_THREE.", __func__);
        return GetFormsInfoThreeArgv(env, argv, asyncCallbackInfo);
    } else if (argc == ARGS_SIZE_TWO) {
        HILOG_INFO("%{public}s, ARGS_SIZE_TWO.", __func__);
        return GetFormsInfoTwoArgv(env, argv, asyncCallbackInfo);
    } else if (argc == ARGS_SIZE_ONE) { // GetFormsInfoByApp promise
        return GetFormsInfoPromise(env, asyncCallbackInfo, true);
    }
    return NapiGetResut(env, 1);
}

