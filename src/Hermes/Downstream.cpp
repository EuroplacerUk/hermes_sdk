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

#include <HermesDataConversion.hpp>
#include "ApiCallback.h"
#include "Network.h"
#include "DownstreamSession.h"
#include "MessageDispatcher.h"
#include "Service.h"
#include "StringBuilder.h"

#include <algorithm>
#include <memory>
#include <vector>

#include <Connection/Downstream.hpp>

using namespace Hermes;
using namespace Hermes::Implementation::Downstream;

struct DownstreamCallbackAdapter : IDownstreamCallback
{
    DownstreamCallbackAdapter(const HermesDownstreamCallbacks& callbacks) :
        m_traceCallback(callbacks.m_traceCallback),
        m_connectedCallback(callbacks.m_connectedCallback),
        m_serviceDescriptionCallback(callbacks.m_serviceDescriptionCallback),
        m_machineReadyCallback(callbacks.m_machineReadyCallback),
        m_revokeMachineReadyCallback(callbacks.m_revokeMachineReadyCallback),
        m_startTransportCallback(callbacks.m_startTransportCallback),
        m_stopTransportCallback(callbacks.m_stopTransportCallback),
        m_queryBoardInfoCallback(callbacks.m_queryBoardInfoCallback),
        m_notificationCallback(callbacks.m_notificationCallback),
        m_commandCallback(callbacks.m_commandCallback),
        m_stateCallback(callbacks.m_stateCallback),
        m_checkAliveCallback(callbacks.m_checkAliveCallback),
        m_disconnectedCallback(callbacks.m_disconnectedCallback)
    {

    }

    void OnConnected(unsigned sessionId, EState state, const ConnectionInfo& in_data) override {
        const Converter2C<ConnectionInfo> converter(in_data);
        m_connectedCallback(sessionId, ToC(state), converter.CPointer());
    };

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
    void On(unsigned sessionId, const CommandData& in_data) override
    {
        const Converter2C<CommandData> converter(in_data);
        m_commandCallback(sessionId, converter.CPointer());
    }
    void OnState(unsigned sessionId, EState state)
    {
        m_stateCallback(sessionId, ToC(state));
    }
    void OnDisconnected(unsigned sessionId, EState state, const Error& in_data)
    {
        const Converter2C<Error> converter(in_data);
        m_disconnectedCallback(sessionId, ToC(state), converter.CPointer());
    }

    void On(unsigned sessionId, EState state, const ServiceDescriptionData& in_data) override
    {
        const Converter2C<ServiceDescriptionData> converter(in_data);
        m_serviceDescriptionCallback(sessionId, ToC(state), converter.CPointer());
    }

    void On(unsigned sessionId, EState state, const MachineReadyData& in_data) override
    {
        const Converter2C<MachineReadyData> converter(in_data);
        m_machineReadyCallback(sessionId, ToC(state), converter.CPointer());
    }
    void On(unsigned sessionId, EState state, const RevokeMachineReadyData& in_data) override
    {
        const Converter2C<RevokeMachineReadyData> converter(in_data);
        m_revokeMachineReadyCallback(sessionId, ToC(state), converter.CPointer());
    }
    void On(unsigned sessionId, EState state, const StartTransportData& in_data) override
    {
        const Converter2C<StartTransportData> converter(in_data);
        m_startTransportCallback(sessionId, ToC(state), converter.CPointer());
    }
    void On(unsigned sessionId, EState state, const StopTransportData& in_data) override
    {
        const Converter2C<StopTransportData> converter(in_data);
        m_stopTransportCallback(sessionId, ToC(state), converter.CPointer());
    }
    void On(unsigned sessionId, const QueryBoardInfoData& in_data) override
    {
        const Converter2C<QueryBoardInfoData> converter(in_data);
        m_queryBoardInfoCallback(sessionId, converter.CPointer());
    }

