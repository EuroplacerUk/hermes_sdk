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

#ifndef HERMES_CPP_ABI

namespace Hermes
{
    //======================== VerticalService implementation =================================
    inline  VerticalService::VerticalService(IVerticalServiceCallback& callback)
    {
        HermesVerticalServiceCallbacks callbacks{};

        callbacks.m_connectedCallback.m_pData = &callback;
        callbacks.m_connectedCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesVerticalState state,
            const HermesConnectionInfo* pConnectionInfo)
            {
                static_cast<IVerticalServiceCallback*>(pCallback)->OnConnected(sessionId, ToCpp(state),
                    ToCpp(*pConnectionInfo));
            };

        callbacks.m_serviceDescriptionCallback.m_pData = &callback;
        callbacks.m_serviceDescriptionCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesVerticalState state,
            const HermesSupervisoryServiceDescriptionData* pServiceDescriptionData)
            {
                static_cast<IVerticalServiceCallback*>(pCallback)->On(sessionId, ToCpp(state), ToCpp(*pServiceDescriptionData));
            };

        callbacks.m_getConfigurationCallback.m_pData = &callback;
        callbacks.m_getConfigurationCallback.m_pCall = [](void* pCallback, uint32_t sessionId, const HermesGetConfigurationData* pData,
            const HermesConnectionInfo* pInfo)
            {
                static_cast<IVerticalServiceCallback*>(pCallback)->On(sessionId, ToCpp(*pData), ToCpp(*pInfo));
            };

        callbacks.m_setConfigurationCallback.m_pData = &callback;
        callbacks.m_setConfigurationCallback.m_pCall = [](void* pCallback, uint32_t sessionId, const HermesSetConfigurationData* pData,
            const HermesConnectionInfo* pInfo)
            {
                static_cast<IVerticalServiceCallback*>(pCallback)->On(sessionId, ToCpp(*pData), ToCpp(*pInfo));
            };

        callbacks.m_sendWorkOrderInfoCallback.m_pData = &callback;
        callbacks.m_sendWorkOrderInfoCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesSendWorkOrderInfoData* pData)
            {
                static_cast<IVerticalServiceCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_queryHermesCapabilitiesCallback.m_pData = &callback;
        callbacks.m_queryHermesCapabilitiesCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesQueryHermesCapabilitiesData* pData)
            {
                static_cast<IVerticalServiceCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_notificationCallback.m_pData = &callback;
        callbacks.m_notificationCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesNotificationData* pData)
            {
                static_cast<IVerticalServiceCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_checkAliveCallback.m_pData = &callback;
        callbacks.m_checkAliveCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesCheckAliveData* pData)
            {
                static_cast<IVerticalServiceCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_disconnectedCallback.m_pData = &callback;
        callbacks.m_disconnectedCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesVerticalState state,
            const HermesError* pError)
            {
                static_cast<IVerticalServiceCallback*>(pCallback)->OnDisconnected(sessionId, ToCpp(state), ToCpp(*pError));
            };

        callbacks.m_traceCallback.m_pData = &callback;
        callbacks.m_traceCallback.m_pCall = [](void* pCallback, unsigned sessionId, EHermesTraceType type,
            HermesStringView trace)
            {
                static_cast<IVerticalServiceCallback*>(pCallback)->OnTrace(sessionId, ToCpp(type), ToCpp(trace));
            };
        m_pImpl = ::CreateHermesVerticalService(&callbacks);
    }

    inline void VerticalService::Run()
    {
        ::RunHermesVerticalService(m_pImpl);
    }

    void VerticalService::Post(std::function<void()>&& f)
    {
        HermesVoidCallback callback = CppToC(std::move(f));
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
#endif