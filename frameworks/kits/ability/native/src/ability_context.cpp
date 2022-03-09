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

#include "ability_context.h"

#include "ability_manager_client.h"
#include "bundle_constants.h"
#include "hilog_wrapper.h"
#include "iservice_registry.h"
#include "resource_manager.h"
#include "sys_mgr_client.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace AppExecFwk {

int AbilityContext::ABILITY_CONTEXT_DEFAULT_REQUEST_CODE(0);
const std::string GRANT_ABILITY_BUNDLE_NAME = "com.ohos.permissionmanager";
const std::string GRANT_ABILITY_ABILITY_NAME = "com.ohos.permissionmanager.GrantAbility";
const std::string PERMISSION_KEY = "ohos.user.grant.permission";

ErrCode AbilityContext::StartAbility(const AAFwk::Want &want, int requestCode)
{
    HILOG_INFO("AbilityContext::StartAbility called, requestCode = %{public}d", requestCode);

    AppExecFwk::AbilityType type = GetAbilityInfoType();
    if (type != AppExecFwk::AbilityType::PAGE && type != AppExecFwk::AbilityType::SERVICE) {
        HILOG_ERROR("AbilityContext::StartAbility AbilityType = %{public}d", type);
        return ERR_INVALID_VALUE;
    }

    HILOG_INFO("%{public}s. Start calling ams->StartAbility.", __func__);
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->StartAbility(want, token_, requestCode);
    HILOG_INFO("%{public}s. End calling ams->StartAbility. ret=%{public}d", __func__, err);
    if (err != ERR_OK) {
        HILOG_ERROR("AbilityContext::StartAbility is failed %{public}d", err);
    }
    return err;
}

ErrCode AbilityContext::StartAbility(const Want &want, int requestCode, const AbilityStartSetting &abilityStartSetting)
{
    AppExecFwk::AbilityType type = GetAbilityInfoType();
    if (AppExecFwk::AbilityType::PAGE != type && AppExecFwk::AbilityType::SERVICE != type) {
        HILOG_ERROR("AbilityContext::StartAbility AbilityType = %{public}d", type);
        return ERR_INVALID_VALUE;
    }

    HILOG_INFO("%{public}s. Start calling ams->StartAbility.", __func__);
    ErrCode err =
        AAFwk::AbilityManagerClient::GetInstance()->StartAbility(want, abilityStartSetting, token_, requestCode);
    HILOG_INFO("%{public}s. End calling ams->StartAbility. ret=%{public}d", __func__, err);
    if (err != ERR_OK) {
        HILOG_ERROR("AbilityContext::StartAbility is failed %{public}d", err);
    }

    return err;
}

ErrCode AbilityContext::TerminateAbility(int requestCode)
{
    HILOG_INFO("%{public}s begin.", __func__);
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->TerminateAbility(token_, requestCode);
    if (err != ERR_OK) {
        HILOG_ERROR("AbilityContext::TerminateAbility is failed %{public}d", err);
    }
    HILOG_INFO("%{public}s end.", __func__);
    return err;
}

ErrCode AbilityContext::TerminateAbility()
{
    HILOG_INFO("%{public}s begin.", __func__);
    std::shared_ptr<AbilityInfo> info = GetAbilityInfo();
    if (info == nullptr) {
        HILOG_ERROR("AbilityContext::TerminateAbility info == nullptr");
        return ERR_NULL_OBJECT;
    }

    ErrCode err = ERR_OK;

    switch (info->type) {
        case AppExecFwk::AbilityType::PAGE:
            HILOG_INFO("%{public}s begin ams->TerminateAbility for PAGE.", __func__);
            err = AAFwk::AbilityManagerClient::GetInstance()->TerminateAbility(token_, resultCode_, &resultWant_);
            HILOG_INFO("%{public}s end ams->TerminateAbility for PAGE, ret=%{public}d", __func__, err);
            break;
        case AppExecFwk::AbilityType::SERVICE:
            HILOG_INFO("%{public}s begin ams->TerminateAbility for SERVICE.", __func__);
            err = AAFwk::AbilityManagerClient::GetInstance()->TerminateAbility(token_, -1, nullptr);
            HILOG_INFO("%{public}s end ams->TerminateAbility for SERVICE, ret=%{public}d", __func__, err);
            break;
        default:
            HILOG_ERROR("AbilityContext::TerminateAbility info type error is %{public}d", info->type);
            break;
    }

    if (err != ERR_OK) {
        HILOG_ERROR("AbilityContext::TerminateAbility is failed %{public}d", err);
    }
    HILOG_INFO("%{public}s end.", __func__);
    return err;
}

