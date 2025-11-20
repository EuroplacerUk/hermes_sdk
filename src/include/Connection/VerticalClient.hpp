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
    struct IVerticalClientCallback : ITraceCallback
    {
        IVerticalClientCallback() = default;

        // callbacks must not be moved or copied!
        IVerticalClientCallback(const IVerticalClientCallback&) = delete;
        IVerticalClientCallback& operator=(const IVerticalClientCallback&) = delete;

        virtual void OnConnected(unsigned sessionId, EVerticalState, const ConnectionInfo&) = 0;
        virtual void On(unsigned sessionId, EVerticalState, const SupervisoryServiceDescriptionData&) = 0;
        virtual void On(unsigned /*sessionId*/, const BoardArrivedData&) {}; // not necessarily interesting, hence not abstract
        virtual void On(unsigned /*sessionId*/, const BoardDepartedData&) {} // not necessarily interesting, hence not abstract
        virtual void On(unsigned sessionId, const QueryWorkOrderInfoData&) = 0;
        virtual void On(unsigned sessionId, const ReplyWorkOrderInfoData&) = 0;
        virtual void On(unsigned /*sessionId*/, const CurrentConfigurationData&) {} // not necessarily interesting, hence not abstract
        virtual void On(unsigned /*sessionId*/, const SendHermesCapabilitiesData&) {} // not necessarily interesting, hence not abstract
        virtual void On(unsigned sessionId, const NotificationData&) = 0;
        virtual void On(unsigned /*sessionId*/, const CheckAliveData&) {} // not necessarily interesting, hence not abstract
        virtual void OnDisconnected(unsigned sessionId, EVerticalState, const Error&) = 0;

    protected:
        virtual ~IVerticalClientCallback() = default;
    };

    //======================= VerticalClient interface =====================================
    class VerticalClient
    {
    public:
        explicit VerticalClient(IVerticalClientCallback&);
        VerticalClient(const VerticalClient&) = delete;
        VerticalClient& operator=(const VerticalClient&) = delete;
        ~VerticalClient() { ::DeleteHermesVerticalClient(m_pImpl); }

        void Run();
        template<class F> void Post(F&&);
        void Enable(const VerticalClientSettings&);

        void Signal(unsigned sessionId, const SupervisoryServiceDescriptionData&);
        void Signal(unsigned sessionId, const GetConfigurationData&);
        void Signal(unsigned sessionId, const SetConfigurationData&);
        void Signal(unsigned sessionId, const QueryHermesCapabilitiesData&);
        void Signal(unsigned sessionId, const SendWorkOrderInfoData&);
        void Signal(unsigned sessionId, const NotificationData&);
        void Signal(unsigned sessionId, const CheckAliveData&);
        void Reset(const NotificationData&);

        // raw XML for testing
        void Signal(unsigned sessionId, StringView rawXml);
        void Reset(StringView rawXml);

        void Disable(const NotificationData&);
        void Stop();

    private:
        HermesVerticalClient* m_pImpl = nullptr;
    };

#ifdef HERMES_CPP_ABI
    HERMESPROTOCOL_API HermesVerticalClient* CreateHermesVerticalClient(IVerticalClientCallback& callback);
#else
    inline static HermesVerticalClient* CreateHermesVerticalClient(IVerticalClientCallback& callback)
    {
        HermesVerticalClientCallbacks callbacks{};

        callbacks.m_connectedCallback.m_pData = &callback;
        callbacks.m_connectedCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesVerticalState state,
            const HermesConnectionInfo* pConnectionInfo)
            {
                static_cast<IVerticalClientCallback*>(pCallback)->OnConnected(sessionId, ToCpp(state),
                    ToCpp(*pConnectionInfo));
            };

        callbacks.m_serviceDescriptionCallback.m_pData = &callback;
        callbacks.m_serviceDescriptionCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesVerticalState state,
            const HermesSupervisoryServiceDescriptionData* pServiceDescriptionData)
            {
                static_cast<IVerticalClientCallback*>(pCallback)->On(sessionId, ToCpp(state), ToCpp(*pServiceDescriptionData));
            };

        callbacks.m_boardArrivedCallback.m_pData = &callback;
        callbacks.m_boardArrivedCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesBoardArrivedData* pData)
            {
                static_cast<IVerticalClientCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_boardDepartedCallback.m_pData = &callback;
        callbacks.m_boardDepartedCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesBoardDepartedData* pData)
            {
                static_cast<IVerticalClientCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_queryWorkOrderInfoCallback.m_pData = &callback;
        callbacks.m_queryWorkOrderInfoCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesQueryWorkOrderInfoData* pData)
            {
                static_cast<IVerticalClientCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_replyWorkOrderInfoCallback.m_pData = &callback;
        callbacks.m_replyWorkOrderInfoCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesReplyWorkOrderInfoData* pData)
            {
                static_cast<IVerticalClientCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_currentConfigurationCallback.m_pData = &callback;
        callbacks.m_currentConfigurationCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesCurrentConfigurationData* pData)
            {
                static_cast<IVerticalClientCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_notificationCallback.m_pData = &callback;
        callbacks.m_notificationCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesNotificationData* pData)
            {
                static_cast<IVerticalClientCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_checkAliveCallback.m_pData = &callback;
        callbacks.m_checkAliveCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesCheckAliveData* pData)
            {
                static_cast<IVerticalClientCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        callbacks.m_disconnectedCallback.m_pData = &callback;
        callbacks.m_disconnectedCallback.m_pCall = [](void* pCallback, uint32_t sessionId, EHermesVerticalState state,
            const HermesError* pError)
            {
                static_cast<IVerticalClientCallback*>(pCallback)->OnDisconnected(sessionId, ToCpp(state), ToCpp(*pError));
            };

        callbacks.m_traceCallback.m_pData = &callback;
        callbacks.m_traceCallback.m_pCall = [](void* pCallback, unsigned sessionId, EHermesTraceType type,
            HermesStringView trace)
            {
                static_cast<IVerticalClientCallback*>(pCallback)->OnTrace(sessionId, ToCpp(type), ToCpp(trace));
            };

        callbacks.m_sendHermesCapabilitiesCallback.m_pData = &callback;
        callbacks.m_sendHermesCapabilitiesCallback.m_pCall = [](void* pCallback, uint32_t sessionId,
            const HermesSendHermesCapabilitiesData* pData)
            {
                static_cast<IVerticalClientCallback*>(pCallback)->On(sessionId, ToCpp(*pData));
            };

        return ::CreateHermesVerticalClient(&callbacks);
    }
#endif
    
}