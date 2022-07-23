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

#include "connect_server_manager.h"

#include <dlfcn.h>
#include <unistd.h>

#include "hilog_wrapper.h"

namespace OHOS::AbilityRuntime {
namespace {
std::string GetInstanceMapMessage(const std::string& messageType, int32_t instanceId, const std::string& instanceName)
{
    std::string message;
    message.append("{\"type\":\"");
    message.append(messageType);
    message.append("\",\"instanceId\":");
    message.append(std::to_string(instanceId));
    message.append(",\"name\":\"");
    message.append(instanceName);
    message.append("\"}");
    return message;
}
}

using StartServer = void (*)(const std::string&);
using SendMessage = void (*)(const std::string&);
using StopServer = void (*)(const std::string&);
using StoreMessage = void (*)(int32_t, const std::string&);
using RemoveMessage = void (*)(int32_t);
using WaitForDebugger = bool (*)();

ConnectServerManager::~ConnectServerManager()
{
    StopConnectServer();
}

ConnectServerManager& ConnectServerManager::Get()
{
    static ConnectServerManager connectServerManager;
    return connectServerManager;
}

void ConnectServerManager::StartConnectServer(const std::string& bundleName)
{
    HILOG_INFO("ConnectServerManager::StartConnectServer Start connect server");
    handlerConnectServerSo_ = dlopen("libconnectserver_debugger.z.so", RTLD_LAZY);
    if (handlerConnectServerSo_ == nullptr) {
        HILOG_ERROR("ConnectServerManager::StartConnectServer failed to open register library");
        return;
    }
    auto startServer = reinterpret_cast<StartServer>(dlsym(handlerConnectServerSo_, "StartServer"));
    if (startServer == nullptr) {
        HILOG_ERROR("ConnectServerManager::StartConnectServer failed to find symbol 'StartServer'");
        return;
    }
    bundleName_ = bundleName;
    startServer(bundleName_);
}

void ConnectServerManager::StopConnectServer()
{
    HILOG_INFO("ConnectServerManager::StopConnectServer Stop connect server");
    if (handlerConnectServerSo_ == nullptr) {
        HILOG_ERROR("ConnectServerManager::StopConnectServer handlerConnectServerSo_ is nullptr");
        return;
    }
    auto stopServer = reinterpret_cast<StopServer>(dlsym(handlerConnectServerSo_, "StopServer"));
    if (stopServer != nullptr) {
        stopServer(bundleName_);
    } else {
        HILOG_ERROR("ConnectServerManager::StopConnectServer failed to find symbol 'StopServer'");
    }
    dlclose(handlerConnectServerSo_);
    handlerConnectServerSo_ = nullptr;
}

bool ConnectServerManager::AddInstance(int32_t instanceId, const std::string& instanceName)
{
    HILOG_INFO("ConnectServerManager::AddInstance Add instance to connect server");
    if (handlerConnectServerSo_ == nullptr) {
        HILOG_ERROR("ConnectServerManager::AddInstance handlerConnectServerSo_ is nullptr");
        return false;
    }

    auto waitForDebugger = reinterpret_cast<WaitForDebugger>(dlsym(handlerConnectServerSo_, "WaitForDebugger"));
    if (waitForDebugger == nullptr) {
        HILOG_ERROR("ConnectServerManager::AddInstance failed to find symbol 'WaitForDebugger'");
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto result = instanceMap_.try_emplace(instanceId, instanceName);
        if (!result.second) {
            HILOG_WARN("ConnectServerManager::AddInstance Instance %{public}d already added", instanceId);
            return false;
        }
    }

    // Get the message including information of new instance, which will be send to IDE.
    std::string message = GetInstanceMapMessage("addInstance", instanceId, instanceName);

    if (waitForDebugger()) {
        // if not connected, message will be stored and sent later when "connected" coming.
        auto storeMessage = reinterpret_cast<StoreMessage>(dlsym(handlerConnectServerSo_, "StoreMessage"));
        if (storeMessage == nullptr) {
            HILOG_ERROR("ConnectServerManager::AddInstance failed to find symbol 'StoreMessage'");
            return false;
        }
        storeMessage(instanceId, message);
        return false;
    }

    // WaitForDebugger() means the connection state of the connect server
    auto sendMessage = reinterpret_cast<SendMessage>(dlsym(handlerConnectServerSo_, "SendMessage"));
    if (sendMessage == nullptr) {
        HILOG_ERROR("ConnectServerManager::AddInstance failed to find symbol 'SendMessage'");
        return false;
    }
    // if connected, message will be sent immediately.
    sendMessage(message);
    return true;
}

void ConnectServerManager::RemoveInstance(int32_t instanceId)
{
    HILOG_INFO("ConnectServerManager::RemoveInstance Remove instance to connect server");
    if (handlerConnectServerSo_ == nullptr) {
        HILOG_ERROR("ConnectServerManager::RemoveInstance handlerConnectServerSo_ is nullptr");
        return;
    }

    std::string instanceName;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = instanceMap_.find(instanceId);
        if (it == instanceMap_.end()) {
            HILOG_WARN("ConnectServerManager::RemoveInstance Instance %{public}d is not found", instanceId);
            return;
        }

        instanceName = std::move(it->second);
        instanceMap_.erase(it);
    }

    auto waitForDebugger = reinterpret_cast<WaitForDebugger>(dlsym(handlerConnectServerSo_, "WaitForDebugger"));
    if (waitForDebugger == nullptr) {
        HILOG_ERROR("ConnectServerManager::RemoveInstance failed to find symbol 'WaitForDebugger'");
        return;
    }

    // Get the message including information of deleted instance, which will be send to IDE.
    std::string message = GetInstanceMapMessage("destroyInstance", instanceId, instanceName);

    if (waitForDebugger()) {
        auto removeMessage = reinterpret_cast<RemoveMessage>(dlsym(handlerConnectServerSo_, "RemoveMessage"));
        if (removeMessage == nullptr) {
            HILOG_ERROR("ConnectServerManager::RemoveInstance failed to find symbol 'RemoveMessage'");
            return;
        }
        removeMessage(instanceId);
        return;
    }

    auto sendMessage = reinterpret_cast<SendMessage>(dlsym(handlerConnectServerSo_, "SendMessage"));
    if (sendMessage == nullptr) {
        HILOG_ERROR("ConnectServerManager::RemoveInstance failed to find symbol 'SendMessage'");
        return;
    }
    sendMessage(message);
}
} // namespace OHOS::AbilityRuntime