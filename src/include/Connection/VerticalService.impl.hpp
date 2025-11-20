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

#include "VerticalService.hpp"

namespace Hermes
{
    //======================== VerticalService implementation =================================
    inline  VerticalService::VerticalService(IVerticalServiceCallback& callback)
    {
        m_pImpl = Hermes::CreateHermesVerticalService(callback);
    }

    inline void VerticalService::Run()
    {
        ::RunHermesVerticalService(m_pImpl);
    }

    template<class F> void VerticalService::Post(F&& f)
    {
        HermesVoidCallback callback;
        callback.m_pData = std::make_unique<F>(std::forward<F>(f)).release();
        callback.m_pCall = [](void* pData)
            {
                auto upF = std::unique_ptr<F>(static_cast<F*>(pData));
                (*upF)();
            };
        ::PostHermesVerticalService(m_pImpl, callback);
    }

    inline void VerticalService::Enable(const VerticalServiceSettings& data)
    {
        const Converter2C<VerticalServiceSettings> converter(data);
        ::EnableHermesVerticalService(m_pImpl, converter.CPointer());
    }

    inline void VerticalService::Signal(unsigned sessionId, const SupervisoryServiceDescriptionData& data)
    {
        const Converter2C<SupervisoryServiceDescriptionData> converter(data);
        ::SignalHermesVerticalServiceDescription(m_pImpl, sessionId, converter.CPointer());
    }

    inline void VerticalService::Signal(unsigned sessionId, const BoardArrivedData& data)
    {
        const Converter2C<BoardArrivedData> converter(data);
        ::SignalHermesBoardArrived(m_pImpl, sessionId, converter.CPointer());
    }

    inline void VerticalService::Signal(const BoardArrivedData& data)
    {
        const Converter2C<BoardArrivedData> converter(data);
        ::SignalHermesBoardArrived(m_pImpl, 0U, converter.CPointer());
    }

    inline void VerticalService::Signal(unsigned sessionId, const BoardDepartedData& data)
    {
        const Converter2C<BoardDepartedData> converter(data);
        ::SignalHermesBoardDeparted(m_pImpl, sessionId, converter.CPointer());
    }

    inline void VerticalService::Signal(const BoardDepartedData& data)
    {
        const Converter2C<BoardDepartedData> converter(data);
        ::SignalHermesBoardDeparted(m_pImpl, 0U, converter.CPointer());
    }

    inline void VerticalService::Signal(unsigned sessionId, const QueryWorkOrderInfoData& data)
    {
        const Converter2C<QueryWorkOrderInfoData> converter(data);
        ::SignalHermesQueryWorkOrderInfo(m_pImpl, sessionId, converter.CPointer());
    }

    inline void VerticalService::Signal(unsigned sessionId, const ReplyWorkOrderInfoData& data)
    {
        const Converter2C<ReplyWorkOrderInfoData> converter(data);
        ::SignalHermesReplyWorkOrderInfo(m_pImpl, sessionId, converter.CPointer());
    }

    inline void VerticalService::Signal(unsigned sessionId, const SendHermesCapabilitiesData& data)
    {
        const Converter2C<SendHermesCapabilitiesData> converter(data);
        ::SignalHermesSendHermesCapabilities(m_pImpl, sessionId, converter.CPointer());
    }

    inline void VerticalService::Signal(unsigned sessionId, const CurrentConfigurationData& data)
    {
        const Converter2C<CurrentConfigurationData> converter(data);
        ::SignalHermesVerticalCurrentConfiguration(m_pImpl, sessionId, converter.CPointer());
    }


    inline void VerticalService::Signal(unsigned sessionId, const NotificationData& data)
    {
        const Converter2C<NotificationData> converter(data);
        ::SignalHermesVerticalServiceNotification(m_pImpl, sessionId, converter.CPointer());
    }

    inline void VerticalService::Signal(unsigned sessionId, const CheckAliveData& data)
    {
        const Converter2C<CheckAliveData> converter(data);
        ::SignalHermesVerticalServiceCheckAlive(m_pImpl, sessionId, converter.CPointer());
    }

    inline void VerticalService::ResetSession(unsigned sessionId, const NotificationData& data)
    {
        const Converter2C<NotificationData> converter(data);
        ::ResetHermesVerticalServiceSession(m_pImpl, sessionId, converter.CPointer());
    }


    inline void VerticalService::Disable(const NotificationData& data)
    {
        const Converter2C<NotificationData> converter(data);
        ::DisableHermesVerticalService(m_pImpl, converter.CPointer());
    }

    inline void VerticalService::Stop()
    {
        ::StopHermesVerticalService(m_pImpl);
    }

}