    void OnTrace(unsigned sessionId, ETraceType traceType, StringView trace) override
    {
        m_traceCallback(sessionId, ToC(traceType), ToC(trace));
    }

private:
    ApiCallback<HermesTraceCallback> m_traceCallback;
    ApiCallback<HermesConnectedCallback> m_connectedCallback;
    ApiCallback<HermesServiceDescriptionCallback> m_serviceDescriptionCallback;
    ApiCallback<HermesMachineReadyCallback> m_machineReadyCallback;
    ApiCallback<HermesRevokeMachineReadyCallback> m_revokeMachineReadyCallback;
    ApiCallback<HermesStartTransportCallback> m_startTransportCallback;
    ApiCallback<HermesStopTransportCallback> m_stopTransportCallback;
    ApiCallback<HermesQueryBoardInfoCallback> m_queryBoardInfoCallback;
    ApiCallback<HermesNotificationCallback> m_notificationCallback;
    ApiCallback<HermesCommandCallback> m_commandCallback;
    ApiCallback<HermesStateCallback> m_stateCallback;
    ApiCallback<HermesCheckAliveCallback> m_checkAliveCallback;
    ApiCallback<HermesDisconnectedCallback> m_disconnectedCallback;
};

struct DownstreamCallbackHolder : CallbackHolder<IDownstreamCallback, DownstreamCallbackAdapter, HermesDownstreamCallbacks>
{
    DownstreamCallbackHolder(const HermesDownstreamCallbacks& callbacks) : CallbackHolder(callbacks)
    {
    }

    DownstreamCallbackHolder(IDownstreamCallback& callbacks) : CallbackHolder(callbacks)
    {
    }
};

struct HermesDownstream : IDownstream, IAcceptorCallback, ISessionCallback
{
    HermesDownstream(unsigned laneId, DownstreamCallbackHolder&& callbacks) :
        m_laneId(laneId),
        m_service(*callbacks),
        m_callbacks{ callbacks }
    {
        m_service.Inform(0U, "Created");
    }


    virtual ~HermesDownstream()
    {
        m_service.Inform(0U, "Deleted");
    }

    void Run() override
    {
        m_service.Log(0U, "RunHermesDownstream");
        m_service.Run();
    }
    void Enable(const DownstreamSettings& in_settings) override
    {
        m_service.Log(0U, "EnableHermesDownstream");

        auto settings = in_settings;
        if (!settings.m_port)
        {
            settings.m_port = static_cast<uint16_t>(cBASE_PORT + m_laneId);
        }
        m_service.Post([this, settings = std::move(settings)]()
        {
            this->Enable(settings);
        });
    }

    void Signal(unsigned sessionId, const ServiceDescriptionData& in_data) override { DoSignal("SignalHermesDownstreamServiceDescription", sessionId, in_data); }
    void Signal(unsigned sessionId, const BoardAvailableData& in_data) override { DoSignal("SignalHermesBoardAvailable", sessionId, in_data); }
    void Signal(unsigned sessionId, const RevokeBoardAvailableData& in_data) override { DoSignal("SignalHermesRevokeBoardAvailable", sessionId, in_data); }
    void Signal(unsigned sessionId, const TransportFinishedData& in_data) override { DoSignal("SignalHermesTransportFinished", sessionId, in_data); }
    void Signal(unsigned sessionId, const BoardForecastData& in_data) override { DoSignal("SignalHermesBoardForecast", sessionId, in_data); }
    void Signal(unsigned sessionId, const SendBoardInfoData& in_data) override { DoSignal("SignalHermesSendBoardInfo", sessionId, in_data); }
    void Signal(unsigned sessionId, const NotificationData& in_data) override { DoSignal("SignalHermesDownstreamNotification", sessionId, in_data); }
    void Signal(unsigned sessionId, const CheckAliveData& in_data) override { DoSignal("SignalHermesDownstreamCheckAlive", sessionId, in_data); }
    void Signal(unsigned sessionId, const CommandData& in_data) override { DoSignal("SignalHermesDownstreamCommand", sessionId, in_data); }

    void Reset(const NotificationData& in_data) override
    {
        m_service.Log(0U, "ResetHermesDownstream");
        m_service.Post([this, data = in_data]()
        {
            this->DoReset(data);
        });
    }

