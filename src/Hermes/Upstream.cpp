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

#include "stdafx.h"

#include <HermesDataConversion.hpp>
#include "ApiCallback.h"
#include "MessageDispatcher.h"
#include "Service.h"
#include "UpstreamSession.h"
#include "HermesChrono.hpp"

#include <cassert>
#include <memory>

#include <Connection/Upstream.hpp>

using namespace Hermes;
using namespace Hermes::Implementation::Upstream;

struct UpstreamCallbackAdapter : IUpstreamCallback
{
    UpstreamCallbackAdapter(const HermesUpstreamCallbacks& callbacks) :
        m_traceCallback(callbacks.m_traceCallback),
        m_connectedCallback(callbacks.m_connectedCallback),
        m_serviceDescriptionCallback(callbacks.m_serviceDescriptionCallback),
        m_boardAvailableCallback(callbacks.m_boardAvailableCallback),
        m_revokeBoardAvailableCallback(callbacks.m_revokeBoardAvailableCallback),
        m_transportFinishedCallback(callbacks.m_transportFinishedCallback),
        m_boardForecastCallback(callbacks.m_boardForecastCallback),
        m_sendBoardInfoCallback(callbacks.m_sendBoardInfoCallback),
        m_notificationCallback(callbacks.m_notificationCallback),
        m_commandCallback(callbacks.m_commandCallback),
        m_checkAliveCallback(callbacks.m_checkAliveCallback),
        m_stateCallback(callbacks.m_stateCallback),
        m_disconnectedCallback(callbacks.m_disconnectedCallback)
    {

    }

    template<typename HermesT, typename DataT> void OnCallback(ApiCallback<HermesT>& callback, unsigned sessionId, EState state, const DataT& in_data)
    {
        const Converter2C<DataT> converter(in_data);
        callback(sessionId, ToC(state), converter.CPointer());
    }
    template<typename HermesT, typename DataT> void OnCallback(ApiCallback<HermesT>& callback, unsigned sessionId, const DataT& in_data)
    {
        const Converter2C<DataT> converter(in_data);
        callback(sessionId, converter.CPointer());
    }

    void OnConnected(unsigned sessionId, EState state, const ConnectionInfo& in_data) override { OnCallback(m_connectedCallback, sessionId, state, in_data); }
    void OnDisconnected(unsigned sessionId, EState state, const Error& in_data) override { OnCallback(m_disconnectedCallback, sessionId, state, in_data); }

    void On(unsigned sessionId, const NotificationData& in_data) override { OnCallback(m_notificationCallback, sessionId, in_data); }
    void On(unsigned sessionId, const CheckAliveData& in_data) override { OnCallback(m_checkAliveCallback, sessionId, in_data); }
    void On(unsigned sessionId, const CommandData& in_data) override { OnCallback(m_commandCallback, sessionId, in_data); }
    void On(unsigned sessionId, EState state, const ServiceDescriptionData& in_data) override { OnCallback(m_serviceDescriptionCallback, sessionId, state, in_data); }
    void On(unsigned sessionId, EState state, const BoardAvailableData& in_data) override { OnCallback(m_boardAvailableCallback, sessionId, state, in_data); }
    void On(unsigned sessionId, EState state, const RevokeBoardAvailableData& in_data) override { OnCallback(m_revokeBoardAvailableCallback, sessionId, state, in_data); }
    void On(unsigned sessionId, EState state, const TransportFinishedData& in_data) override { OnCallback(m_transportFinishedCallback, sessionId, state, in_data); }
    void On(unsigned sessionId, EState state, const BoardForecastData& in_data) override { OnCallback(m_boardForecastCallback, sessionId, state, in_data); }
    void On(unsigned sessionId, const SendBoardInfoData& in_data) override { OnCallback(m_sendBoardInfoCallback, sessionId, in_data); }

    void OnState(unsigned sessionId, EState state)
    {
        m_stateCallback(sessionId, ToC(state));
    }

