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
#include "VerticalClientSession.h"
#include "HermesChrono.hpp"

#include <cassert>
#include <memory>

#include <Connection/VerticalClient.hpp>

using namespace Hermes;
using namespace Hermes::Implementation::VerticalClient;

struct VerticalClientCallbackAdapter : IVerticalClientCallback
{
    VerticalClientCallbackAdapter(const HermesVerticalClientCallbacks& callbacks) :
        m_traceCallback(callbacks.m_traceCallback),
        m_connectedCallback(callbacks.m_connectedCallback),
        m_serviceDescriptionCallback(callbacks.m_serviceDescriptionCallback),
        m_boardArrivedCallback(callbacks.m_boardArrivedCallback),
        m_boardDepartedCallback(callbacks.m_boardDepartedCallback),
        m_queryWorkOrderInfoCallback(callbacks.m_queryWorkOrderInfoCallback),
        m_replyWorkOrderInfoCallback(callbacks.m_replyWorkOrderInfoCallback),
        m_currentConfigurationCallback(callbacks.m_currentConfigurationCallback),
        m_sendHermesCapabilitiesCallback(callbacks.m_sendHermesCapabilitiesCallback),
        m_notificationCallback(callbacks.m_notificationCallback),
        m_checkAliveCallback(callbacks.m_checkAliveCallback),
        m_disconnectedCallback(callbacks.m_disconnectedCallback)
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
    void On(unsigned sessionId, const BoardArrivedData& in_data) override
    {
        const Converter2C<BoardArrivedData> configurationConverter(in_data);
        m_boardArrivedCallback(sessionId, configurationConverter.CPointer());
    }
    void On(unsigned sessionId, const BoardDepartedData& in_data) override
    {
        const Converter2C<BoardDepartedData> configurationConverter(in_data);
        m_boardDepartedCallback(sessionId, configurationConverter.CPointer());
    }
    void On(unsigned sessionId, const QueryWorkOrderInfoData& in_data) override
    {
        const Converter2C<QueryWorkOrderInfoData> converter(in_data);
        m_queryWorkOrderInfoCallback(sessionId, converter.CPointer());
    }
    void On(unsigned sessionId, const ReplyWorkOrderInfoData& in_data) override
    {
        const Converter2C<ReplyWorkOrderInfoData> converter(in_data);
        m_replyWorkOrderInfoCallback(sessionId, converter.CPointer());
    }
    void On(unsigned sessionId, const CurrentConfigurationData& in_data) override
    {
        const Converter2C<CurrentConfigurationData> converter(in_data);
        m_currentConfigurationCallback(sessionId, converter.CPointer());
    }
    void On(unsigned sessionId, const SendHermesCapabilitiesData& in_data) override
    {
        const Converter2C<SendHermesCapabilitiesData> converter(in_data);
        m_sendHermesCapabilitiesCallback(sessionId, converter.CPointer());
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
    ApiCallback<HermesBoardArrivedCallback> m_boardArrivedCallback;
    ApiCallback<HermesBoardDepartedCallback> m_boardDepartedCallback;
    ApiCallback<HermesQueryWorkOrderInfoCallback> m_queryWorkOrderInfoCallback;
    ApiCallback<HermesReplyWorkOrderInfoCallback> m_replyWorkOrderInfoCallback;
    ApiCallback<HermesCurrentConfigurationCallback> m_currentConfigurationCallback;
    ApiCallback<HermesSendHermesCapabilitiesCallback> m_sendHermesCapabilitiesCallback;
    ApiCallback<HermesNotificationCallback> m_notificationCallback;
    ApiCallback<HermesCheckAliveCallback> m_checkAliveCallback;
    ApiCallback<HermesVerticalDisconnectedCallback> m_disconnectedCallback;
};

struct VerticalClientCallbackHolder : CallbackHolder<IVerticalClientCallback, VerticalClientCallbackAdapter, HermesVerticalClientCallbacks>
{
    VerticalClientCallbackHolder(const HermesVerticalClientCallbacks& callbacks) : CallbackHolder(callbacks)
    {
    }