    // raw XML for testing
    void Signal(unsigned sessionId, StringView rawXml) override
    {
        m_service.Log(sessionId, "SignalHermesDownstreamRawXml");
        m_service.Post([this, sessionId, xmlData = std::string{ rawXml }]() mutable
        {
            MessageDispatcher dispatcher{ sessionId, m_service };
            auto parseData = xmlData;

            bool wasDispatched = false;
            dispatcher.Add<ServiceDescriptionData>([&](const auto& data) { wasDispatched = true; DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<BoardAvailableData>([&](const auto& data) { wasDispatched = true; DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<RevokeBoardAvailableData>([&](const auto& data) { wasDispatched = true; DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<TransportFinishedData>([&](const auto& data) { wasDispatched = true; DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<BoardForecastData>([&](const auto& data) { wasDispatched = true; DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<SendBoardInfoData>([&](const auto& data) { wasDispatched = true; DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<NotificationData>([&](const auto& data) { wasDispatched = true; DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<CheckAliveData>([&](const auto& data) { wasDispatched = true; DoSignal(sessionId, data, xmlData); });

            dispatcher.Dispatch(parseData);
            if (wasDispatched)
                return;

            DoSignal(sessionId, NotificationData{}, xmlData);
        });
    }
    void Reset(StringView rawXml) override
    {
        m_service.Log(0U, "ResetHermesDownstreamRawXml");
        m_service.Post([this, data = std::string{rawXml}]() mutable
        {
            if (!data.empty() && m_upSession)
            {
                m_upSession->Signal(NotificationData(), data);
            }
            DoReset();
        });
    }

    void Disable(const NotificationData& in_data) override
    {
        m_service.Log(0U, "DisableHermesDownstream");
        m_service.Post([this, data = in_data]()
        {
            DoDisable(data);
        });
    }
    void Stop() override
    {
        m_service.Log(0U, "StopHermesDownstream");
        m_service.Post([this]()
        {
            DoStop();
        });
    }
    void Delete() override
    {
        m_service.Log(0U, "DeleteHermesDownstream");
        m_service.Stop();
        delete this;
    }

    void Post(std::function<void()>&& func) override
    {
        m_service.Log(0U, "PostHermesDownstream");
        m_service.Post(std::move(func));
    }

private:
    unsigned m_laneId = 0U;
    Service m_service;
    DownstreamSettings m_settings;

    std::unique_ptr<Session> m_upSession;
    std::unique_ptr<IAcceptor>  m_upAcceptor{ CreateAcceptor(m_service, *this) };

    DownstreamCallbackHolder m_callbacks;

    bool m_enabled{ false };

    //================= forwarding calls =========================
    void DoEnable(const DownstreamSettings& settings)
    {
        m_service.Log(0U, "Enable(", settings, "); m_enabled=", m_enabled, ", m_settings=", m_settings);

        if (m_enabled && m_settings == settings)
            return;

        m_enabled = true;

        RemoveCurrentSession_(NotificationData(ENotificationCode::eCONNECTION_RESET_BECAUSE_OF_CHANGED_CONFIGURATION,
            ESeverity::eINFO, "ConfigurationChanged"));

        m_settings = settings;
        NetworkConfiguration networkConfiguration;
        networkConfiguration.m_hostName = m_settings.m_optionalClientAddress.value_or("");
        networkConfiguration.m_port = m_settings.m_port;
        networkConfiguration.m_checkAlivePeriodInSeconds = m_settings.m_checkAlivePeriodInSeconds;
        networkConfiguration.m_retryDelayInSeconds = m_settings.m_reconnectWaitTimeInSeconds;
        
        m_upAcceptor->StartListening(networkConfiguration);
    }

    template<typename DataT> void DoSignal(const char* api, unsigned sessionId, const DataT& in_data)
    {
        m_service.Log(sessionId, api);
        m_service.Post([this, sessionId, data = in_data]()
        {
            DoSignal(sessionId, data, Serialize(data));
        });
    }

    template<typename DataT>
    void DoSignal(unsigned sessionId, const DataT& data, StringView rawXml)
    {
        m_service.Log(sessionId, "Signal(", data, ',', rawXml, ')');

        auto* pSession = Session_(sessionId);
        if (!pSession)
            return m_service.Log(sessionId, "No matching session");

        pSession->Signal(data, rawXml);
    }

    void DoDisable(const NotificationData& notificationData)
    {
        m_service.Log(0U, "Disable(", notificationData, "); m_enabled=", m_enabled);

        if (!m_enabled)
            return;

        m_enabled = false;
        m_upAcceptor->StopListening();
        RemoveCurrentSession_(notificationData);
    }

    void DoStop()
    {
        m_service.Log(0U, "Stop()");
        NotificationData notificationData(ENotificationCode::eMACHINE_SHUTDOWN, ESeverity::eINFO, "Downstream service stopped by application");
        m_upAcceptor->StopListening();
        RemoveCurrentSession_(notificationData);
        m_service.Stop();
    }

    //================= IAcceptorCallback =========================
    void OnAccepted(std::unique_ptr<IServerSocket>&& upSocket) override
    {
        const auto newSessionId = upSocket->SessionId();

        // refuse connection if we have an active one:
        if (m_upSession)
        {
            m_service.Warn(m_upSession->Id(), "Due to existing session, refusing to accept new session with id=", newSessionId);

            // assemble a notification
            std::ostringstream oss;
            oss << "Refusing connection to " << upSocket->GetConnectionInfo()
                << " due to established connection to " << m_upSession->PeerConnectionInfo();

            NotificationData notification(ENotificationCode::eCONNECTION_REFUSED_BECAUSE_OF_ESTABLISHED_CONNECTION, ESeverity::eERROR
                , oss.str());
            const auto& xmlString = Serialize(notification);
            upSocket->Send(xmlString);

            // send a check alive to the current connection to reduce time for timeout detection:
            CheckAliveData checkAliveData{};
            m_upSession->Signal(checkAliveData, Serialize(checkAliveData));
            return;
        }

        m_upSession = std::make_unique<Session>(std::move(upSocket), m_service, m_settings);
        m_upSession->Connect(*this);
    }

    //================= ISessionCallback =========================
    void OnSocketConnected(unsigned sessionId, EState state, const ConnectionInfo& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->OnConnected(sessionId, state, in_data);
    }

    void On(unsigned sessionId, EState state, const ServiceDescriptionData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, state, in_data);
    }

    void On(unsigned sessionId, EState state, const MachineReadyData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, state, in_data);
    }

    void On(unsigned sessionId, EState state, const RevokeMachineReadyData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, state, in_data);
    }

    void On(unsigned sessionId, EState state, const StartTransportData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, state, in_data);
    }

    void On(unsigned sessionId, EState state, const StopTransportData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, state, in_data);
    }

    void On(unsigned sessionId, EState, const QueryBoardInfoData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, in_data);
    }

    void On(unsigned sessionId, EState, const NotificationData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, in_data);
    }

    void On(unsigned sessionId, EState, const CommandData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, in_data);
    }

    void On(unsigned sessionId, EState, const CheckAliveData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        if (in_data.m_optionalType 
            && *in_data.m_optionalType == ECheckAliveType::ePING
            && m_settings.m_checkAliveResponseMode == ECheckAliveResponseMode::eAUTO)
        {
            CheckAliveData data{in_data};
            data.m_optionalType = ECheckAliveType::ePONG;
            m_service.Post([this, sessionId, data = std::move(data)]() { DoSignal(sessionId, data, Serialize(data)); });
        }

        m_callbacks->On(sessionId, in_data);
    }

    void OnState(unsigned sessionId, EState state) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->OnState(sessionId, state);
    }

    void OnDisconnected(unsigned sessionId, EState, const Error& error) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;
        
        m_upSession.reset();

        m_callbacks->OnDisconnected(sessionId, EState::eDISCONNECTED, error);
    }

    void DoReset(const NotificationData& data)
    {
        RemoveCurrentSession_(data);
    }

    void DoReset()
    {
        RemoveCurrentSession_();
    }

    //=================== internal =========================
    private:

    Session* Session_(unsigned id)
    {
        if (m_upSession && m_upSession->Id() == id)
            return m_upSession.get();

        m_service.Warn(id, "Session ID no longer valid; current session ID=", m_upSession ? m_upSession->Id() : 0U);
        return nullptr;
    }

    void RemoveCurrentSession_()
    {
        if (!m_upSession)
            return;

        auto sessionId = m_upSession->Id();
        m_service.Log(sessionId, "RemoveCurrentSession_()");
        m_upSession->Disconnect();
        const Error error;

        m_callbacks->OnDisconnected(sessionId, EState::eDISCONNECTED, error);
        m_upSession.reset();
    }

    void RemoveCurrentSession_(const NotificationData& data)
    {
        if (!m_upSession)
            return;

        m_upSession->Signal(data, Serialize(data));
        RemoveCurrentSession_();
    }

};

//===================== implementation of public C functions ====================
static DownstreamPtrT<HermesDownstream> DoCreateHermesDownstream(uint32_t laneId, DownstreamCallbackHolder&& callback)
{
    return DownstreamPtrT<HermesDownstream>{ new HermesDownstream(laneId, std::move(callback)) };
}

#ifdef HERMES_CPP_ABI
DownstreamPtr Hermes::CreateHermesDownstream(uint32_t laneId, IDownstreamCallback& callback)
{
    return DoCreateHermesDownstream(laneId, DownstreamCallbackHolder{ callback });
}
#else
#error "HERMES_CPP_ABI should always be defined for building Hermes library"
#endif

HermesDownstream* CreateHermesDownstream(uint32_t laneId, const HermesDownstreamCallbacks* pCallbacks)
{
    return DoCreateHermesDownstream(laneId, DownstreamCallbackHolder{ *pCallbacks }).release();
}

void RunHermesDownstream(HermesDownstream* pDownstream)
{
    pDownstream->Run();
}

void EnableHermesDownstream(HermesDownstream* pDownstream, const HermesDownstreamSettings* pSettings)
{
    pDownstream->Enable(ToCpp(*pSettings));
}

void PostHermesDownstream(HermesDownstream* pDownstream, HermesVoidCallback voidCallback)
{
    auto function = [voidCallback]() {voidCallback.m_pCall(voidCallback.m_pData); };
    pDownstream->Post(function);
}

void SignalHermesDownstreamServiceDescription(HermesDownstream* pDownstream, uint32_t sessionId, const HermesServiceDescriptionData* pData)
{
    auto data = ToCpp(*pData);
    pDownstream->Signal(sessionId, data);
}

void SignalHermesBoardAvailable(HermesDownstream* pDownstream, uint32_t sessionId, const HermesBoardAvailableData* pData)
{
    auto data = ToCpp(*pData);
    pDownstream->Signal(sessionId, data);
}

void SignalHermesRevokeBoardAvailable(HermesDownstream* pDownstream, uint32_t sessionId, const HermesRevokeBoardAvailableData* pData)
{
    auto data = ToCpp(*pData);
    pDownstream->Signal(sessionId, data);
}

void SignalHermesTransportFinished(HermesDownstream* pDownstream, uint32_t sessionId, const HermesTransportFinishedData* pData)
{
    auto data = ToCpp(*pData);
    pDownstream->Signal(sessionId, data);
}

void SignalHermesBoardForecast(HermesDownstream* pDownstream, uint32_t sessionId, const HermesBoardForecastData* pData)
{
    auto data = ToCpp(*pData);
    pDownstream->Signal(sessionId, data);
}

void SignalHermesSendBoardInfo(HermesDownstream* pDownstream, uint32_t sessionId, const HermesSendBoardInfoData* pData)
{
    auto data = ToCpp(*pData);
    pDownstream->Signal(sessionId, data);
}

void SignalHermesDownstreamNotification(HermesDownstream* pDownstream, uint32_t sessionId, const HermesNotificationData* pData)
{
    auto data = ToCpp(*pData);
    pDownstream->Signal(sessionId, data);
}

void SignalHermesDownstreamCommand(HermesDownstream* pDownstream, uint32_t sessionId, const HermesCommandData* pData)
{
    auto data = ToCpp(*pData);
    pDownstream->Signal(sessionId, data);
}

void SignalHermesDownstreamCheckAlive(HermesDownstream* pDownstream, uint32_t sessionId, const HermesCheckAliveData* pData)
{
    auto data = ToCpp(*pData);
    pDownstream->Signal(sessionId, data);
}

void ResetHermesDownstream(HermesDownstream* pDownstream, const HermesNotificationData* pData)
{
    auto data = ToCpp(*pData);
    pDownstream->Reset(data);
}

void SignalHermesDownstreamRawXml(HermesDownstream* pDownstream, uint32_t sessionId, HermesStringView rawXml)
{
    pDownstream->Signal(sessionId, ToCpp(rawXml));
}

void ResetHermesDownstreamRawXml(HermesDownstream* pDownstream, HermesStringView rawXml)
{
    pDownstream->Reset(ToCpp(rawXml));
}

void DisableHermesDownstream(HermesDownstream* pDownstream, const HermesNotificationData* pData)
{
    auto data = ToCpp(*pData);
    pDownstream->Disable(data);
}

void StopHermesDownstream(HermesDownstream* pDownstream)
{
    pDownstream->Stop();
}

void DeleteHermesDownstream(HermesDownstream* pDownstream)
{
    if (!pDownstream)
        return;
    pDownstream->Delete();
}