    void OnTrace(unsigned sessionId, ETraceType traceType, StringView trace) override
    {
        m_traceCallback(sessionId, ToC(traceType), ToC(trace));
    }

private:
    ApiCallback<HermesTraceCallback> m_traceCallback;
    ApiCallback<HermesConnectedCallback> m_connectedCallback;
    ApiCallback<HermesServiceDescriptionCallback> m_serviceDescriptionCallback;
    ApiCallback<HermesBoardAvailableCallback> m_boardAvailableCallback;
    ApiCallback<HermesRevokeBoardAvailableCallback> m_revokeBoardAvailableCallback;
    ApiCallback<HermesTransportFinishedCallback> m_transportFinishedCallback;
    ApiCallback<HermesBoardForecastCallback> m_boardForecastCallback;
    ApiCallback<HermesSendBoardInfoCallback> m_sendBoardInfoCallback;
    ApiCallback<HermesNotificationCallback> m_notificationCallback;
    ApiCallback<HermesCommandCallback> m_commandCallback;
    ApiCallback<HermesCheckAliveCallback> m_checkAliveCallback;
    ApiCallback<HermesStateCallback> m_stateCallback;
    ApiCallback<HermesDisconnectedCallback> m_disconnectedCallback;
};

struct UpstreamCallbackHolder : CallbackHolder<IUpstreamCallback, UpstreamCallbackAdapter, HermesUpstreamCallbacks>
{
    UpstreamCallbackHolder(const HermesUpstreamCallbacks& callbacks) : CallbackHolder(callbacks)
    {
    }

    UpstreamCallbackHolder(IUpstreamCallback& callbacks) : CallbackHolder(callbacks)
    {
    }
};

struct HermesUpstream : IUpstream, ISessionCallback
{
    HermesUpstream(unsigned laneId, UpstreamCallbackHolder&& callbacks) :
        m_laneId(laneId),
        m_service(*callbacks),
        m_callbacks{callbacks}
    {
        m_service.Inform(0U, "Created");
    }

    virtual ~HermesUpstream()
    {
        m_service.Inform(0U, "Deleted");
    }

    void Run() override
    {
        m_service.Log(0U, "RunHermesUpstream");

        m_service.Run();
    }
    void Enable(const UpstreamSettings& in_settings) override
    {
        m_service.Log(0U, "EnableHermesUpstream");
        auto settings = in_settings;
        if (!settings.m_port)
        {
            settings.m_port = static_cast<uint16_t>(cBASE_PORT + m_laneId);
        }

        m_service.Post([this, settings = std::move(settings)]()
        {
            this->DoEnable(settings);
        });
    }

    void Signal(unsigned sessionId, const ServiceDescriptionData& in_data) override { DoSignal("SignalHermesUpstreamServiceDescription", sessionId, in_data); }
    void Signal(unsigned sessionId, const MachineReadyData& in_data) override { DoSignal("SignalHermesMachineReady", sessionId, in_data); }
    void Signal(unsigned sessionId, const RevokeMachineReadyData& in_data) override { DoSignal("SignalHermesRevokeMachineReady", sessionId, in_data); }
    void Signal(unsigned sessionId, const StartTransportData& in_data) override { DoSignal("SignalHermesStartTransport", sessionId, in_data); }
    void Signal(unsigned sessionId, const StopTransportData& in_data) override { DoSignal("SignalHermesStopTransport", sessionId, in_data); }
    void Signal(unsigned sessionId, const QueryBoardInfoData& in_data) override { DoSignal("SignalHermesQueryBoardInfo", sessionId, in_data); }
    void Signal(unsigned sessionId, const NotificationData& in_data) override { DoSignal("SignalHermesUpstreamNotification", sessionId, in_data); }
    void Signal(unsigned sessionId, const CheckAliveData& in_data) override { DoSignal("SignalHermesUpstreamCheckAlive", sessionId, in_data); }
    void Signal(unsigned sessionId, const CommandData& in_data) override { DoSignal("SignalHermesUpstreamCommand", sessionId, in_data); }
    void Reset(const NotificationData& in_data) override
    {
        m_service.Log(0U, "ResetHermesUpstream");

        m_service.Post([this, data = in_data]()
        {
            DoReset(data);
        });
    }

