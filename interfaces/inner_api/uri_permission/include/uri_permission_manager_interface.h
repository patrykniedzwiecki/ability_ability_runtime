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

#ifndef OHOS_ABILITY_RUNTIME_URI_PERMISSION_MANAGER_INTERFACE_H
#define OHOS_ABILITY_RUNTIME_URI_PERMISSION_MANAGER_INTERFACE_H

#include "base/security/access_token/interfaces/innerkits/accesstoken/include/access_token.h"
#include "iremote_broker.h"
#include "uri.h"

namespace OHOS {
namespace AAFwk {
class IUriPermissionManager : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ability.UriPermissionManager");
    
    /**
     * @brief Authorize the uri permission of fromBundleName to targetBundleName.
     *
     * @param uri The file uri.
     * @param flag Want::FLAG_AUTH_READ_URI_PERMISSION or Want::FLAG_AUTH_WRITE_URI_PERMISSION.
     * @param fromBundleName The owner of uri.
     * @param targetBundleName The user of uri.
     * @param autoremove the uri is temperarily or not
     * @return Returns true if the authorization is successful, otherwise returns false.
     */
    virtual bool GrantUriPermission(const Uri &uri, unsigned int flag,
        const std::string fromBundleName,
        const std::string targetBundleName,
        int autoremove) = 0;
    
    /**
     * @brief Authorize the uri permission of self to targetBundleName.
     *
     * @param uri The file uri.
     * @param flag Want::FLAG_AUTH_READ_URI_PERMISSION or Want::FLAG_AUTH_WRITE_URI_PERMISSION.
     * @param targetBundleName The user of uri.
     * @return Returns true if the authorization is successful, otherwise returns false.
     */
    virtual bool GrantUriPermissionFromSelf(const Uri &uri, unsigned int flag,
        const std::string targetBundleName) = 0;

    /**
     * @brief Check whether the tokenId has URI permissions.
     *
     * @param uri The file uri.
     * @param flag Want::FLAG_AUTH_READ_URI_PERMISSION or Want::FLAG_AUTH_WRITE_URI_PERMISSION.
     * @param tokenId The user of uri.
     * @return Returns true if the verification is successful, otherwise returns false.
     */
    virtual bool VerifyUriPermission(const Uri &uri, unsigned int flag,
        const Security::AccessToken::AccessTokenID tokenId) = 0;

    /**
     * @brief Clear user's uri authorization record with autoremove flag.
     *
     * @param tokenId A tokenId of an application.
     * @return Returns true if the remove is successful, otherwise returns false.
     */
    virtual bool RevokeUriPermission(const Security::AccessToken::AccessTokenID tokenId) = 0;

    /**
     * @brief Clear user's uri authorization record.
     *
     * @param uri The file uri.
     * @param bundleName bundleName of an application.
     * @return Returns true if the remove is successful, otherwise returns false.
     */
    virtual bool RevokeUriPermissionManually(const Uri &uri, const std::string bundleName) = 0;

    enum UriPermMgrCmd {
        // ipc id for GrantUriPermission
        ON_GRANT_URI_PERMISSION = 0,

        // ipc id for VerifyUriPermission
        ON_VERIFY_URI_PERMISSION,

        // ipc id for RevokeUriPermission
        ON_REVOKE_URI_PERMISSION,

        ON_REVOKE_URI_PERMISSION_MANUALLY,

        ON_GRANT_URI_PERMISSION_FROM_SELF,
    };
};
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_URI_PERMISSION_MANAGER_INTERFACE_H