std::string AbilityContext::GetCallingBundle()
{
    return callingBundleName_;
}

std::shared_ptr<ElementName> AbilityContext::GetElementName()
{
    HILOG_INFO("%{public}s begin.", __func__);
    std::shared_ptr<AbilityInfo> info = GetAbilityInfo();
    if (info == nullptr) {
        HILOG_ERROR("AbilityContext::GetElementName info == nullptr");
        return nullptr;
    }

    std::shared_ptr<ElementName> elementName = std::make_shared<ElementName>();
    if (elementName == nullptr) {
        HILOG_ERROR("AbilityContext::GetElementName elementName == nullptr");
        return nullptr;
    }
    elementName->SetAbilityName(info->name);
    elementName->SetBundleName(info->bundleName);
    elementName->SetDeviceID(info->deviceId);
    HILOG_INFO("%{public}s end.", __func__);
    return elementName;
}

std::shared_ptr<ElementName> AbilityContext::GetCallingAbility()
{
    HILOG_INFO("%{public}s begin.", __func__);
    std::shared_ptr<ElementName> elementName = std::make_shared<ElementName>();

    if (elementName == nullptr) {
        HILOG_ERROR("AbilityContext::GetElementName elementName == nullptr");
        return nullptr;
    }
    elementName->SetAbilityName(callingAbilityName_);
    elementName->SetBundleName(callingBundleName_);
    elementName->SetDeviceID(callingDeviceId_);
    HILOG_INFO("%{public}s end.", __func__);
    return elementName;
}

bool AbilityContext::ConnectAbility(const Want &want, const sptr<AAFwk::IAbilityConnection> &conn)
{
    HILOG_INFO("%{public}s begin.", __func__);

    AppExecFwk::AbilityType type = GetAbilityInfoType();

    std::shared_ptr<AbilityInfo> abilityInfo = GetAbilityInfo();
    if (abilityInfo == nullptr) {
        HILOG_ERROR("AbilityContext::ConnectAbility info == nullptr");
        return false;
    }

    if (AppExecFwk::AbilityType::PAGE != type && AppExecFwk::AbilityType::SERVICE != type) {
        HILOG_ERROR("AbilityContext::ConnectAbility AbilityType = %{public}d", type);
        return false;
    }

    ErrCode ret = AAFwk::AbilityManagerClient::GetInstance()->ConnectAbility(want, conn, token_);
    HILOG_INFO("%{public}s end ConnectAbility, ret=%{public}d", __func__, ret);
    bool value = ((ret == ERR_OK) ? true : false);
    if (!value) {
        HILOG_ERROR("AbilityContext::ConnectAbility ErrorCode = %{public}d", ret);
    }
    HILOG_INFO("%{public}s end.", __func__);
    return value;
}

