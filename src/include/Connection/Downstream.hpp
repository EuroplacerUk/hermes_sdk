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

#include "HermesDataConversion.hpp"

#include <functional>
#include <memory>

namespace Hermes
{

    struct IDownstreamCallback
    {
        IDownstreamCallback() = default;

        // callbacks must not be moved or copied!
        IDownstreamCallback(const IDownstreamCallback&) = delete;
        IDownstreamCallback& operator=(const IDownstreamCallback&) = delete;

        virtual void OnConnected(unsigned sessionId, EState, const ConnectionInfo&) = 0;
        virtual void On(unsigned sessionId, const NotificationData&) = 0;
        virtual void On(unsigned /*sessionId*/, const CheckAliveData&) {} // not necessarily interesting, hence not abstract
        virtual void On(unsigned sessionId, const CommandData&) = 0;
        virtual void OnState(unsigned sessionId, EState) = 0;
        virtual void OnDisconnected(unsigned sessionId, EState, const Error&) = 0;

        virtual void On(unsigned sessionId, EState, const ServiceDescriptionData&) = 0;
        virtual void On(unsigned sessionId, EState, const MachineReadyData&) = 0;
        virtual void On(unsigned sessionId, EState, const RevokeMachineReadyData&) = 0;
        virtual void On(unsigned sessionId, EState, const StartTransportData&) = 0;
        virtual void On(unsigned sessionId, EState, const StopTransportData&) = 0;
        virtual void On(unsigned /*sessionId*/, const QueryBoardInfoData&) {} // not necessarily interesting, hence not abstract

        virtual void OnTrace(unsigned sessionId, ETraceType, StringView trace) = 0;

    protected:
        virtual ~IDownstreamCallback() = default;
    };

    //======================= Downstream interface =====================================
    class Downstream
    {
    public:
        explicit Downstream(unsigned laneId, IDownstreamCallback&);
        Downstream(const Downstream&) = delete;
        Downstream& operator=(const Downstream&) = delete;
        ~Downstream() { ::DeleteHermesDownstream(m_pImpl); }

        void Run();
        template<class F> void Post(F&&);
        void Enable(const DownstreamSettings&);

        void Signal(unsigned sessionId, const ServiceDescriptionData&);
        void Signal(unsigned sessionId, const BoardAvailableData&);
        void Signal(unsigned sessionId, const RevokeBoardAvailableData&);
        void Signal(unsigned sessionId, const TransportFinishedData&);
        void Signal(unsigned sessionId, const BoardForecastData&);
        void Signal(unsigned sessionId, const SendBoardInfoData&);
        void Signal(unsigned sessionId, const NotificationData&);
        void Signal(unsigned sessionId, const CheckAliveData&);
        void Signal(unsigned sessionId, const CommandData&);
        void Reset(const NotificationData&);

        // raw XML for testing
        void Signal(unsigned sessionId, StringView rawXml);
        void Reset(StringView rawXml);

        void Disable(const NotificationData&);
        void Stop();

    private:
        HermesDownstream* m_pImpl = nullptr;
    };

    // Convenience interface for situations in which the same class implements IUpstreamCallback and IDownstreamCallback.
    // Because some function names clash, this adds a level of indirection to different function names, eg.
    // On(unsigned, const ServiceDescriptionData&)
    // becomes:
    // OnUpstream(unsigned, const ServiceDescriptionData&)
    // and
    // OnDownstream(unsigned, const ServiceDescriptionData&)
    //
    // Deriving from UpstreamCallbackHelper and DownstreamCallbackHelper gives unique names for all callback methods.
    // Unambiguous methods, eg. On(unsigned, EState, const BoardAvailableData&) remain unchanged!
    struct DownstreamCallbackHelper : IDownstreamCallback
    {
        virtual void OnDownstreamConnected(unsigned sessionId, EState, const ConnectionInfo&) = 0;
        virtual void OnDownstream(unsigned sessionId, EState, const ServiceDescriptionData&) = 0;
        virtual void OnDownstream(unsigned sessionId, const NotificationData& data) = 0;
        virtual void OnDownstream(unsigned sessionId, const CheckAliveData& data) = 0;
        virtual void OnDownstream(unsigned sessionId, const CommandData& data) = 0;
        virtual void OnDownstreamState(unsigned sessionId, EState) = 0;
        virtual void OnDownstreamDisconnected(unsigned sessionId, EState, const Error&) = 0;
        virtual void OnDownstreamTrace(unsigned sessionId, ETraceType, StringView trace) = 0;

        void OnConnected(unsigned sessionId, EState state, const ConnectionInfo& data) override { OnDownstreamConnected(sessionId, state, data); }
        void On(unsigned sessionId, EState state, const ServiceDescriptionData& data) override { OnDownstream(sessionId, state, data); }
        void On(unsigned sessionId, const NotificationData& data) override { OnDownstream(sessionId, data); }
        void On(unsigned sessionId, const CheckAliveData& data) override { OnDownstream(sessionId, data); }
        void On(unsigned sessionId, const CommandData& data) override { OnDownstream(sessionId, data); }
        void OnState(unsigned sessionId, EState state) override { OnDownstreamState(sessionId, state); }
        void OnDisconnected(unsigned sessionId, EState state, const Error& data) override { OnDownstreamDisconnected(sessionId, state, data); }
        void OnTrace(unsigned sessionId, ETraceType type, StringView data) override { OnDownstreamTrace(sessionId, type, data); }
    };
}