    VerticalClientCallbackHolder(IVerticalClientCallback& callbacks) : CallbackHolder(callbacks)
    {
    }
};

struct HermesVerticalClient : IVerticalClient, ISessionCallback
{
    HermesVerticalClient(VerticalClientCallbackHolder&& callbacks) :
        m_service(*callbacks),
        m_callbacks { callbacks }
    {
        m_service.Inform(0U, "Created");
    }

    virtual ~HermesVerticalClient()
    {
        m_service.Inform(0U, "Deleted");
    }

    void Run() override {
        m_service.Log(0U, "RunHermesVerticalClient");

        m_service.Run();
    }
    void Post(std::function<void()>&& fn) override {
        m_service.Log(0U, "PostHermesVerticalClient");

        m_service.Post(std::move(fn));
    }

    void Enable(const VerticalClientSettings& in_settings) override {
        m_service.Log(0U, "EnableHermesVerticalClient");
        auto settings = in_settings;
        if (!settings.m_port)
        {
            settings.m_port = static_cast<uint16_t>(cBASE_PORT);
        }

        m_service.Post([this, settings = std::move(settings)]()
        {
            this->DoEnable(settings);
        });
    }

    void Signal(unsigned sessionId, const SupervisoryServiceDescriptionData& in_data)  override { DoSignal("SignalHermesVerticalClientDescription", sessionId, in_data); }
    void Signal(unsigned sessionId, const GetConfigurationData& in_data)  override { DoSignal("SignalHermesVerticalGetConfiguration", sessionId, in_data); }
    void Signal(unsigned sessionId, const SetConfigurationData& in_data) override { DoSignal("SignalHermesVerticalSetConfiguration", sessionId, in_data); }
    void Signal(unsigned sessionId, const QueryHermesCapabilitiesData& in_data)  override { DoSignal("SignalHermesQueryHermesCapabilities", sessionId, in_data); }
    void Signal(unsigned sessionId, const SendWorkOrderInfoData& in_data) override { DoSignal("SignalHermesSendWorkOrderInfo", sessionId, in_data); }
    void Signal(unsigned sessionId, const NotificationData& in_data)  override { DoSignal("SignalHermesVerticalClientNotification", sessionId, in_data); }
    void Signal(unsigned sessionId, const CheckAliveData& in_data)  override { DoSignal("SignalHermesVerticalClientCheckAlive", sessionId, in_data); }
    void Reset(const NotificationData& in_data)  override {  
        m_service.Log(0U, "ResetHermesVerticalClient");

        m_service.Post([this, data = in_data]()
        {
            this->DoReset(data);
        });
    }