ErrCode AbilityContext::DisconnectAbility(const sptr<AAFwk::IAbilityConnection> &conn)
{
    HILOG_INFO("%{public}s begin.", __func__);

    AppExecFwk::AbilityType type = GetAbilityInfoType();
    if (AppExecFwk::AbilityType::PAGE != type && AppExecFwk::AbilityType::SERVICE != type) {
        HILOG_ERROR("AbilityContext::DisconnectAbility AbilityType = %{public}d", type);
        return ERR_INVALID_VALUE;
    }

    ErrCode ret = AAFwk::AbilityManagerClient::GetInstance()->DisconnectAbility(conn);
    HILOG_INFO("%{public}s end ams->DisconnectAbility, ret=%{public}d", __func__, ret);
    if (ret != ERR_OK) {
        HILOG_ERROR("AbilityContext::DisconnectAbility error");
    }
    HILOG_DEBUG("AbilityContext::DisconnectAbility end");
    return ret;
}

bool AbilityContext::StopAbility(const AAFwk::Want &want)
{
    HILOG_INFO("%{public}s begin.", __func__);
    AppExecFwk::AbilityType type = GetAbilityInfoType();
    if (AppExecFwk::AbilityType::PAGE != type && AppExecFwk::AbilityType::SERVICE != type) {
        HILOG_ERROR("AbilityContext::StopAbility AbilityType = %{public}d", type);
        return false;
    }

    HILOG_INFO("%{public}s begin ams->StopServiceAbility", __func__);
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->StopServiceAbility(want);
    HILOG_INFO("%{public}s end ams->StopServiceAbility, ret=%{public}d", __func__, err);
    if (err != ERR_OK) {
        HILOG_ERROR("AbilityContext::StopAbility is failed %{public}d", err);
        return false;
    }

    HILOG_INFO("%{public}s end.", __func__);
    return true;
}

sptr<IRemoteObject> AbilityContext::GetToken()
{
    return token_;
}

std::shared_ptr<ApplicationInfo> AbilityContext::GetApplicationInfo() const
{
    return ContextContainer::GetApplicationInfo();
}

std::string AbilityContext::GetCacheDir()
{
    return ContextContainer::GetCacheDir();
}

std::string AbilityContext::GetCodeCacheDir()
{
    return ContextContainer::GetCodeCacheDir();
}

std::string AbilityContext::GetDatabaseDir()
{
    return ContextContainer::GetDatabaseDir();
}

std::string AbilityContext::GetDataDir()
{
    return ContextContainer::GetDataDir();
}

std::string AbilityContext::GetDir(const std::string &name, int mode)
{
    return ContextContainer::GetDir(name, mode);
}

sptr<IBundleMgr> AbilityContext::GetBundleManager() const
{
    return ContextContainer::GetBundleManager();
}

std::string AbilityContext::GetBundleCodePath()
{
    return ContextContainer::GetBundleCodePath();
}

std::string AbilityContext::GetBundleName()
{
    return ContextContainer::GetBundleName();
}

std::string AbilityContext::GetBundleResourcePath()
{
    return ContextContainer::GetBundleResourcePath();
}

std::shared_ptr<Context> AbilityContext::GetApplicationContext() const
{
    return ContextContainer::GetApplicationContext();
}

std::shared_ptr<Context> AbilityContext::GetContext()
{
    return ContextContainer::GetContext();
}

sptr<AAFwk::IAbilityManager> AbilityContext::GetAbilityManager()
{
    return ContextContainer::GetAbilityManager();
}

std::shared_ptr<ProcessInfo> AbilityContext::GetProcessInfo() const
{
    return ContextContainer::GetProcessInfo();
}

std::string AbilityContext::GetAppType()
{
    return ContextContainer::GetAppType();
}

const std::shared_ptr<AbilityInfo> AbilityContext::GetAbilityInfo()
{
    return ContextContainer::GetAbilityInfo();
}

std::shared_ptr<HapModuleInfo> AbilityContext::GetHapModuleInfo()
{
    return ContextContainer::GetHapModuleInfo();
}

AppExecFwk::AbilityType AbilityContext::GetAbilityInfoType()
{
    std::shared_ptr<AbilityInfo> info = GetAbilityInfo();
    if (info == nullptr) {
        HILOG_ERROR("AbilityContext::GetAbilityInfoType info == nullptr");
        return AppExecFwk::AbilityType::UNKNOWN;
    }

    return info->type;
}

