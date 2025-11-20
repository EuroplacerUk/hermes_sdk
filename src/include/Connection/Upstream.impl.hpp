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

#include "Upstream.hpp"

namespace Hermes
{
    //======================== Upstream implementation =================================
    inline Upstream::Upstream(unsigned laneId, IUpstreamCallback& callback)
    {
        m_pImpl = Hermes::CreateHermesUpstream(laneId, callback);
    }

    inline void Upstream::Run()
    {
        ::RunHermesUpstream(m_pImpl);
    }

    inline void Upstream::Enable(const UpstreamSettings& data)
    {
        const Converter2C<UpstreamSettings> converter(data);
        ::EnableHermesUpstream(m_pImpl, converter.CPointer());
    }

    template<class F> void Upstream::Post(F&& f)
    {
        HermesVoidCallback callback;
        callback.m_pData = std::make_unique<F>(std::forward<F>(f)).release();
        callback.m_pCall = [](void* pData)
            {
                auto upF = std::unique_ptr<F>(static_cast<F*>(pData));
                (*upF)();
            };
        ::PostHermesUpstream(m_pImpl, callback);
    }

    inline void Upstream::Signal(unsigned sessionId, const ServiceDescriptionData& data)
    {
        const Converter2C<ServiceDescriptionData> converter(data);
        ::SignalHermesUpstreamServiceDescription(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Upstream::Signal(unsigned sessionId, const MachineReadyData& data)
    {
        const Converter2C<MachineReadyData> converter(data);
        ::SignalHermesMachineReady(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Upstream::Signal(unsigned sessionId, const RevokeMachineReadyData& data)
    {
        const Converter2C<RevokeMachineReadyData> converter(data);
        ::SignalHermesRevokeMachineReady(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Upstream::Signal(unsigned sessionId, const StartTransportData& data)
    {
        const Converter2C<StartTransportData> converter(data);
        ::SignalHermesStartTransport(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Upstream::Signal(unsigned sessionId, const StopTransportData& data)
    {
        const Converter2C<StopTransportData> converter(data);
        ::SignalHermesStopTransport(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Upstream::Signal(unsigned sessionId, const QueryBoardInfoData& data)
    {
        const Converter2C<QueryBoardInfoData> converter(data);
        ::SignalHermesQueryBoardInfo(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Upstream::Signal(unsigned sessionId, const NotificationData& data)
    {
        const Converter2C<NotificationData> converter(data);
        ::SignalHermesUpstreamNotification(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Upstream::Reset(const NotificationData& data)
    {
        const Converter2C<NotificationData> converter(data);
        ::ResetHermesUpstream(m_pImpl, converter.CPointer());
    }

    inline void Upstream::Signal(unsigned sessionId, const CheckAliveData& data)
    {
        const Converter2C<CheckAliveData> converter(data);
        ::SignalHermesUpstreamCheckAlive(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Upstream::Signal(unsigned sessionId, const CommandData& data)
    {
        const Converter2C<CommandData> converter(data);
        ::SignalHermesUpstreamCommand(m_pImpl, sessionId, converter.CPointer());
    }

    inline void Upstream::Signal(unsigned sessionId, StringView rawXml)
    {
        ::SignalHermesUpstreamRawXml(m_pImpl, sessionId, ToC(rawXml));
    }

    inline void Upstream::Reset(StringView rawXml)
    {
        ::ResetHermesUpstreamRawXml(m_pImpl, ToC(rawXml));
    }

    inline void Upstream::Disable(const NotificationData& data)
    {
        const Converter2C<NotificationData> converter(data);
        ::DisableHermesUpstream(m_pImpl, converter.CPointer());
    }

    inline void Upstream::Stop()
    {
        ::StopHermesUpstream(m_pImpl);
    }
}