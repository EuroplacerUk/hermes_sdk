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

#include "VerticalClient.hpp"

namespace Hermes
{
    //======================== VerticalClient implementation =================================
    inline VerticalClient::VerticalClient(IVerticalClientCallback& callback)
    {
        m_pImpl = Hermes::CreateHermesVerticalClient(callback);
    }

    inline void VerticalClient::Run()
    {
        ::RunHermesVerticalClient(m_pImpl);
    }

    template<class F> void VerticalClient::Post(F&& f)
    {
        HermesVoidCallback callback;
        callback.m_pData = std::make_unique<F>(std::forward<F>(f)).release();
        callback.m_pCall = [](void* pData)
            {
                auto upF = std::unique_ptr<F>(static_cast<F*>(pData));
                (*upF)();
            };
        ::PostHermesVerticalClient(m_pImpl, callback);
    }

    inline void VerticalClient::Enable(const VerticalClientSettings& data)
    {
        const Converter2C<VerticalClientSettings> converter(data);
        ::EnableHermesVerticalClient(m_pImpl, converter.CPointer());
    }

    inline void VerticalClient::Signal(unsigned sessionId, const SupervisoryServiceDescriptionData& data)
    {
        const Converter2C<SupervisoryServiceDescriptionData> converter(data);
        ::SignalHermesVerticalClientDescription(m_pImpl, sessionId, converter.CPointer());
    }

    inline void VerticalClient::Signal(unsigned sessionId, const SendWorkOrderInfoData& data)
    {
        const Converter2C<SendWorkOrderInfoData> converter(data);
        ::SignalHermesSendWorkOrderInfo(m_pImpl, sessionId, converter.CPointer());
    }

    inline void VerticalClient::Signal(unsigned sessionId, const GetConfigurationData& data)
    {
        const Converter2C<GetConfigurationData> converter(data);
        ::SignalHermesVerticalGetConfiguration(m_pImpl, sessionId, converter.CPointer());
    }

    inline void VerticalClient::Signal(unsigned sessionId, const SetConfigurationData& data)
    {
        const Converter2C<SetConfigurationData> converter(data);
        ::SignalHermesVerticalSetConfiguration(m_pImpl, sessionId, converter.CPointer());
    }

    inline void VerticalClient::Signal(unsigned sessionId, const QueryHermesCapabilitiesData& data)
    {
        const Converter2C<QueryHermesCapabilitiesData> converter(data);
        ::SignalHermesVerticalQueryHermesCapabilities(m_pImpl, sessionId, converter.CPointer());
    }

    inline void VerticalClient::Signal(unsigned sessionId, const NotificationData& data)
    {
        const Converter2C<NotificationData> converter(data);
        ::SignalHermesVerticalClientNotification(m_pImpl, sessionId, converter.CPointer());
    }

    inline void VerticalClient::Signal(unsigned sessionId, const CheckAliveData& data)
    {
        const Converter2C<CheckAliveData> converter(data);
        ::SignalHermesVerticalClientCheckAlive(m_pImpl, sessionId, converter.CPointer());
    }

    inline void VerticalClient::Reset(const NotificationData& data)
    {
        const Converter2C<NotificationData> converter(data);
        ::ResetHermesVerticalClient(m_pImpl, converter.CPointer());
    }

    inline void VerticalClient::Signal(unsigned sessionId, StringView rawXml)
    {
        ::SignalHermesVerticalClientRawXml(m_pImpl, sessionId, ToC(rawXml));
    }

    inline void VerticalClient::Reset(StringView rawXml)
    {
        ::ResetHermesVerticalClientRawXml(m_pImpl, ToC(rawXml));
    }

    inline void VerticalClient::Disable(const NotificationData& data)
    {
        const Converter2C<NotificationData> converter(data);
        ::DisableHermesVerticalClient(m_pImpl, converter.CPointer());
    }

    inline void VerticalClient::Stop()
    {
        ::StopHermesVerticalClient(m_pImpl);
    }

    
}