std::shared_ptr<Context> AbilityContext::CreateBundleContext(std::string bundleName, int flag, int accountId)
{
    return ContextContainer::CreateBundleContext(bundleName, flag, accountId);
}

std::shared_ptr<Global::Resource::ResourceManager> AbilityContext::GetResourceManager() const
{
    HILOG_INFO("%{public}s begin.", __func__);
    std::shared_ptr<Context> appcontext = GetApplicationContext();
    if (appcontext == nullptr) {
        HILOG_ERROR("AbilityContext::GetResourceManager appcontext is nullptr");
        return nullptr;
    }

    HILOG_INFO("%{public}s begin appcontext->GetResourceManager.", __func__);
    std::shared_ptr<Global::Resource::ResourceManager> resourceManager = appcontext->GetResourceManager();
    HILOG_INFO("%{public}s end appcontext->GetResourceManager.", __func__);
    if (resourceManager == nullptr) {
        HILOG_ERROR("AbilityContext::GetResourceManager resourceManager is nullptr");
        return nullptr;
    }
    HILOG_INFO("%{public}s end.", __func__);
    return resourceManager;
}

int AbilityContext::VerifySelfPermission(const std::string &permission)
{
    HILOG_INFO("%{public}s begin. permission=%{public}s", __func__, permission.c_str());
    if (permission.empty()) {
        HILOG_ERROR("VerifySelfPermission permission invalid");
        return AppExecFwk::Constants::PERMISSION_NOT_GRANTED;
    }

    std::string bundle_name = GetBundleName();
    if (bundle_name.empty()) {
        HILOG_ERROR("VerifySelfPermission failed to get bundle name error");
        return AppExecFwk::Constants::PERMISSION_NOT_GRANTED;
    }

    sptr<IBundleMgr> ptr = GetBundleManager();
    if (ptr == nullptr) {
        HILOG_ERROR("VerifySelfPermission failed to get bundle manager service");
        return AppExecFwk::Constants::PERMISSION_NOT_GRANTED;
    }

    HILOG_INFO("%{public}s start bms->CheckPermission. bundle_name=%{public}s", __func__, bundle_name.c_str());
    int ret = ptr->CheckPermission(bundle_name, permission);
    HILOG_INFO("%{public}s end bms->CheckPermission, ret=%{public}d", __func__, ret);
    HILOG_INFO("%{public}s end.", __func__);
    return ret;
}

int AbilityContext::VerifyCallingPermission(const std::string &permission)
{
    HILOG_INFO("%{public}s begin. permission=%{public}s", __func__, permission.c_str());
    if (permission.empty()) {
        HILOG_ERROR("VerifyCallingPermission permission invalid");
        return AppExecFwk::Constants::PERMISSION_NOT_GRANTED;
    }

    std::string bundle_name = GetCallingBundle();
    if (bundle_name.empty()) {
        HILOG_ERROR("VerifyCallingPermission failed to get bundle name by uid");
        return AppExecFwk::Constants::PERMISSION_NOT_GRANTED;
    }

    sptr<IBundleMgr> ptr = GetBundleManager();
    if (ptr == nullptr) {
        HILOG_ERROR("VerifyCallingPermission failed to get bundle manager service");
        return AppExecFwk::Constants::PERMISSION_NOT_GRANTED;
    }

    HILOG_INFO("%{public}s start bms->CheckPermission. bundle_name=%{public}s", __func__, bundle_name.c_str());
    int ret = ptr->CheckPermission(bundle_name, permission);
    HILOG_INFO("%{public}s end bms->CheckPermission, ret=%{public}d", __func__, ret);
    HILOG_INFO("%{public}s end.", __func__);
    return ret;
}