    // raw XML for testing
    void Signal(unsigned sessionId, StringView rawXml) override
    {
        m_service.Log(sessionId, "SignalHermesUpstreamRawXml");
        m_service.Post([this, sessionId, xmlData = std::string{rawXml}]() mutable
        {
            MessageDispatcher dispatcher{ sessionId, m_service };
            auto parseData = xmlData;

            bool wasDispatched = false;
            dispatcher.Add<ServiceDescriptionData>([&](const auto& data) { wasDispatched = true; DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<MachineReadyData>([&](const auto& data) { wasDispatched = true; DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<RevokeMachineReadyData>([&](const auto& data) { wasDispatched = true; DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<StartTransportData>([&](const auto& data) { wasDispatched = true; DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<StopTransportData>([&](const auto& data) { wasDispatched = true; DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<QueryBoardInfoData>([&](const auto& data) { wasDispatched = true; DoSignal(sessionId, data, xmlData); });
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
        m_service.Log(0U, "ResetHermesUpstreamRawXml");
        m_service.Post([this, data = std::string{rawXml}]()
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
        m_service.Log(0U, "DisableHermesUpstream");
        m_service.Post([this, data = in_data]()
        {
            DoDisable(data);
        });
    }
    void Stop() override
    {
        m_service.Log(0U, "StopHermesUpstream");

        m_service.Post([this]()
        {
            DoStop();
        });
    }
    void Delete() override
    {
        m_service.Log(0U, "DeleteHermesUpstream");

        m_service.Stop();
        delete this;
    }

    void Post(std::function<void()>&& fn) override
    {
        m_service.Log(0U, "PostHermesUpstream");

        m_service.Post(std::move(fn));
    }

private:
    unsigned m_laneId = 0U;
    Service m_service;
    asio::system_timer m_timer{ m_service.GetUnderlyingService() };
    UpstreamSettings m_settings;

    unsigned m_sessionId{ 0U };
    unsigned m_connectedSessionId{ 0U };

    UpstreamCallbackHolder m_callbacks;

    std::unique_ptr<Session> m_upSession;

    bool m_enabled{ false };

    void DoEnable(const UpstreamSettings& settings)
    {
        m_service.Log(0U, "Enable(", settings, "); m_enabled=", m_enabled, ", m_settings=", m_settings);

        if (m_enabled && settings == m_settings)
            return;

        m_enabled = true;
        RemoveSession_(NotificationData(ENotificationCode::eCONNECTION_RESET_BECAUSE_OF_CHANGED_CONFIGURATION,
            ESeverity::eINFO, "ConfigurationChanged"));

        m_settings = settings;
        CreateNewSession_();
    }

    void DoDisable(const NotificationData& notificationData)
    {
        m_service.Log(0U, "Disable(", notificationData, "); m_enabled=", m_enabled);

        if (!m_enabled)
            return;

        m_enabled = false;
        RemoveSession_(notificationData);
    }

    template<typename DataT> void DoSignal(const char* api, unsigned sessionId, const DataT& in_data)
    {
        m_service.Log(sessionId, api);
        m_service.Post([this, sessionId, data = in_data]()
            {
                DoSignal(sessionId, data, Serialize(data));
            });
    }

    template<class DataT>
    void DoSignal(unsigned sessionId, const DataT& data, StringView rawXml)
    {
        m_service.Log(sessionId, "Signal(", data, ',', rawXml, ')');

        auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        pSession->Signal(data, rawXml);
    }

    void DoStop()
    {
        m_service.Log(0U, "Stop()");

        NotificationData notificationData(ENotificationCode::eMACHINE_SHUTDOWN, ESeverity::eINFO, "Upstream service stopped by application");
        RemoveSession_(notificationData);
        m_service.Stop();
    }

    //================= ISessionCallback =========================
    void OnSocketConnected(unsigned sessionId, EState state, const ConnectionInfo& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_connectedSessionId = pSession->Id();
        m_callbacks->OnConnected(m_connectedSessionId, state, in_data);
    }

    void On(unsigned sessionId, EState state, const ServiceDescriptionData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, state, in_data);
    }

    void On(unsigned sessionId, EState state, const BoardAvailableData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, state, in_data);
    }

    void On(unsigned sessionId, EState state, const RevokeBoardAvailableData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, state, in_data);
    }

    void On(unsigned sessionId, EState state, const TransportFinishedData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, state, in_data);
    }

    void On(unsigned sessionId, EState state, const BoardForecastData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, state, in_data);
    }

    void On(unsigned sessionId, EState, const SendBoardInfoData& in_data) override
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

    void OnDisconnected(unsigned sessionId, EState state, const Error& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        if (pSession->OptionalPeerServiceDescriptionData())
        {
            DelayCreateNewSession_(1.0);
        }
        else
        {
            DelayCreateNewSession_(m_settings.m_reconnectWaitTimeInSeconds);
        }

        m_upSession.reset();

        m_callbacks->OnDisconnected(sessionId, state, in_data);
    }

    void DoReset(const NotificationData& data)
    {
        RemoveSession_(data);
        DelayCreateNewSession_(1.0);
    }

    void DoReset()
    {
        RemoveSession_();
        DelayCreateNewSession_(1.0);
    }

    //============ internal impplementation ==============
    private:

    Session* Session_(unsigned id)
    {
        if (m_upSession && m_upSession->Id() == id)
            return m_upSession.get();

        m_service.Warn(id, "Session ID no longer valid; current session ID=", m_upSession ? m_upSession->Id() : 0U);
        return nullptr;
    }

    void RemoveSession_()
    {
        if (!m_upSession)
            return;

        auto sessionId = m_upSession->Id();
        m_service.Log(sessionId, "RemoveSession_()");

        m_upSession->Disconnect();
        m_upSession.reset();

        if (sessionId != m_connectedSessionId)
            return;

        m_connectedSessionId = 0U;
        Error error;

        m_callbacks->OnDisconnected(sessionId, EState::eDISCONNECTED, error);
    }

    void RemoveSession_(const NotificationData& data)
    {
        if (!m_upSession)
            return;

        m_upSession->Signal(data, Serialize(data));
        RemoveSession_();
    }

    void CreateNewSession_()
    {
        if (m_upSession)
            return;

        if (!m_enabled)
        {
            m_service.Log(0U, "CreateNewSession_, but disabled");
            return;
        }

        if (!++m_sessionId) { ++m_sessionId; } // avoid zero sessionId

        m_upSession = std::make_unique<Session>(m_sessionId, m_service, m_settings, *this);
        m_upSession->Connect();
    }

    asio::awaitable<void> DoDelayCreateNewSession(double delay)
    {
        m_service.Log(0U, "DelayCreateNewSession_");

        try {
            m_timer.expires_after(Hermes::GetSeconds(delay));
            co_await m_timer.async_wait();
        }
        catch (const boost::system::system_error&) {
            co_return;
        }
        CreateNewSession_();
    }

    void DelayCreateNewSession_(double delay)
    {
        asio::co_spawn(m_service.GetUnderlyingService(), DoDelayCreateNewSession(delay), asio::detached);
    }
};

//===================== implementation of public C functions =====================
static UpstreamPtrT<HermesUpstream> DoCreateHermesUpstream(uint32_t laneId, UpstreamCallbackHolder&& callback)
{
    return UpstreamPtrT<HermesUpstream>{ new HermesUpstream(laneId, std::move(callback)) };
}

#ifdef HERMES_CPP_ABI
UpstreamPtr Hermes::CreateHermesUpstream(uint32_t laneId, IUpstreamCallback& callback)
{
    return DoCreateHermesUpstream(laneId, UpstreamCallbackHolder{ callback });
}
#else
#error "HERMES_CPP_ABI should always be defined for building Hermes library"
#endif

HermesUpstream* CreateHermesUpstream(uint32_t laneId, const HermesUpstreamCallbacks* pCallbacks)
{
    return DoCreateHermesUpstream(laneId, UpstreamCallbackHolder{ *pCallbacks }).release();
}

void RunHermesUpstream(HermesUpstream* pUpstream)
{
    pUpstream->Run();
}

void PostHermesUpstream(HermesUpstream* pUpstream, HermesVoidCallback voidCallback)
{
    auto fn = CToCpp(voidCallback);
    pUpstream->Post(std::move(fn));

}

void EnableHermesUpstream(HermesUpstream* pUpstream, const HermesUpstreamSettings* pSettings)
{
    auto data = ToCpp(*pSettings);
    pUpstream->Enable(data);
}

void SignalHermesUpstreamServiceDescription(HermesUpstream* pUpstream, uint32_t sessionId, const HermesServiceDescriptionData* pData)
{
    auto data = ToCpp(*pData);
    pUpstream->Signal(sessionId, data);
}

void SignalHermesMachineReady(HermesUpstream* pUpstream, uint32_t sessionId, const HermesMachineReadyData* pData)
{
    auto data = ToCpp(*pData);
    pUpstream->Signal(sessionId, data);
}

void SignalHermesRevokeMachineReady(HermesUpstream* pUpstream, uint32_t sessionId, const HermesRevokeMachineReadyData* pData)
{
    auto data = ToCpp(*pData);
    pUpstream->Signal(sessionId, data);
}

void SignalHermesStartTransport(HermesUpstream* pUpstream, uint32_t sessionId, const HermesStartTransportData* pData)
{
    auto data = ToCpp(*pData);
    pUpstream->Signal(sessionId, data);
}

void SignalHermesQueryBoardInfo(HermesUpstream* pUpstream, uint32_t sessionId, const HermesQueryBoardInfoData* pData)
{
    auto data = ToCpp(*pData);
    pUpstream->Signal(sessionId, data);
}

void SignalHermesStopTransport(HermesUpstream* pUpstream, uint32_t sessionId, const HermesStopTransportData* pData)
{
    auto data = ToCpp(*pData);
    pUpstream->Signal(sessionId, data);
}

void SignalHermesUpstreamNotification(HermesUpstream* pUpstream, uint32_t sessionId, const HermesNotificationData* pData)
{
    auto data = ToCpp(*pData);
    pUpstream->Signal(sessionId, data);
}

void SignalHermesUpstreamCommand(HermesUpstream* pUpstream, uint32_t sessionId, const HermesCommandData* pData)
{
    auto data = ToCpp(*pData);
    pUpstream->Signal(sessionId, data);
}

void SignalHermesUpstreamCheckAlive(HermesUpstream* pUpstream, uint32_t sessionId, const HermesCheckAliveData* pData)
{
    auto data = ToCpp(*pData);
    pUpstream->Signal(sessionId, data);
}

void ResetHermesUpstream(HermesUpstream* pUpstream, const HermesNotificationData* pData)
{    
    auto data = ToCpp(*pData);
    pUpstream->Reset(data);
}

void SignalHermesUpstreamRawXml(HermesUpstream* pUpstream, uint32_t sessionId, HermesStringView rawXml)
{
    pUpstream->Signal(sessionId, ToCpp(rawXml));
}

void ResetHermesUpstreamRawXml(HermesUpstream* pUpstream, HermesStringView rawXml)
{
    pUpstream->Reset(ToCpp(rawXml));
}

void DisableHermesUpstream(HermesUpstream* pUpstream, const HermesNotificationData* pData)
{
    auto data = ToCpp(*pData);
    pUpstream->Disable(data);
}

void StopHermesUpstream(HermesUpstream* pUpstream)
{
    pUpstream->Stop();
}

void DeleteHermesUpstream(HermesUpstream* pUpstream)
{
    if (!pUpstream)
        return;

    pUpstream->Delete();
}
