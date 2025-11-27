/***********************************************************************
Copyright 2018 ASM Assembly Systems GmbH & Co. KG

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
#include "stdafx.h"
#include <HermesData.hpp>

#include "ApiCallback.h"
#include "VerticalServiceSession.h"
#include "Network.h"
#include "Service.h"

#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <Connection/VerticalService.hpp>

using namespace Hermes;
using namespace Hermes::Implementation::VerticalService;

struct VerticalServiceCallbackAdapter : IVerticalServiceCallback
{
    VerticalServiceCallbackAdapter(const HermesVerticalServiceCallbacks& callbacks) :
        m_traceCallback(callbacks.m_traceCallback),
        m_connectedCallback(callbacks.m_connectedCallback),
        m_serviceDescriptionCallback(callbacks.m_serviceDescriptionCallback),
        m_sendWorkOrderInfoCallback(callbacks.m_sendWorkOrderInfoCallback),
        m_notificationCallback(callbacks.m_notificationCallback),
        m_checkAliveCallback(callbacks.m_checkAliveCallback),
        m_setConfigurationCallback(callbacks.m_setConfigurationCallback),
        m_getConfigurationCallback(callbacks.m_getConfigurationCallback),
        m_disconnectedCallback(callbacks.m_disconnectedCallback),
        m_queryHermesCapabilitiesCallback(callbacks.m_queryHermesCapabilitiesCallback)
    {

    }

    void OnConnected(unsigned sessionId, EVerticalState state, const ConnectionInfo& in_data) override {
        const Converter2C<ConnectionInfo> converter(in_data);
        m_connectedCallback(sessionId, ToC(state), converter.CPointer());
    };
    void On(unsigned sessionId, EVerticalState state, const SupervisoryServiceDescriptionData& in_data) override
    {
        const Converter2C<SupervisoryServiceDescriptionData> converter(in_data);
        m_serviceDescriptionCallback(sessionId, ToC(state), converter.CPointer());
    }
    void On(unsigned sessionId, const GetConfigurationData& in_data, const ConnectionInfo& connection) override
    {
        const Converter2C<GetConfigurationData> configurationConverter(in_data);
        const Converter2C<ConnectionInfo> connectionConverter(connection);
        m_getConfigurationCallback(sessionId, configurationConverter.CPointer(), connectionConverter.CPointer());
    }
    void On(unsigned sessionId, const SetConfigurationData& in_data, const ConnectionInfo& connection)override
    {
        const Converter2C<SetConfigurationData> configurationConverter(in_data);
        const Converter2C<ConnectionInfo> connectionConverter(connection);
        m_setConfigurationCallback(sessionId, configurationConverter.CPointer(), connectionConverter.CPointer());
    }
    void On(unsigned sessionId, const SendWorkOrderInfoData& in_data) override
    {
        const Converter2C<SendWorkOrderInfoData> converter(in_data);
        m_sendWorkOrderInfoCallback(sessionId, converter.CPointer());
    }
    void On(unsigned sessionId, const QueryHermesCapabilitiesData& in_data) override
    {
        const Converter2C<QueryHermesCapabilitiesData> converter(in_data);
        m_queryHermesCapabilitiesCallback(sessionId, converter.CPointer());
    }

    void OnDisconnected(unsigned sessionId, EVerticalState state, const Error& in_data) override
    {
        const Converter2C<Error> converter(in_data);
        m_disconnectedCallback(sessionId, ToC(state), converter.CPointer());
    }

    void On(unsigned sessionId, const NotificationData& in_data) override
    {
        const Converter2C<NotificationData> converter(in_data);
        m_notificationCallback(sessionId, converter.CPointer());
    }
    void On(unsigned sessionId, const CheckAliveData& in_data) override
    {
        const Converter2C<CheckAliveData> converter(in_data);
        m_checkAliveCallback(sessionId, converter.CPointer());
    }
    void OnTrace(unsigned sessionId, ETraceType traceType, StringView trace) override
    {
        m_traceCallback(sessionId, ToC(traceType), ToC(trace));
    }

private:
    ApiCallback<HermesTraceCallback> m_traceCallback;
    ApiCallback<HermesVerticalConnectedCallback> m_connectedCallback;
    ApiCallback<HermesSupervisoryServiceDescriptionCallback> m_serviceDescriptionCallback;
    ApiCallback<HermesSendWorkOrderInfoCallback> m_sendWorkOrderInfoCallback;
    ApiCallback<HermesNotificationCallback> m_notificationCallback;
    ApiCallback<HermesCheckAliveCallback> m_checkAliveCallback;
    ApiCallback<HermesSetConfigurationCallback> m_setConfigurationCallback;
    ApiCallback<HermesGetConfigurationCallback> m_getConfigurationCallback;
    ApiCallback<HermesVerticalDisconnectedCallback> m_disconnectedCallback;
    ApiCallback<HermesQueryHermesCapabilitiesCallback> m_queryHermesCapabilitiesCallback;
};

struct VerticalServiceCallbackHolder : CallbackHolder<IVerticalServiceCallback, VerticalServiceCallbackAdapter, HermesVerticalServiceCallbacks>
{
    VerticalServiceCallbackHolder(const HermesVerticalServiceCallbacks& callbacks) : CallbackHolder(callbacks)
    {
    }

    VerticalServiceCallbackHolder(IVerticalServiceCallback& callbacks) : CallbackHolder(callbacks)
    {
    }
};

struct HermesVerticalService : IVerticalService, IAcceptorCallback, ISessionCallback
{

    HermesVerticalService(VerticalServiceCallbackHolder&& callbacks) :
        m_service(*callbacks),
        m_callbacks{callbacks}
    {
        m_service.Inform(0U, "Created");
    }

    virtual ~HermesVerticalService()
    {
        m_service.Inform(0U, "Deleted");
    }

    void Run() override
    {
        m_service.Log(0U, "RunHermesVerticalService");

        m_service.Run();
    }

    void Post(std::function<void()>&& fn) override
    {
        m_service.Log(0U, "PostHermesVerticalService");

        m_service.Post(std::move(fn));
    }
    void Enable(const VerticalServiceSettings& in_data) override {
        m_service.Log(0U, "EnableHermesVerticalService");

        m_service.Post([this, settings = in_data]()
        {
            this->DoEnable(settings);
        });
    };

    void Signal(unsigned sessionId, const SupervisoryServiceDescriptionData& in_data) override { DoSignal("SignalHermesVerticalServiceDescription", sessionId, in_data); }
    void Signal(unsigned sessionId, const BoardArrivedData& in_data) override { DoSignal("SignalHermesBoardArrived", sessionId, in_data); } // only to a specific client
    void Signal(const BoardArrivedData& in_data) override { DoSignalTracking("SignalHermesBoardArrived", in_data); } // to all clients that have specified FeatureBoardTracking
    void Signal(unsigned sessionId, const BoardDepartedData& in_data) override { DoSignal("SignalHermesBoardDeparted", sessionId, in_data); } // only to a specific client
    void Signal(const BoardDepartedData& in_data) override { DoSignalTracking("SignalHermesBoardDeparted", in_data); } // to all clients that have specified FeatureBoardTracking
    void Signal(unsigned sessionId, const QueryWorkOrderInfoData& in_data) override { DoSignal("SignalHermesQueryWorkOrderInfo", sessionId, in_data); }
    void Signal(unsigned sessionId, const ReplyWorkOrderInfoData& in_data) override { DoSignal("SignalHermesSendWorkOrderInfo", sessionId, in_data); }
    void Signal(unsigned sessionId, const SendHermesCapabilitiesData& in_data) override { DoSignal("SignalSendHermesCapabilitiesData", sessionId, in_data); }
    void Signal(unsigned sessionId, const CurrentConfigurationData& in_data) override { DoSignal("SignalVerticalCurrentConfiguration", sessionId, in_data); }
    void Signal(unsigned sessionId, const NotificationData& in_data) override { DoSignal("SignalHermesVerticalServiceNotification", sessionId, in_data); }
    void Signal(unsigned sessionId, const CheckAliveData& in_data) override { DoSignal("SignalHermesVerticalCheckAlive", sessionId, in_data); }
    void ResetSession(unsigned sessionId, const NotificationData& in_data) override
    {
        m_service.Log(sessionId, "ResetHermesVerticalServiceSession");
        m_service.Post([this, sessionId, data = in_data]()
        {
            this->RemoveSession_(sessionId, data);
        });
    }

    void Disable(const NotificationData& in_data) override
    {
        m_service.Log(0U, "DisableHermesVerticalService");

        m_service.Post([this, data = in_data]()
        {
            this->DoDisable(data);
        });
    }
    void Stop() override
    {
        m_service.Log(0U, "StopHermesVerticalService");

        m_service.Post([this]()
        {
            this->DoStop();
        });
    }

    void Delete() override {
        m_service.Log(0U, "DeleteHermesVerticalService");

        m_service.Stop();
        delete this;
    };

private:
    Service m_service;
    VerticalServiceSettings m_settings;

    // we only hold on to the accepting session
    std::unique_ptr<IAcceptor> m_upAcceptor{ CreateAcceptor(m_service, *this) };
    std::map<unsigned, Session> m_sessionMap;

    VerticalServiceCallbackHolder m_callbacks;

    bool m_enabled{ false };

    //================= forwarding calls =========================
    void DoEnable(const VerticalServiceSettings& settings)
    {
        m_service.Log(0U, "Enable(", settings, "); m_enabled=", m_enabled, ", m_settings=", m_settings);

        if (m_enabled && m_settings == settings)
            return;

        RemoveSessions_(NotificationData(ENotificationCode::eCONNECTION_RESET_BECAUSE_OF_CHANGED_CONFIGURATION,
            ESeverity::eINFO, "ConfigurationChanged"));

        m_enabled = true;
        m_settings = settings;

        NetworkConfiguration networkConfiguration;
        networkConfiguration.m_port = settings.m_port ? settings.m_port : cCONFIG_PORT;
        networkConfiguration.m_retryDelayInSeconds = settings.m_reconnectWaitTimeInSeconds;
        networkConfiguration.m_checkAlivePeriodInSeconds = settings.m_checkAlivePeriodInSeconds;

        m_upAcceptor->StartListening(networkConfiguration);
    }

    void DoDisable(const NotificationData& data)
    {
        m_service.Log(0U, "Disable(", data, "); m_enabled=", m_enabled);

        if (!m_enabled)
            return;

        m_enabled = false;
        m_upAcceptor->StopListening();
        RemoveSessions_(data);
    }

    void DoStop()
    {
        m_service.Log(0U, "Stop(), sessionCount=", m_sessionMap.size());

        const NotificationData notificationData(ENotificationCode::eMACHINE_SHUTDOWN, ESeverity::eINFO, "Vertical service stopped by application");
        RemoveSessions_(notificationData);
        m_service.Stop();
    }

    void RemoveSessions_(const NotificationData& data)
    {
        auto sessionMap = std::move(m_sessionMap);
        m_sessionMap.clear();

        const Error error{};
        for (auto& entry : sessionMap)
        {
            entry.second.Signal(data);
            entry.second.Disconnect();
            m_callbacks->OnDisconnected(entry.second.Id(), EVerticalState::eDISCONNECTED, error);
        }
    }

    void RemoveSession_(unsigned sessionId, const NotificationData& data)
    {
        auto itFound = m_sessionMap.find(sessionId);
        if (itFound == m_sessionMap.end())
            return;

        auto session = std::move(itFound->second);
        m_sessionMap.erase(itFound);

        session.Signal(data);
        session.Disconnect();
        const Error error{};
        m_callbacks->OnDisconnected(session.Id(), EVerticalState::eDISCONNECTED, error);
    }

    Session* Session_(unsigned id)
    {
        auto itFound = m_sessionMap.find(id);
        if (itFound == m_sessionMap.end())
        {
            m_service.Warn(id, "Session ID no longer valid");
            return nullptr;
        }

        return &itFound->second;
    }


    template <typename DataT> void DoSignal(const char* fn, unsigned sessionId, const DataT& in_data)
    {
        m_service.Log(sessionId, fn);
        m_service.Post([this, sessionId, data = in_data]()
            {
                this->Signal_(sessionId, data);
            });
    }

    template <typename DataT> void DoSignalTracking(const char* fn, const DataT& in_data)
    {
        m_service.Log(0, fn);
        m_service.Post([this, data = in_data]()
            {
                this->SignalTrackingData_(data);
            });
    }

    template<class DataT>
    void Signal_(unsigned sessionId, const DataT& data)
    {
        m_service.Log(sessionId, "Signal(", data, ')');

        auto* pSession = Session_(sessionId);
        if (!pSession)
            return m_service.Log(sessionId, "No matching session to signal to");

        pSession->Signal(data);
    }

    template<class DataT>
    void SignalTrackingData_(const DataT& data)
    {
        m_service.Log(0U, "Signal(", data, ')');

        for (auto& entry : m_sessionMap)
        {
            auto& session = entry.second;
            if (!session.OptionalPeerServiceDescriptionData() ||
                !session.OptionalPeerServiceDescriptionData()->m_supportedFeatures.m_optionalFeatureBoardTracking)
                continue;
            session.Signal(data);
        }
    }

    //================= IAcceptorCallback =========================
    void OnAccepted(std::unique_ptr<IServerSocket>&& upSocket) override
    {
        auto sessionId = upSocket->SessionId();
        m_service.Inform(sessionId, "OnAccepted: ", upSocket->GetConnectionInfo());
#ifdef _WINDOWS
        auto result = m_sessionMap.try_emplace(sessionId, std::move(upSocket), m_service, m_settings);
#else
        auto result = m_sessionMap.emplace(sessionId, VerticalService::Session(std::move(upSocket), m_service, m_settings));
#endif
        if (!result.second)
        {
            // should not really happen, unless someone launches a Denial of Service attack
            m_service.Warn(sessionId, "Duplicate session ID");
            return;
        }
        result.first->second.Connect(*this);
    }

    //================= VerticalService::ISessionCallback =========================
    void OnSocketConnected(unsigned sessionId, EVerticalState state, const ConnectionInfo& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->OnConnected(sessionId, state, in_data);
    }

    void On(unsigned sessionId, EVerticalState state, const SupervisoryServiceDescriptionData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, state, in_data);
    }

    void On(unsigned sessionId, EVerticalState, const GetConfigurationData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, in_data, pSession->PeerConnectionInfo());
    }

    void On(unsigned sessionId, EVerticalState, const SetConfigurationData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, in_data, pSession->PeerConnectionInfo());
    }

    void On(unsigned sessionId, EVerticalState, const SendWorkOrderInfoData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, in_data);
    }

    void On(unsigned sessionId, EVerticalState, const QueryHermesCapabilitiesData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, in_data);
    }

    void On(unsigned sessionId, EVerticalState, const NotificationData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, in_data);
    }

    void On(unsigned sessionId, EVerticalState, const CheckAliveData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        if (in_data.m_optionalType
            && *in_data.m_optionalType == ECheckAliveType::ePING
            && m_settings.m_checkAliveResponseMode == ECheckAliveResponseMode::eAUTO)
        {
            CheckAliveData data{ in_data };
            data.m_optionalType = ECheckAliveType::ePONG;
            m_service.Post([this, sessionId, data = std::move(data)]() { Signal_(sessionId, data); });
        }
        m_callbacks->On(sessionId, in_data);
    }

    void OnDisconnected(unsigned sessionId, EVerticalState state, const Error& error) override
    {
        // only notify if this was signalled as connected:
        if (!m_sessionMap.erase(sessionId))
            return;

        m_callbacks->OnDisconnected(sessionId, state, error);
    }
};

static VerticalServicePtrT<HermesVerticalService> DoCreateHermesVerticalService(VerticalServiceCallbackHolder&& callback)
{
    return VerticalServicePtrT<HermesVerticalService>{ new HermesVerticalService(std::move(callback)) };
}

HermesVerticalService* CreateHermesVerticalService(const HermesVerticalServiceCallbacks* pCallbacks)
{
    return DoCreateHermesVerticalService(VerticalServiceCallbackHolder{*pCallbacks}).release();
}

VerticalServicePtr Hermes::CreateHermesVerticalService(IVerticalServiceCallback& callbacks)
{
    return DoCreateHermesVerticalService(VerticalServiceCallbackHolder{ callbacks });
}

void RunHermesVerticalService(HermesVerticalService* pVerticalService)
{
    pVerticalService->Run();
}

void PostHermesVerticalService(HermesVerticalService* pVerticalService, HermesVoidCallback voidCallback)
{
    auto fn = CToCpp(voidCallback);
    pVerticalService->Post(std::move(fn));
}

void EnableHermesVerticalService(HermesVerticalService* pVerticalService,
    const HermesVerticalServiceSettings* pSettings)
{
    auto data = ToCpp(*pSettings);
    pVerticalService->Enable(data);
}

void SignalHermesVerticalServiceDescription(HermesVerticalService* pVerticalService, uint32_t sessionId,
    const HermesSupervisoryServiceDescriptionData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalService->Signal(sessionId, data);
}

void SignalHermesBoardArrived(HermesVerticalService* pVerticalService, uint32_t sessionId,
    const HermesBoardArrivedData* pData)
{
    auto data = ToCpp(*pData);
    if (sessionId == 0U)
        pVerticalService->Signal(data);
    else
        pVerticalService->Signal(sessionId, data);
}

void SignalHermesBoardDeparted(HermesVerticalService* pVerticalService, uint32_t sessionId,
    const HermesBoardDepartedData* pData)
{
    auto data = ToCpp(*pData);
    if (sessionId == 0U)
        pVerticalService->Signal(data);
    else
        pVerticalService->Signal(sessionId, data);
}

void SignalHermesSendHermesCapabilities(HermesVerticalService* pVerticalService, uint32_t sessionId,
    const HermesSendHermesCapabilitiesData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalService->Signal(sessionId, data);
}

void ResetHermesVerticalServiceSession(HermesVerticalService* pVerticalService, uint32_t sessionId, 
    const HermesNotificationData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalService->ResetSession(sessionId, data);
}

void SignalHermesQueryWorkOrderInfo(HermesVerticalService* pVerticalService, uint32_t sessionId,
    const HermesQueryWorkOrderInfoData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalService->Signal(sessionId, data);
}

void SignalHermesReplyWorkOrderInfo(HermesVerticalService* pVerticalService, uint32_t sessionId,
    const HermesReplyWorkOrderInfoData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalService->Signal(sessionId, data);
}

void SignalSendHermesCapabilitiesData(HermesVerticalService* pVerticalService, uint32_t sessionId,
    const HermesSendHermesCapabilitiesData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalService->Signal(sessionId, data);
}

void SignalHermesSendWorkOrderInfo(HermesVerticalService* pVerticalService, uint32_t sessionId,
    const HermesReplyWorkOrderInfoData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalService->Signal(sessionId, data);
}

void SignalHermesVerticalServiceNotification(HermesVerticalService* pVerticalService, uint32_t sessionId,
    const HermesNotificationData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalService->Signal(sessionId, data);
}

void SignalHermesVerticalServiceCheckAlive(HermesVerticalService* pVerticalService, uint32_t sessionId,
    const HermesCheckAliveData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalService->Signal(sessionId, data);
}

void SignalHermesVerticalCurrentConfiguration(HermesVerticalService* pVerticalService, uint32_t sessionId,
    const HermesCurrentConfigurationData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalService->Signal(sessionId, data);
}

void DisableHermesVerticalService(HermesVerticalService* pVerticalService, const HermesNotificationData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalService->Disable(data);
}

void StopHermesVerticalService(HermesVerticalService* pVerticalService)
{
    pVerticalService->Stop();
}

void DeleteHermesVerticalService(HermesVerticalService* pVerticalService)
{
    if (!pVerticalService)
        return;

    pVerticalService->Delete();
}