bool AbilityContext::CanRequestPermission(const std::string &permission)
{
    HILOG_INFO("%{public}s begin. permission=%{public}s", __func__, permission.c_str());
    if (permission.empty()) {
        HILOG_ERROR("CanRequestPermission permission invalid");
        return true;
    }

    std::string bundle_name = GetBundleName();
    if (bundle_name.empty()) {
        HILOG_ERROR("CanRequestPermission failed to get bundle name error");
        return true;
    }

    sptr<IBundleMgr> ptr = GetBundleManager();
    if (ptr == nullptr) {
        HILOG_ERROR("CanRequestPermission failed to get bundle manager service");
        return true;
    }

    HILOG_INFO("%{public}s start bms->CanRequestPermission. bundle_name=%{public}s", __func__, bundle_name.c_str());
    bool ret = ptr->CanRequestPermission(bundle_name, permission, 0);
    HILOG_INFO("%{public}s end bms->CanRequestPermission, ret=%{public}s", __func__, ret ? "true" : "false");
    HILOG_INFO("%{public}s end.", __func__);
    return ret;
}

int AbilityContext::VerifyCallingOrSelfPermission(const std::string &permission)
{
    return VerifySelfPermission(permission);
}

int AbilityContext::VerifyPermission(const std::string &permission, int pid, int uid)
{
    HILOG_INFO("%{public}s begin. permission=%{public}s, pid=%{public}d, uid=%{public}d",
        __func__,
        permission.c_str(),
        pid,
        uid);
    if (permission.empty()) {
        HILOG_ERROR("VerifyPermission permission invalid");
        return AppExecFwk::Constants::PERMISSION_NOT_GRANTED;
    }

    sptr<IBundleMgr> ptr = GetBundleManager();
    if (ptr == nullptr) {
        HILOG_ERROR("VerifyPermission failed to get bundle manager service");
        return AppExecFwk::Constants::PERMISSION_NOT_GRANTED;
    }

    std::string bundle_name;
    if (!ptr->GetBundleNameForUid(uid, bundle_name)) {
        HILOG_ERROR("VerifyPermission failed to get bundle name by uid");
        return AppExecFwk::Constants::PERMISSION_NOT_GRANTED;
    }

    HILOG_INFO("%{public}s start bms->CheckPermission. bundle_name=%{public}s", __func__, bundle_name.c_str());
    int ret = ptr->CheckPermission(bundle_name, permission);
    HILOG_INFO("%{public}s end bms->CheckPermission, ret=%{public}d", __func__, ret);
    HILOG_INFO("%{public}s end.", __func__);
    return ret;
}

void AbilityContext::GetPermissionDes(const std::string &permissionName, std::string &des)
{
    sptr<IBundleMgr> ptr = GetBundleManager();
    if (ptr == nullptr) {
        HILOG_ERROR("GetPermissionDes failed to get bundle manager service");
        return;
    }

    PermissionDef permissionDef;
    HILOG_INFO("%{public}s start bms->GetPermissionDef. permissionName=%{public}s", __func__, permissionName.c_str());
    if (ptr->GetPermissionDef(permissionName, permissionDef)) {
        des = permissionDef.description;
    }
    HILOG_INFO("%{public}s end bms->GetPermissionDef.", __func__);
}

void AbilityContext::RequestPermissionsFromUser(std::vector<std::string> &permissions, int requestCode)
{
    HILOG_INFO("%{public}s begin.", __func__);
    if (permissions.size() == 0) {
        HILOG_ERROR("AbilityContext::RequestPermissionsFromUser permissions is empty");
        return;
    }

    if (requestCode < 0) {
        HILOG_ERROR("AbilityContext::RequestPermissionsFromUser requestCode should be >= 0");
        return;
    }

    AAFwk::Want want;
    want.SetElementName(GRANT_ABILITY_BUNDLE_NAME, GRANT_ABILITY_ABILITY_NAME);
    want.SetParam(PERMISSION_KEY, permissions);
    StartAbility(want, requestCode);
    HILOG_INFO("%{public}s end.", __func__);
}

