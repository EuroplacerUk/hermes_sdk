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

#include "Downstream.hpp"

#ifndef HERMES_CPP_ABI

namespace Hermes
{
    //======================== Downstream implementation =================================
    inline Downstream::Downstream(unsigned laneId, IDownstreamCallback& callback)
    {
        HermesDownstreamCallbacks callbacks{};

        callbacks.m_connectedCallback.m_pData = &callback;
        callbacks.m_connectedCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesState state,
            const HermesConnectionInfo* pConnectionInfo)
            {
                static_cast<IDownstreamCallback*>(pCallback)->OnConnected(sessionId, ToCpp(state),
                    ToCpp(*pConnectionInfo));
            };

        callbacks.m_serviceDescriptionCallback.m_pData = &callback;
        callbacks.m_serviceDescriptionCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesState state,
            const HermesServiceDescriptionData* pServiceDescriptionData)
            {
                static_cast<IDownstreamCallback*>(pCallback)->On(sessionId, ToCpp(state), ToCpp(*pServiceDescriptionData));
            };

        callbacks.m_machineReadyCallback.m_pData = &callback;
        callbacks.m_machineReadyCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesState state,
            const HermesMachineReadyData* pData)
            {
                static_cast<IDownstreamCallback*>(pCallback)->On(sessionId, ToCpp(state), ToCpp(*pData));
            };

        callbacks.m_revokeMachineReadyCallback.m_pData = &callback;
        callbacks.m_revokeMachineReadyCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesState state,
            const HermesRevokeMachineReadyData* pData)
            {
                static_cast<IDownstreamCallback*>(pCallback)->On(sessionId, ToCpp(state), ToCpp(*pData));
            };

        callbacks.m_startTransportCallback.m_pData = &callback;
        callbacks.m_startTransportCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesState state,
            const HermesStartTransportData* pData)
            {
                static_cast<IDownstreamCallback*>(pCallback)->On(sessionId, ToCpp(state), ToCpp(*pData));
            };

        callbacks.m_stopTransportCallback.m_pData = &callback;
        callbacks.m_stopTransportCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesState state,
            const HermesStopTransportData* pData)
            {
                static_cast<IDownstreamCallback*>(pCallback)->On(sessionId, ToCpp(state), ToCpp(*pData));
            };

        callbacks.m_queryBoardInfoCallback.m_pData = &callback;
        callbacks.m_queryBoardInfoCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesQueryBoardInfoData* pData)
            {
                static_cast<IDownstreamCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_notificationCallback.m_pData = &callback;
        callbacks.m_notificationCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesNotificationData* pData)
            {
                static_cast<IDownstreamCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_checkAliveCallback.m_pData = &callback;
        callbacks.m_checkAliveCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesCheckAliveData* pData)
            {
                static_cast<IDownstreamCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_commandCallback.m_pData = &callback;
        callbacks.m_commandCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesCommandData* pData)
            {
                static_cast<IDownstreamCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_stateCallback.m_pData = &callback;
        callbacks.m_stateCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesState state)
            {
                static_cast<IDownstreamCallback*>(pCallback)->OnState(sessionId, ToCpp(state));
            };

        callbacks.m_disconnectedCallback.m_pData = &callback;
        callbacks.m_disconnectedCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesState state,
            const HermesError* pError)
            {
                static_cast<IDownstreamCallback*>(pCallback)->OnDisconnected(sessionId, ToCpp(state), ToCpp(*pError));
            };

        callbacks.m_traceCallback.m_pData = &callback;
        callbacks.m_traceCallback.m_pCall = [](void* pCallback, unsigned sessionId, EHermesTraceType type,
            HermesStringView trace)
            {
                static_cast<IDownstreamCallback*>(pCallback)->OnTrace(sessionId, ToCpp(type), ToCpp(trace));
            };

        m_pImpl = ::CreateHermesDownstream(laneId, &callbacks);
    }

    inline void Downstream::Run()
    {
        ::RunHermesDownstream(m_pImpl);
    }

    void Downstream::Post(std::function<void()>&& f)
    {
        HermesVoidCallback callback = CppToC(std::move(f));
        ::PostHermesDownstream(m_pImpl, callback);
    }

    inline void Downstream::Enable(const DownstreamSettings& data)
    {
        const Converter2C<DownstreamSettings> converter(data);
        ::EnableHermesDownstream(m_pImpl, converter.CPointer());
    }

    inline void Downstream::Signal(unsigned sessionId, const ServiceDescriptionData& data)
    {
        const Converter2C<ServiceDescriptionData> converter(data);
        ::SignalHermesDownstreamServiceDescription(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Downstream::Signal(unsigned sessionId, const BoardAvailableData& data)
    {
        const Converter2C<BoardAvailableData> converter(data);
        ::SignalHermesBoardAvailable(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Downstream::Signal(unsigned sessionId, const RevokeBoardAvailableData& data)
    {
        const Converter2C<RevokeBoardAvailableData> converter(data);
        ::SignalHermesRevokeBoardAvailable(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Downstream::Signal(unsigned sessionId, const TransportFinishedData& data)
    {
        const Converter2C<TransportFinishedData> converter(data);
        ::SignalHermesTransportFinished(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Downstream::Signal(unsigned sessionId, const BoardForecastData& data)
    {
        const Converter2C<BoardForecastData> converter(data);
        ::SignalHermesBoardForecast(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Downstream::Signal(unsigned sessionId, const SendBoardInfoData& data)
    {
        const Converter2C<SendBoardInfoData> converter(data);
        ::SignalHermesSendBoardInfo(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Downstream::Signal(unsigned sessionId, const NotificationData& data)
    {
        const Converter2C<NotificationData> converter(data);
        ::SignalHermesDownstreamNotification(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Downstream::Signal(unsigned sessionId, const CheckAliveData& data)
    {
        const Converter2C<CheckAliveData> converter(data);
        ::SignalHermesDownstreamCheckAlive(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Downstream::Signal(unsigned sessionId, const CommandData& data)
    {
        const Converter2C<CommandData> converter(data);
        ::SignalHermesDownstreamCommand(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Downstream::Reset(const NotificationData& data)
    {
        const Converter2C<NotificationData> converter(data);
        ::ResetHermesDownstream(m_pImpl, converter.CPointer());
    }

    inline void Downstream::Signal(unsigned sessionId, StringView rawXml)
    {
        ::SignalHermesDownstreamRawXml(m_pImpl, sessionId, ToC(rawXml));
    }

    inline void Downstream::Reset(StringView rawXml)
    {
        ::ResetHermesDownstreamRawXml(m_pImpl, ToC(rawXml));
    }

    inline void Downstream::Disable(const NotificationData& data)
    {
        const Converter2C<NotificationData> converter(data);
        ::DisableHermesDownstream(m_pImpl, converter.CPointer());
    }

    inline void Downstream::Stop()
    {
        ::StopHermesDownstream(m_pImpl);
    }
}
#endif