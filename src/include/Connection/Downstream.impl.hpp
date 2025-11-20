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

namespace Hermes
{
    //======================== Downstream implementation =================================
    inline Downstream::Downstream(unsigned laneId, IDownstreamCallback& callback)
    {
        m_pImpl = Hermes::CreateHermesDownstream(laneId, callback);
    }

    inline void Downstream::Run()
    {
        ::RunHermesDownstream(m_pImpl);
    }

    template<class F> void Downstream::Post(F&& f)
    {
        HermesVoidCallback callback;
        callback.m_pData = std::make_unique<F>(std::forward<F>(f)).release();
        callback.m_pCall = [](void* pData)
            {
                auto upF = std::unique_ptr<F>(static_cast<F*>(pData));
                (*upF)();
            };
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