bool AbilityContext::DeleteFile(const std::string &fileName)
{
    return ContextContainer::DeleteFile(fileName);
}

void AbilityContext::SetCallingContext(
    const std::string &deviceId, const std::string &bundleName, const std::string &abilityName)
{
    callingDeviceId_ = deviceId;
    callingBundleName_ = bundleName;
    callingAbilityName_ = abilityName;
}

Uri AbilityContext::GetCaller()
{
    return ContextContainer::GetCaller();
}

void AbilityContext::AttachBaseContext(const std::shared_ptr<Context> &base)
{
    HILOG_INFO("AbilityContext::AttachBaseContext. Start.");
    ContextContainer::AttachBaseContext(base);
    HILOG_INFO("AbilityContext::AttachBaseContext. End.");
}

std::string AbilityContext::GetExternalCacheDir()
{
    return ContextContainer::GetExternalCacheDir();
}

std::string AbilityContext::GetExternalFilesDir(std::string &type)
{
    return ContextContainer::GetExternalFilesDir(type);
}

std::string AbilityContext::GetFilesDir()
{
    return ContextContainer::GetFilesDir();
}

std::string AbilityContext::GetNoBackupFilesDir()
{
    return ContextContainer::GetNoBackupFilesDir();
}

void AbilityContext::UnauthUriPermission(const std::string &permission, const Uri &uri, int uid)
{
    ContextContainer::UnauthUriPermission(permission, uri, uid);
}

std::string AbilityContext::GetDistributedDir()
{
    return ContextContainer::GetDistributedDir();
}

void AbilityContext::SetPattern(int patternId)
{
    ContextContainer::SetPattern(patternId);
}

std::shared_ptr<Context> AbilityContext::GetAbilityPackageContext()
{
    return ContextContainer::GetAbilityPackageContext();
}

std::string AbilityContext::GetProcessName()
{
    return ContextContainer::GetProcessName();
}

void AbilityContext::InitResourceManager(BundleInfo &bundleInfo, std::shared_ptr<ContextDeal> &deal)
{
    ContextContainer::InitResourceManager(bundleInfo, deal);
}

std::string AbilityContext::GetString(int resId)
{
    return ContextContainer::GetString(resId);
}

std::vector<std::string> AbilityContext::GetStringArray(int resId)
{
    return ContextContainer::GetStringArray(resId);
}

std::vector<int> AbilityContext::GetIntArray(int resId)
{
    return ContextContainer::GetIntArray(resId);
}

std::map<std::string, std::string> AbilityContext::GetTheme()
{
    return ContextContainer::GetTheme();
}

void AbilityContext::SetTheme(int themeId)
{
    ContextContainer::SetTheme(themeId);
}

std::map<std::string, std::string> AbilityContext::GetPattern()
{
    return ContextContainer::GetPattern();
}

int AbilityContext::GetColor(int resId)
{
    return ContextContainer::GetColor(resId);
}

int AbilityContext::GetThemeId()
{
    return ContextContainer::GetThemeId();
}

bool AbilityContext::TerminateAbilityResult(int startId)
{
    HILOG_INFO("%{public}s begin.", __func__);
    auto abilityClient = AAFwk::AbilityManagerClient::GetInstance();
    if (abilityClient == nullptr) {
        HILOG_ERROR("AbilityContext::TerminateAbilityResult abilityClient is nullptr");
        return false;
    }

    HILOG_INFO("%{public}s begin ams->TerminateAbilityResult, startId=%{public}d.", __func__, startId);
    ErrCode errval = abilityClient->TerminateAbilityResult(token_, startId);
    HILOG_INFO("%{public}s end ams->TerminateAbilityResult, ret=%{public}d.", __func__, errval);
    if (errval != ERR_OK) {
        HILOG_ERROR("AbilityContext::TerminateAbilityResult TerminateAbilityResult retval is %d", errval);
    }

    HILOG_INFO("%{public}s end.", __func__);
    return (errval == ERR_OK) ? true : false;
}

