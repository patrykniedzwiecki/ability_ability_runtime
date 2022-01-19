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

#include "configuration.h"

#include <nlohmann/json.hpp>

#include "app_log_wrapper.h"
#include "string_ex.h"
namespace ConfigurationInner {
    const std::string CONNECTION_SYMBOL {"#"};
    const std::string EMPTY_STRING {""};

    /*
     * This must be synchronized with the value in GlobalConfigurationKey
     */
    const std::vector<std::string> SystemConfigurationKeyStore {
        OHOS::AAFwk::GlobalConfigurationKey::SYSTEM_LANGUAGE,
        OHOS::AAFwk::GlobalConfigurationKey::SYSTEM_ORIENTATION,
    };
}

namespace OHOS {
namespace AppExecFwk {
using json = nlohmann::json;

Configuration::Configuration()
{}

Configuration::Configuration(const Configuration &other) : defaultDisplayId_(other.defaultDisplayId_)
{
    configParameter_ = other.configParameter_;
}

Configuration& Configuration::operator= (const Configuration &other)
{
    if (this == &other) {
        return *this;
    }

    defaultDisplayId_ = other.defaultDisplayId_;
    configParameter_.clear();
    configParameter_ = other.configParameter_;
    return *this;
}

Configuration::~Configuration()
{}

bool Configuration::MakeTheKey(std::string &getKey, int id, const std::string &param) const
{
    if (param.empty()) {
        return false;
    }

    if (std::find(ConfigurationInner::SystemConfigurationKeyStore.begin(),
        ConfigurationInner::SystemConfigurationKeyStore.end(), param) ==
        ConfigurationInner::SystemConfigurationKeyStore.end()) {
        return false;
    }

    getKey.clear();
    getKey += std::to_string(id);
    getKey += ConfigurationInner::CONNECTION_SYMBOL;
    getKey += param;
    APP_LOGW(" getKey [%{public}s]", getKey.c_str());

    return true;
}

bool Configuration::AddItem(int displayId, const std::string &key, const std::string &value)
{
    if (key.empty() || value.empty()) {
        return false;
    }

    std::string getKey;
    if (!MakeTheKey(getKey, displayId, key)) {
        return false;
    }

    configParameter_[getKey] = value;
    return true;
}

std::string Configuration::GetItem(int displayId, const std::string &key) const
{
    if (key.empty()) {
        return ConfigurationInner::EMPTY_STRING;
    }

    std::string getKey;
    if (!MakeTheKey(getKey, displayId, key)) {
        return ConfigurationInner::EMPTY_STRING;
    }

    auto iter = configParameter_.find(getKey);
    if (iter != configParameter_.end()) {
        return iter->second;
    }

    return ConfigurationInner::EMPTY_STRING;
}

int Configuration::GetItemSize() const
{
    return configParameter_.size();
}

void Configuration::GetAllKey(std::vector<std::string> &keychain) const
{
    keychain.clear();
    for (const auto &it :configParameter_) {
        keychain.push_back(it.first);
    }
}

std::string Configuration::GetValue(const std::string &key) const
{
    auto iter = configParameter_.find(key);
    if (iter != configParameter_.end()) {
        return iter->second;
    }

    return ConfigurationInner::EMPTY_STRING;
}

void Configuration::CompareDifferent(std::vector<std::string> &diffKeyV, const Configuration &other)
{
    if (other.GetItemSize() == 0) {
        return;
    }

    diffKeyV.clear();
    std::vector<std::string> otherk;
    other.GetAllKey(otherk);
    for (const auto &iter : otherk) {
        APP_LOGW(" iter : [%{public}s] | Val: [%{public}s]", iter.c_str(), other.GetValue(iter).c_str());
        // Insert new content directly
        auto pair = configParameter_.insert(std::make_pair(iter, other.GetValue(iter)));
        if (pair.second) {
            diffKeyV.push_back(iter); // One of the changes this time
            continue;
        }
        // Compare what you already have
        if (!other.GetValue(iter).empty() && other.GetValue(iter) != GetValue(iter)) {
            diffKeyV.push_back(iter);
        }
    }
}

void Configuration::Merge(const std::string &mergeItemKey, const Configuration &other)
{
    auto myItem = GetValue(mergeItemKey);
    auto otherItem = other.GetValue(mergeItemKey);
    // myItem possible empty
    if (!otherItem.empty() && otherItem != myItem) {
        configParameter_[mergeItemKey] = otherItem;
    }
}

int Configuration::RemoveItem(int displayId, const std::string &key)
{
    if (key.empty()) {
        return 0;
    }

    std::string getKey;
    if (!MakeTheKey(getKey, displayId, key)) {
        return 0;
    }

    return configParameter_.erase(getKey);
}

bool Configuration::AddItem(const std::string &key, const std::string &value)
{
    return AddItem(defaultDisplayId_, key, value);
}

std::string Configuration::GetItem(const std::string &key) const
{
    return GetItem(defaultDisplayId_, key);
}

std::string Configuration::GetItemForIdAndKey(const std::string &key) const
{
    if (key.empty()) {
        return ConfigurationInner::EMPTY_STRING;
    }

    APP_LOGI("Configuration::%{public}s called. [%{public}s]", __func__, key.c_str());

    auto iter = configParameter_.find(key);
    if (iter != configParameter_.end()) {
        return iter->second;
    }

    return ConfigurationInner::EMPTY_STRING;
}

int Configuration::RemoveItem(const std::string &key)
{
    return RemoveItem(defaultDisplayId_, key);
}

const std::string& Configuration::GetName() const
{
    json configArray(configParameter_);
    toStrintg_ = configArray.dump();
    return toStrintg_;
}

bool Configuration::ReadFromParcel(Parcel &parcel)
{
    APP_LOGW("ReadFromParcel");
    defaultDisplayId_ = parcel.ReadInt32();
    int32_t configSize = parcel.ReadInt32();
    std::vector<std::string> keys;
    std::vector<std::string> values;
    keys.clear();
    values.clear();
    parcel.ReadStringVector(&keys);
    parcel.ReadStringVector(&values);

    std::string key;
    std::string val;
    for (int32_t i = 0; i < configSize; i++) {
        key = keys.at(i);
        val = values.at(i);
        configParameter_.emplace(key, val);
    }
    return true;
}

Configuration *Configuration::Unmarshalling(Parcel &parcel)
{
    Configuration *Configuration = new (std::nothrow) OHOS::AppExecFwk::Configuration();
    if (Configuration && !Configuration->ReadFromParcel(parcel)) {
        delete Configuration;
        Configuration = nullptr;
    }
    return Configuration;
}

bool Configuration::Marshalling(Parcel &parcel) const
{
    APP_LOGW("Marshalling");
    std::vector<std::string> keys;
    std::vector<std::string> values;
    keys.clear();
    values.clear();
    parcel.WriteInt32(defaultDisplayId_);
    parcel.WriteInt32(configParameter_.size());
    for (const auto &config : configParameter_) {
        keys.emplace_back(config.first);
        values.emplace_back(config.second);
    }

    parcel.WriteStringVector(keys);
    parcel.WriteStringVector(values);
    return true;
}
}  // namespace AppExecFwk
}  // namespace OHOS