    // raw XML for testing
    virtual void Signal(unsigned sessionId, StringView rawXml)  override {
        m_service.Log(sessionId, "SignalHermesVerticalClientRawXml");
        m_service.Post([this, sessionId, xmlData = std::string{ rawXml }]() mutable
        {
            MessageDispatcher dispatcher{ sessionId, m_service };
            auto parseData = xmlData;

            bool wasDispatched = false;
            dispatcher.Add<SupervisoryServiceDescriptionData>([&](const auto& data) { wasDispatched = true; this->DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<SendWorkOrderInfoData>([&](const auto& data) { wasDispatched = true; this->DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<GetConfigurationData>([&](const auto& data) { wasDispatched = true; this->DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<SetConfigurationData>([&](const auto& data) { wasDispatched = true; this->DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<NotificationData>([&](const auto& data) { wasDispatched = true; this->DoSignal(sessionId, data, xmlData); });
            dispatcher.Add<CheckAliveData>([&](const auto& data) { wasDispatched = true; this->DoSignal(sessionId, data, xmlData); });

            dispatcher.Dispatch(parseData);
            if (wasDispatched)
                return;

            this->DoSignal(sessionId, NotificationData{}, xmlData);
        });
    }
    virtual void Reset(StringView rawXml) override {
        m_service.Log(0U, "ResetHermesVerticalClientRawXml");
        m_service.Post([this, data = std::string{ rawXml }]()
        {
            if (!data.empty() && m_upSession)
            {
                m_upSession->Signal(NotificationData(), data);
            }
            DoReset();
        });
    }

    void Disable(const NotificationData& in_data)  override {
        m_service.Log(0U, "DisableHermesVerticalClient");
        m_service.Post([this, data = in_data]()
        {
            this->DoDisable(data);
        });
    }
    virtual void Stop()  override {
        m_service.Log(0U, "StopHermesVerticalClient");

        m_service.Post([this]()
        {
            this->DoStop();
        });
    }
    virtual void Delete() override {
        m_service.Log(0U, "DeleteHermesVerticalClient");

        m_service.Stop();
        delete this;
    }

private:
    Service m_service;
    asio::system_timer m_timer{ m_service.GetUnderlyingService() };
    VerticalClientSettings m_settings;

    unsigned m_sessionId{ 0U };
    unsigned m_connectedSessionId{ 0U };

    VerticalClientCallbackHolder m_callbacks;

    std::unique_ptr<Session> m_upSession;

    bool m_enabled{ false };

    void DoEnable(const VerticalClientSettings& settings)
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

    template <typename DataT> void DoSignal(const char* fn, unsigned sessionId, const DataT& in_data)
    {
        m_service.Log(sessionId, fn);
        m_service.Post([this, sessionId, data = in_data]()
        {
            this->DoSignal(sessionId, data, Serialize(data));
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

        NotificationData notificationData(ENotificationCode::eMACHINE_SHUTDOWN, ESeverity::eINFO, "Vertial client stopped by application");
        RemoveSession_(notificationData);
        m_service.Stop();
    }

    //================= ISessionCallback =========================
    void OnSocketConnected(unsigned sessionId, EVerticalState state, const ConnectionInfo& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_connectedSessionId = pSession->Id();
        m_callbacks->OnConnected(m_connectedSessionId, state, in_data);
    }

    void On(unsigned sessionId, EVerticalState state, const SupervisoryServiceDescriptionData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, state, in_data);
    }

    void On(unsigned sessionId, EVerticalState, const BoardArrivedData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, in_data);
    }

    void On(unsigned sessionId, EVerticalState, const BoardDepartedData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, in_data);
    }

    void On(unsigned sessionId, EVerticalState, const QueryWorkOrderInfoData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, in_data);
    }

    void On(unsigned sessionId, EVerticalState, const ReplyWorkOrderInfoData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, in_data);
    }

    void On(unsigned sessionId, EVerticalState, const CurrentConfigurationData& in_data) override
    {
        const auto* pSession = Session_(sessionId);
        if (!pSession)
            return;

        m_callbacks->On(sessionId, in_data);
    }

    void On(unsigned sessionId, EVerticalState, const SendHermesCapabilitiesData& in_data) override
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
            m_service.Post([this, sessionId, data = std::move(data)]() { DoSignal(sessionId, data, Serialize(data)); });
        }
        m_callbacks->On(sessionId, in_data);
    }

    void OnDisconnected(unsigned sessionId, EVerticalState state, const Error& error) override
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
        m_callbacks->OnDisconnected(sessionId, state, error);
    }

    void DoReset()
    {
        RemoveSession_();
        DelayCreateNewSession_(1.0);
    }

    void DoReset(const NotificationData& data)
    {
        RemoveSession_(data);
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
        m_callbacks->OnDisconnected(sessionId, EVerticalState::eDISCONNECTED, error);
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

    void DelayCreateNewSession_(double delay)
    {
        m_service.Log(0U, "DelayCreateNewSession_");

        m_timer.expires_after(Hermes::GetSeconds(delay));
        m_timer.async_wait([this](const boost::system::error_code& ec)
        {
            if (ec) // timer cancelled or whatever
                return;
            CreateNewSession_();
        });
    }
};

//===================== implementation of public C functions =====================
static VerticalClientPtrT<HermesVerticalClient> DoCreateHermesVerticalClient(VerticalClientCallbackHolder&& callback)
{
    return VerticalClientPtrT<HermesVerticalClient>{ new HermesVerticalClient(std::move(callback)) };
}

VerticalClientPtr Hermes::CreateHermesVerticalClient(IVerticalClientCallback& callback)
{
    return DoCreateHermesVerticalClient(VerticalClientCallbackHolder{ callback });
}

HermesVerticalClient* CreateHermesVerticalClient(const HermesVerticalClientCallbacks* pCallbacks)
{
    return DoCreateHermesVerticalClient(VerticalClientCallbackHolder{ *pCallbacks }).release();
}

void RunHermesVerticalClient(HermesVerticalClient* pVerticalClient)
{
    pVerticalClient->Run();
}

void PostHermesVerticalClient(HermesVerticalClient* pVerticalClient, HermesVoidCallback voidCallback)
{
    auto fn = CToCpp(voidCallback);
    pVerticalClient->Post(std::move(fn));
}

void EnableHermesVerticalClient(HermesVerticalClient* pVerticalClient, const HermesVerticalClientSettings* pSettings)
{
    auto data = ToCpp(*pSettings);
    pVerticalClient->Enable(data);
}

void SignalHermesVerticalClientDescription(HermesVerticalClient* pVerticalClient, uint32_t sessionId, const HermesSupervisoryServiceDescriptionData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalClient->Signal(sessionId, data);
}

void SignalHermesSendWorkOrderInfo(HermesVerticalClient* pVerticalClient, uint32_t sessionId, const HermesSendWorkOrderInfoData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalClient->Signal(sessionId, data);
}

void SignalHermesVerticalGetConfiguration(HermesVerticalClient* pVerticalClient, uint32_t sessionId, const HermesGetConfigurationData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalClient->Signal(sessionId, data);
}

void SignalHermesVerticalSetConfiguration(HermesVerticalClient* pVerticalClient, uint32_t sessionId, const HermesSetConfigurationData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalClient->Signal(sessionId, data);
}

void SignalHermesVerticalQueryHermesCapabilities(HermesVerticalClient* pVerticalClient, uint32_t sessionId, const HermesQueryHermesCapabilitiesData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalClient->Signal(sessionId, data);
}

void SignalHermesVerticalClientNotification(HermesVerticalClient* pVerticalClient, uint32_t sessionId, const HermesNotificationData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalClient->Signal(sessionId, data);
}

void SignalHermesVerticalClientCheckAlive(HermesVerticalClient* pVerticalClient, uint32_t sessionId, const HermesCheckAliveData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalClient->Signal(sessionId, data);
}

void ResetHermesVerticalClient(HermesVerticalClient* pVerticalClient, const HermesNotificationData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalClient->Reset(data);
}


void DisableHermesVerticalClient(HermesVerticalClient* pVerticalClient, const HermesNotificationData* pData)
{
    auto data = ToCpp(*pData);
    pVerticalClient->Disable(data);
}

void StopHermesVerticalClient(HermesVerticalClient* pVerticalClient)
{
    pVerticalClient->Stop();
}

void DeleteHermesVerticalClient(HermesVerticalClient* pVerticalClient)
{
    if (!pVerticalClient)
        return;

    pVerticalClient->Delete();
}

void SignalHermesVerticalClientRawXml(HermesVerticalClient* pVerticalClient, uint32_t sessionId, HermesStringView rawXml)
{
    pVerticalClient->Signal(sessionId, ToCpp(rawXml));
}

void ResetHermesVerticalClientRawXml(HermesVerticalClient* pVerticalClient, HermesStringView rawXml)
{
    pVerticalClient->Reset(ToCpp(rawXml));
}
