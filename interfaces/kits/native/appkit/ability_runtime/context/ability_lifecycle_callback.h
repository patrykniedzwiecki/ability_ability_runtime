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

#ifndef OHOS_ABILITY_RUNTIME_CONTEXT_ABILITY_LIFECYCLE_CALLBACK_H
#define OHOS_ABILITY_RUNTIME_CONTEXT_ABILITY_LIFECYCLE_CALLBACK_H

#include <map>
#include <memory>

class NativeEngine;
class NativeValue;
class NativeReference;
struct NativeCallbackInfo;

namespace OHOS {
namespace AbilityRuntime {
class AbilityLifecycleCallback {
public:
    virtual ~AbilityLifecycleCallback() {}
    /**
     * Called back when the ability is started for initialization.
     *
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     * @param ability: Indicates the ability to register for listening.
     * @StageModelOnly
     */
    virtual void OnAbilityCreate(const std::shared_ptr<NativeReference> &ability) = 0;

    /**
     * Called back when the window stage is created.
     *
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     * @param ability: Indicates the ability to register for listening.
     * @param windowStage: window stage to create.
     * @StageModelOnly
     */
    virtual void OnWindowStageCreate(const std::shared_ptr<NativeReference> &ability,
        const std::shared_ptr<NativeReference> &windowStage) = 0;

    /**
     * Called back when the window stage is created.
     *
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     * @param ability: Indicates the ability to register for listening.
     * @param windowStage: window stage to destroy.
     * @StageModelOnly
     */
    virtual void OnWindowStageDestroy(const std::shared_ptr<NativeReference> &ability,
        const std::shared_ptr<NativeReference> &windowStage) = 0;

    /**
     * Called back when the window stage is actived.
     *
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     * @param ability: Indicates the ability to register for listening.
     * @StageModelOnly
     */
    virtual void OnWindowStageActive(const std::shared_ptr<NativeReference> &ability,
        const std::shared_ptr<NativeReference> &windowStage) = 0;

    /**
     * Called back when the window stage is inactived.
     *
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     * @param ability: Indicates the ability to register for listening.
     * @StageModelOnly
     */
    virtual void OnWindowStageInactive(const std::shared_ptr<NativeReference> &ability,
        const std::shared_ptr<NativeReference> &windowStage) = 0;

    /**
     * Called back when the ability window stage is created.
     *
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     * @param ability: Indicates the ability to register for listening.
     * @StageModelOnly
     */
    virtual void OnAbilityDestroy(const std::shared_ptr<NativeReference> &ability) = 0;

    /**
     * Called back when the ability window stage is created.
     *
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     * @param ability: Indicates the ability to register for listening.
     * @StageModelOnly
     */
    virtual void OnAbilityForeground(const std::shared_ptr<NativeReference> &ability) = 0;

    /**
     * Called back when the ability window stage is created.
     *
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     * @param ability: Indicates the ability to register for listening.
     * @StageModelOnly
     */
    virtual void OnAbilityBackground(const std::shared_ptr<NativeReference> &ability) = 0;

    /**
     * Called back when the ability window stage is created.
     *
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     * @param ability: Indicates the ability to register for listening.
     * @StageModelOnly
     */
    virtual void OnAbilityContinue(const std::shared_ptr<NativeReference> &ability) = 0;
};

class JsAbilityLifecycleCallback : public AbilityLifecycleCallback,
                                   public std::enable_shared_from_this<JsAbilityLifecycleCallback> {
public:
    explicit JsAbilityLifecycleCallback(NativeEngine* engine);
    void OnAbilityCreate(const std::shared_ptr<NativeReference> &ability) override;
    void OnWindowStageCreate(const std::shared_ptr<NativeReference> &ability,
        const std::shared_ptr<NativeReference> &windowStage) override;
    void OnWindowStageDestroy(const std::shared_ptr<NativeReference> &ability,
        const std::shared_ptr<NativeReference> &windowStage) override;
    void OnWindowStageActive(const std::shared_ptr<NativeReference> &ability,
        const std::shared_ptr<NativeReference> &windowStage) override;
    void OnWindowStageInactive(const std::shared_ptr<NativeReference> &ability,
        const std::shared_ptr<NativeReference> &windowStage) override;
    void OnAbilityDestroy(const std::shared_ptr<NativeReference> &ability) override;
    void OnAbilityForeground(const std::shared_ptr<NativeReference> &ability) override;
    void OnAbilityBackground(const std::shared_ptr<NativeReference> &ability) override;
    void OnAbilityContinue(const std::shared_ptr<NativeReference> &ability) override;
    int32_t Register(NativeValue *jsCallback);
    bool UnRegister(int32_t callbackId);
    bool IsEmpty();
    static int32_t serialNumber_;

private:
    NativeEngine* engine_;
    std::shared_ptr<NativeReference> jsCallback_;
    std::map<int32_t, std::shared_ptr<NativeReference>> callbacks_;
    void CallJsMethod(const std::string &methodName, const std::shared_ptr<NativeReference> &ability);
    void CallJsMethodInner(const std::string &methodName, const std::shared_ptr<NativeReference> &ability);
    void CallWindowStageJsMethod(const std::string &methodName, const std::shared_ptr<NativeReference> &ability,
        const std::shared_ptr<NativeReference> &windowStage);
    void CallWindowStageJsMethodInner(const std::string &methodName, const std::shared_ptr<NativeReference> &ability,
        const std::shared_ptr<NativeReference> &windowStage);
};
}  // namespace AbilityRuntime
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_CONTEXT_ABILITY_LIFECYCLE_CALLBACK_H
