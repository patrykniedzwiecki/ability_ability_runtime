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

#include "dataobs_mgr_stub.h"

#include "errors.h"
#include "string_ex.h"

#include "data_ability_observer_proxy.h"
#include "data_ability_observer_stub.h"
#include "dataobs_mgr_errors.h"
#include "ipc_skeleton.h"

namespace OHOS {
namespace AAFwk {
using Uri = OHOS::Uri;

const DataObsManagerStub::RequestFuncType DataObsManagerStub::HANDLES[TRANS_BUTT] = {
    &DataObsManagerStub::RegisterObserverInner,
    &DataObsManagerStub::UnregisterObserverInner,
    &DataObsManagerStub::NotifyChangeInner,
    &DataObsManagerStub::RegisterObserverExtInner,
    &DataObsManagerStub::UnregisterObserverExtInner,
    &DataObsManagerStub::NotifyChangeExtInner
};

DataObsManagerStub::DataObsManagerStub() {}

DataObsManagerStub::~DataObsManagerStub() {}

int DataObsManagerStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    HILOG_INFO("cmd = %{public}d, flags= %{public}d,callingPid:%{public}u", code, option.GetFlags(),
        IPCSkeleton::GetCallingPid());
    std::u16string descriptor = DataObsManagerStub::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        HILOG_ERROR("local descriptor is not equal to remote");
        return ERR_INVALID_STATE;
    }

    if (code < TRANS_HEAD || code >= TRANS_BUTT || HANDLES[code] == nullptr) {
        HILOG_ERROR("not support code:%{public}u, BUTT:%{public}d", code, TRANS_BUTT);
        return -1;
    }
    return (this->*HANDLES[code])(data, reply);
}

int DataObsManagerStub::RegisterObserverInner(MessageParcel &data, MessageParcel &reply)
{
    Uri *uri = data.ReadParcelable<Uri>();
    if (uri == nullptr) {
        HILOG_ERROR("uri is nullptr");
        return ERR_INVALID_VALUE;
    }

    auto observer = iface_cast<IDataAbilityObserver>(data.ReadRemoteObject());
    int32_t result = RegisterObserver(*uri, observer);
    reply.WriteInt32(result);
    delete uri;
    return NO_ERROR;
}

int DataObsManagerStub::UnregisterObserverInner(MessageParcel &data, MessageParcel &reply)
{
    Uri *uri = data.ReadParcelable<Uri>();
    if (uri == nullptr) {
        HILOG_ERROR("uri is nullptr");
        return ERR_INVALID_VALUE;
    }

    auto observer = iface_cast<IDataAbilityObserver>(data.ReadRemoteObject());
    int32_t result = UnregisterObserver(*uri, observer);
    reply.WriteInt32(result);
    delete uri;
    return NO_ERROR;
}

int DataObsManagerStub::NotifyChangeInner(MessageParcel &data, MessageParcel &reply)
{
    Uri *uri = data.ReadParcelable<Uri>();
    if (uri == nullptr) {
        HILOG_ERROR("uri is nullptr");
        return ERR_INVALID_VALUE;
    }

    int32_t result = NotifyChange(*uri);
    reply.WriteInt32(result);
    delete uri;
    return NO_ERROR;
}

int32_t DataObsManagerStub::RegisterObserverExtInner(MessageParcel &data, MessageParcel &reply)
{
    Uri *uri = data.ReadParcelable<Uri>();
    if (uri == nullptr) {
        HILOG_ERROR("uri is nullptr");
        return INVALID_PARAM;
    }

    auto observer = iface_cast<IDataAbilityObserver>(data.ReadRemoteObject());
    bool isDescendants = data.ReadBool();
    reply.WriteInt32(RegisterObserverExt(*uri, observer, isDescendants));
    delete uri;
    return SUCCESS;
}

int32_t DataObsManagerStub::UnregisterObserverExtInner(MessageParcel &data, MessageParcel &reply)
{
    auto observer = iface_cast<IDataAbilityObserver>(data.ReadRemoteObject());
    reply.WriteInt32(UnregisterObserverExt(observer));
    return SUCCESS;
}

int32_t DataObsManagerStub::NotifyChangeExtInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t size = data.ReadInt32();
    std::list<Uri> uris;
    Uri *uri = nullptr;
    for (int32_t i = 0; i < size; ++i) {
        uri = data.ReadParcelable<Uri>();
        if (uri == nullptr) {
            HILOG_ERROR("uri is nullptr");
            return INVALID_PARAM;
        }
        uris.emplace_back(*uri);
    }

    reply.WriteInt32(NotifyChangeExt(uris));
    return SUCCESS;
}
}  // namespace AAFwk
}  // namespace OHOS