int AbilityContext::GetDisplayOrientation()
{
    return ContextContainer::GetDisplayOrientation();
}

std::string AbilityContext::GetPreferencesDir()
{
    return ContextContainer::GetPreferencesDir();
}

void AbilityContext::SetColorMode(int mode)
{
    ContextContainer::SetColorMode(mode);
}

int AbilityContext::GetColorMode()
{
    return ContextContainer::GetColorMode();
}

int AbilityContext::GetMissionId()
{
    return ContextContainer::GetMissionId();
}

void AbilityContext::TerminateAndRemoveMission()
{
    ContextContainer::TerminateAndRemoveMission();
}

void AbilityContext::StartAbilities(const std::vector<AAFwk::Want> &wants)
{
    HILOG_INFO("%{public}s begin.", __func__);
    for (auto want : wants) {
        StartAbility(want, ABILITY_CONTEXT_DEFAULT_REQUEST_CODE);
    }
    HILOG_INFO("%{public}s end.", __func__);
}

bool AbilityContext::IsFirstInMission()
{
    HILOG_INFO("%{public}s begin.", __func__);
    auto abilityClient = AAFwk::AbilityManagerClient::GetInstance();
    if (abilityClient == nullptr) {
        HILOG_ERROR("AbilityContext::IsFirstInMission abilityClient is nullptr");
        return false;
    }
    HILOG_INFO("%{public}s begin ams->IsFirstInMission.", __func__);
    ErrCode errval = abilityClient->IsFirstInMission(token_);
    HILOG_INFO("%{public}s end ams->IsFirstInMission, ret=%{public}d", __func__, errval);
    if (errval != ERR_OK) {
        HILOG_ERROR("AbilityContext::IsFirstInMission IsFirstInMission retval is %d", errval);
    }
    HILOG_INFO("%{public}s end.", __func__);

    return (errval == ERR_OK) ? true : false;
}

std::shared_ptr<TaskDispatcher> AbilityContext::GetUITaskDispatcher()
{
    return ContextContainer::GetUITaskDispatcher();
}

std::shared_ptr<TaskDispatcher> AbilityContext::GetMainTaskDispatcher()
{
    return ContextContainer::GetMainTaskDispatcher();
}

std::shared_ptr<TaskDispatcher> AbilityContext::CreateParallelTaskDispatcher(
    const std::string &name, const TaskPriority &priority)
{
    return ContextContainer::CreateParallelTaskDispatcher(name, priority);
}

std::shared_ptr<TaskDispatcher> AbilityContext::CreateSerialTaskDispatcher(
    const std::string &name, const TaskPriority &priority)
{
    return ContextContainer::CreateSerialTaskDispatcher(name, priority);
}

std::shared_ptr<TaskDispatcher> AbilityContext::GetGlobalTaskDispatcher(const TaskPriority &priority)
{
    return ContextContainer::GetGlobalTaskDispatcher(priority);
}

bool AbilityContext::MoveMissionToEnd(bool nonFirst)
{
    return ContextContainer::MoveMissionToEnd(nonFirst);
}

void AbilityContext::LockMission()
{
    ContextContainer::LockMission();
}

void AbilityContext::UnlockMission()
{
    ContextContainer::UnlockMission();
}

bool AbilityContext::SetMissionInformation(const MissionInformation &missionInformation)
{
    return ContextContainer::SetMissionInformation(missionInformation);
}

void AbilityContext::SetShowOnLockScreen(bool isAllow)
{
    ContextContainer::SetShowOnLockScreen(isAllow);
}

bool AbilityContext::IsUpdatingConfigurations()
{
    return ContextContainer::IsUpdatingConfigurations();
}

bool AbilityContext::PrintDrawnCompleted()
{
    return ContextContainer::PrintDrawnCompleted();
}
}  // namespace AppExecFwk
}  // namespace OHOS
