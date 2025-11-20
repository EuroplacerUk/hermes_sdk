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
    struct IVerticalServiceCallback : ITraceCallback
    {
        IVerticalServiceCallback() = default;

        // callbacks must not be moved or copied!
        IVerticalServiceCallback(const IVerticalServiceCallback&) = delete;
        IVerticalServiceCallback& operator=(const IVerticalServiceCallback&) = delete;

        virtual void OnConnected(unsigned sessionId, EVerticalState, const ConnectionInfo&) = 0;
        virtual void On(unsigned sessionId, EVerticalState, const SupervisoryServiceDescriptionData&) = 0;
        virtual void On(unsigned sessionId, const GetConfigurationData&, const ConnectionInfo&) = 0;
        virtual void On(unsigned sessionId, const SetConfigurationData&, const ConnectionInfo&) = 0;
        virtual void On(unsigned /*sessionId*/, const SendWorkOrderInfoData&) {}; // not necessarily interesting, hence not abstract
        virtual void On(unsigned sessionId, const NotificationData&) = 0;
        virtual void On(unsigned sessionId, const QueryHermesCapabilitiesData&) = 0;
        virtual void On(unsigned /*sessionId*/, const CheckAliveData&) {} // not necessarily interesting, hence not abstract
        virtual void OnDisconnected(unsigned sessionId, EVerticalState, const Error&) = 0;

    protected:
        virtual ~IVerticalServiceCallback() = default;
    };

    //======================= VerticalService interface =====================================  
    class VerticalService
    {
    public:
        explicit VerticalService(IVerticalServiceCallback&);
        VerticalService(const VerticalService&) = delete;
        VerticalService& operator=(const VerticalService&) = delete;
        ~VerticalService() { ::DeleteHermesVerticalService(m_pImpl); }

        void Run();
        template<class F> void Post(F&&);
        void Enable(const VerticalServiceSettings&);

        void Signal(unsigned sessionId, const SupervisoryServiceDescriptionData&);
        void Signal(unsigned sessionId, const BoardArrivedData&); // only to a specific client
        void Signal(const BoardArrivedData&); // to all clients that have specified FeatureBoardTracking
        void Signal(unsigned sessionId, const BoardDepartedData&); // only to a specific client
        void Signal(const BoardDepartedData&); // to all clients that have specified FeatureBoardTracking
        void Signal(unsigned sessionId, const QueryWorkOrderInfoData&);
        void Signal(unsigned sessionId, const ReplyWorkOrderInfoData&);
        void Signal(unsigned sessionId, const SendHermesCapabilitiesData&);
        void Signal(unsigned sessionId, const CurrentConfigurationData&);
        void Signal(unsigned sessionId, const NotificationData&);
        void Signal(unsigned sessionId, const CheckAliveData&);
        void Signal(unsigned sessionId, const CommandData&);
        void ResetSession(unsigned sessionId, const NotificationData&);

        void Disable(const NotificationData&);
        void Stop();

    private:
        HermesVerticalService* m_pImpl = nullptr;
    };

#ifdef HERMES_CPP_ABI
    HERMESPROTOCOL_API HermesVerticalService* CreateHermesVerticalService(IVerticalServiceCallback& callbacks);
#else
    inline static HermesVerticalService* CreateHermesVerticalService(IVerticalServiceCallback& callback)
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

        return ::CreateHermesVerticalService(&callbacks);
    }
#endif
}