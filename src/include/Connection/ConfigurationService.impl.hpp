/***********************************************************************
Copyright 2018 ASM Assembly Systems GmbH & Co. KG
2025 Europlacer Ltd

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
************************************************************************/

// Copyright (c) ASM Assembly Systems GmbH & Co. KG
#pragma once

#include "ConfigurationService.hpp"

namespace Hermes
{
    //======================== ConfigurationService implementation =================================
    inline ConfigurationService::ConfigurationService(IConfigurationServiceCallback& callback) :
        m_callback(callback)
    {
        HermesConfigurationServiceCallbacks callbacks{};
        callbacks.m_connectedCallback.m_pData = this;
        callbacks.m_connectedCallback.m_pCall = [](void* pVoid, uint32_t sessionId, EHermesState, const HermesConnectionInfo* pInfo)
            {
                static_cast<ConfigurationService*>(pVoid)->m_callback.OnConnected(sessionId, ToCpp(*pInfo));
            };

        callbacks.m_getConfigurationCallback.m_pData = this;
        callbacks.m_getConfigurationCallback.m_pCall = [](void* pVoid, uint32_t sessionId,
            const HermesGetConfigurationData*, const HermesConnectionInfo* pConnectionInfo)
            {
                auto pThis = static_cast<ConfigurationService*>(pVoid);
                auto configuration = pThis->m_callback.OnGetConfiguration(sessionId, ToCpp(*pConnectionInfo));
                const Converter2C<CurrentConfigurationData> converter(configuration);
                ::SignalHermesCurrentConfiguration(pThis->m_pImpl, sessionId, converter.CPointer());
            };

        callbacks.m_setConfigurationCallback.m_pData = this;
        callbacks.m_setConfigurationCallback.m_pCall = [](void* pVoid, uint32_t sessionId,
            const HermesSetConfigurationData* pConfiguration, const HermesConnectionInfo* pConnectionInfo)
            {
                auto pThis = static_cast<ConfigurationService*>(pVoid);
                auto error = pThis->m_callback.OnSetConfiguration(sessionId, ToCpp(*pConnectionInfo), ToCpp(*pConfiguration));
                if (!error)
                    return;

                NotificationData notification(ENotificationCode::eCONFIGURATION_ERROR, ESeverity::eERROR, error.m_text);
                const Converter2C<NotificationData> converter(notification);
                ::SignalHermesConfigurationNotification(pThis->m_pImpl, sessionId, converter.CPointer());
            };

        callbacks.m_disconnectedCallback.m_pData = this;
        callbacks.m_disconnectedCallback.m_pCall = [](void* pVoid, uint32_t sessionId, EHermesState, const HermesError* pError)
            {
                static_cast<ConfigurationService*>(pVoid)->m_callback.OnDisconnected(sessionId, ToCpp(*pError));
            };

        callbacks.m_traceCallback.m_pData = this;
        callbacks.m_traceCallback.m_pCall = [](void* pVoid, unsigned sessionId, EHermesTraceType type,
            HermesStringView trace)
            {
                static_cast<ConfigurationService*>(pVoid)->m_callback.OnTrace(sessionId, ToCpp(type), ToCpp(trace));
            };

        m_pImpl = ::CreateHermesConfigurationService(&callbacks);
    }

    inline void ConfigurationService::Run()
    {
        ::RunHermesConfigurationService(m_pImpl);
    }

    template<class F> void ConfigurationService::Post(F&& f)
    {
        HermesVoidCallback callback;
        callback.m_pData = std::make_unique<F>(std::forward<F>(f)).release();
        callback.m_pCall = [](void* pData)
            {
                auto upF = std::unique_ptr<F>(static_cast<F*>(pData));
                (*upF)();
            };
        ::PostHermesConfigurationService(m_pImpl, callback);
    }

    inline void ConfigurationService::Enable(const ConfigurationServiceSettings& data)
    {
        const Converter2C<ConfigurationServiceSettings> converter(data);
        ::EnableHermesConfigurationService(m_pImpl, converter.CPointer());
    }

    inline void ConfigurationService::Disable(const NotificationData& data)
    {
        const Converter2C<NotificationData> converter(data);
        ::DisableHermesConfigurationService(m_pImpl, converter.CPointer());
    }

    inline void ConfigurationService::Stop()
    {
        ::StopHermesConfigurationService(m_pImpl);
    }
}