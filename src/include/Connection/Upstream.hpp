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
    struct IUpstreamCallback : ITraceCallback
    {
        IUpstreamCallback() = default;

        // callbacks must not be moved or copied!
        IUpstreamCallback(const IUpstreamCallback&) = delete;
        IUpstreamCallback& operator=(const IUpstreamCallback&) = delete;

        virtual void OnConnected(unsigned sessionId, EState, const ConnectionInfo&) = 0;
        virtual void On(unsigned sessionId, const NotificationData&) = 0;
        virtual void On(unsigned /*sessionId*/, const CheckAliveData&) {} // not necessarily interesting, hence not abstract
        virtual void On(unsigned sessionId, const CommandData&) = 0;
        virtual void OnState(unsigned sessionId, EState) = 0;
        virtual void OnDisconnected(unsigned sessionId, EState, const Error&) = 0;

        virtual void On(unsigned sessionId, EState, const ServiceDescriptionData&) = 0;
        virtual void On(unsigned sessionId, EState, const BoardAvailableData&) = 0;
        virtual void On(unsigned sessionId, EState, const RevokeBoardAvailableData&) = 0;
        virtual void On(unsigned sessionId, EState, const TransportFinishedData&) = 0;
        virtual void On(unsigned /*sessionId*/, EState, const BoardForecastData&) {} // not necessarily interesting, hence not abstract
        virtual void On(unsigned /*sessionId*/, const SendBoardInfoData&) {} // not necessarily interesting, hence not abstract

    protected:
        virtual ~IUpstreamCallback() = default;
    };

    //======================= Upstream interface =====================================
    class Upstream
    {
    public:
        explicit Upstream(unsigned laneId, IUpstreamCallback&);
        Upstream(const Upstream&) = delete;
        Upstream& operator=(const Upstream&) = delete;
        ~Upstream() { ::DeleteHermesUpstream(m_pImpl); }

        void Run();
        template<class F> void Post(F&&);
        void Enable(const UpstreamSettings&);

        void Signal(unsigned sessionId, const ServiceDescriptionData&);
        void Signal(unsigned sessionId, const MachineReadyData&);
        void Signal(unsigned sessionId, const RevokeMachineReadyData&);
        void Signal(unsigned sessionId, const StartTransportData&);
        void Signal(unsigned sessionId, const StopTransportData&);
        void Signal(unsigned sessionId, const QueryBoardInfoData&);
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
        HermesUpstream* m_pImpl = nullptr;
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
    struct UpstreamCallbackHelper : IUpstreamCallback
    {
        virtual void OnUpstreamConnected(unsigned sessionId, EState, const ConnectionInfo&) = 0;
        virtual void OnUpstream(unsigned sessionId, EState, const ServiceDescriptionData&) = 0;
        virtual void OnUpstream(unsigned sessionId, const NotificationData& data) = 0;
        virtual void OnUpstream(unsigned sessionId, const CheckAliveData& data) = 0;
        virtual void OnUpstream(unsigned sessionId, const CommandData& data) = 0;
        virtual void OnUpstreamState(unsigned sessionId, EState) = 0;
        virtual void OnUpstreamDisconnected(unsigned sessionId, EState, const Error&) = 0;
        virtual void OnUpstreamTrace(unsigned sessionId, ETraceType, StringView trace) = 0;

        void OnConnected(unsigned sessionId, EState state, const ConnectionInfo& data) override { OnUpstreamConnected(sessionId, state, data); }
        void On(unsigned sessionId, EState state, const ServiceDescriptionData& data) override { OnUpstream(sessionId, state, data); }
        void On(unsigned sessionId, const NotificationData& data) override { OnUpstream(sessionId, data); }
        void On(unsigned sessionId, const CheckAliveData& data) override { OnUpstream(sessionId, data); }
        void On(unsigned sessionId, const CommandData& data) override { OnUpstream(sessionId, data); }
        void OnState(unsigned sessionId, EState state) override { OnUpstreamState(sessionId, state); }
        void OnDisconnected(unsigned sessionId, EState state, const Error& data) override { OnUpstreamDisconnected(sessionId, state, data); }
        void OnTrace(unsigned sessionId, ETraceType type, StringView data) override { OnUpstreamTrace(sessionId, type, data); }
    };

#ifdef HERMES_CPP_ABI
    HERMESPROTOCOL_API HermesUpstream* CreateHermesUpstream(uint32_t laneId, IUpstreamCallback& callback);
#else
    inline static HermesUpstream* CreateHermesUpstream(uint32_t laneId, IUpstreamCallback& callback)
    {
        HermesUpstreamCallbacks callbacks{};

        callbacks.m_connectedCallback.m_pData = &callback;
        callbacks.m_connectedCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesState state,
            const HermesConnectionInfo* pConnectionInfo)
            {
                static_cast<IUpstreamCallback*>(pCallback)->OnConnected(sessionId, ToCpp(state),
                    ToCpp(*pConnectionInfo));
            };

        callbacks.m_serviceDescriptionCallback.m_pData = &callback;
        callbacks.m_serviceDescriptionCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesState state,
            const HermesServiceDescriptionData* pServiceDescriptionData)
            {
                static_cast<IUpstreamCallback*>(pCallback)->On(sessionId, ToCpp(state), ToCpp(*pServiceDescriptionData));
            };

        callbacks.m_boardAvailableCallback.m_pData = &callback;
        callbacks.m_boardAvailableCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesState state,
            const HermesBoardAvailableData* pData)
            {
                static_cast<IUpstreamCallback*>(pCallback)->On(sessionId, ToCpp(state), ToCpp(*pData));
            };

        callbacks.m_revokeBoardAvailableCallback.m_pData = &callback;
        callbacks.m_revokeBoardAvailableCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesState state,
            const HermesRevokeBoardAvailableData* pData)
            {
                static_cast<IUpstreamCallback*>(pCallback)->On(sessionId, ToCpp(state), ToCpp(*pData));
            };

        callbacks.m_transportFinishedCallback.m_pData = &callback;
        callbacks.m_transportFinishedCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesState state,
            const HermesTransportFinishedData* pData)
            {
                static_cast<IUpstreamCallback*>(pCallback)->On(sessionId, ToCpp(state), ToCpp(*pData));
            };

        callbacks.m_boardForecastCallback.m_pData = &callback;
        callbacks.m_boardForecastCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesState state,
            const HermesBoardForecastData* pData)
            {
                static_cast<IUpstreamCallback*>(pCallback)->On(sessionId, ToCpp(state), ToCpp(*pData));
            };

        callbacks.m_sendBoardInfoCallback.m_pData = &callback;
        callbacks.m_sendBoardInfoCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesSendBoardInfoData* pData)
            {
                static_cast<IUpstreamCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_notificationCallback.m_pData = &callback;
        callbacks.m_notificationCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesNotificationData* pData)
            {
                static_cast<IUpstreamCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_checkAliveCallback.m_pData = &callback;
        callbacks.m_checkAliveCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesCheckAliveData* pData)
            {
                static_cast<IUpstreamCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_commandCallback.m_pData = &callback;
        callbacks.m_commandCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesCommandData* pData)
            {
                static_cast<IUpstreamCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_stateCallback.m_pData = &callback;
        callbacks.m_stateCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesState state)
            {
                static_cast<IUpstreamCallback*>(pCallback)->OnState(sessionId, ToCpp(state));
            };

        callbacks.m_disconnectedCallback.m_pData = &callback;
        callbacks.m_disconnectedCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesState state,
            const HermesError* pError)
            {
                static_cast<IUpstreamCallback*>(pCallback)->OnDisconnected(sessionId, ToCpp(state), ToCpp(*pError));
            };

        callbacks.m_traceCallback.m_pData = &callback;
        callbacks.m_traceCallback.m_pCall = [](void* pCallback, unsigned sessionId, EHermesTraceType type,
            HermesStringView trace)
            {
                static_cast<IUpstreamCallback*>(pCallback)->OnTrace(sessionId, ToCpp(type), ToCpp(trace));
            };

        return ::CreateHermesUpstream(laneId, &callbacks);
    }
#endif


}