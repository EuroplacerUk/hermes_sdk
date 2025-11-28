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
#include "ConfigurationServiceSession.h"
#include "Network.h"
#include "Service.h"

#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <Connection/ConfigurationService.hpp>


using namespace Hermes;
using namespace Hermes::Implementation;

struct ConfigurationServiceCallbackAdapter : IConfigurationServiceCallback
{
    ConfigurationServiceCallbackAdapter(const HermesConfigurationServiceCallbacks& callbacks) :
        m_traceCallback(callbacks.m_traceCallback),
        m_connectedCallback(callbacks.m_connectedCallback),
        m_disconnectedCallback(callbacks.m_disconnectedCallback),
        m_setConfigurationCallback(callbacks.m_setConfigurationCallback),
        m_getConfigurationCallback(callbacks.m_getConfigurationCallback)
    {

    }

    void OnConnected(unsigned sessionId, const ConnectionInfo& in_data) override 
    { 
        const Converter2C<ConnectionInfo> converter(in_data);
        m_connectedCallback(sessionId, eHERMES_STATE_SOCKET_CONNECTED, converter.CPointer());
    }
    void OnDisconnected(unsigned sessionId, const Error& in_data) override 
    { 
        const Converter2C<Error> converter(in_data);
        m_disconnectedCallback(sessionId, eHERMES_STATE_DISCONNECTED, converter.CPointer());
    }

    Error OnSetConfiguration(unsigned sessionId, const ConnectionInfo& connection, const SetConfigurationData& in_data) override
    {
        const Converter2C<ConnectionInfo> connectionConverter(connection);
        const Converter2C<SetConfigurationData> dataConverter(in_data);

        ResponderCapture<ISetConfigurationResponse, NotificationData> responder{ sessionId };
        CallbackScope<ISetConfigurationResponse> scope(m_pSetConfigurationResponder, responder);

        m_setConfigurationCallback(sessionId, dataConverter.CPointer(), connectionConverter.CPointer());

        auto notif = responder.GetData();
        if (notif.m_severity == ESeverity::eFATAL || notif.m_severity == ESeverity::eERROR)
        {
            return Error{ EErrorCode::eCLIENT_ERROR, notif.m_description };
        }
        return Error{};
    }

    CurrentConfigurationData OnGetConfiguration(unsigned sessionId, const ConnectionInfo& connection) override
    {
        const Converter2C<ConnectionInfo> connectionConverter(connection);
        const Converter2C<GetConfigurationData> dataConverter(GetConfigurationData{});

        ResponderCapture<IGetConfigurationResponse, CurrentConfigurationData> responder{ sessionId };

        CallbackScope<IGetConfigurationResponse> scope(m_pGetConfigurationResponder, responder);
        m_getConfigurationCallback(sessionId, dataConverter.CPointer(), connectionConverter.CPointer());
        return responder.GetData();
    }

    void OnTrace(unsigned sessionId, ETraceType traceType, StringView trace) override
    {
        m_traceCallback(sessionId, ToC(traceType), ToC(trace));
    }

    void SignalC(uint32_t sessionId, const CurrentConfigurationData& in_data)
    {
        assert(m_pGetConfigurationResponder->Id() == sessionId);
        if (!m_pGetConfigurationResponder)
            return;

        if (m_pGetConfigurationResponder->Id() != sessionId)
            return;

        m_pGetConfigurationResponder->Signal(in_data);
    }

    void SignalC(uint32_t sessionId, const NotificationData& in_data)
    {
        assert(m_pSetConfigurationResponder->Id() == sessionId);
        if (!m_pSetConfigurationResponder)
            return;

        if (m_pSetConfigurationResponder->Id() != sessionId)
            return;

        m_pSetConfigurationResponder->Signal(in_data);
    }

private:
    ApiCallback<HermesTraceCallback> m_traceCallback;
    ApiCallback<HermesConnectedCallback> m_connectedCallback;
    ApiCallback<HermesSetConfigurationCallback> m_setConfigurationCallback;
    ApiCallback<HermesGetConfigurationCallback> m_getConfigurationCallback;
    ApiCallback<HermesDisconnectedCallback> m_disconnectedCallback;

    ISetConfigurationResponse* m_pSetConfigurationResponder = nullptr; // temporarily stored during the callback
    IGetConfigurationResponse* m_pGetConfigurationResponder = nullptr; // temporarily stored during the callback

    template<class CallbackT>
    class CallbackScope
    {
    public:
        CallbackScope(CallbackT*& pCallbackHolder, CallbackT& callback) :
            m_pHolder(pCallbackHolder)
        {
            m_pHolder = &callback;
        }

        ~CallbackScope()
        {
            m_pHolder = nullptr;
        }

    private:
        CallbackT*& m_pHolder;
    };


    template<typename Interface, typename Data>
    struct ResponderCapture : Interface
    {
        ResponderCapture(uint32_t sessionId) : m_sessionId{ sessionId }
        {

        }
        uint32_t Id() const override { return m_sessionId; }
        void Signal(const Data& data) { m_data = data; }
        const Data& GetData() const { return m_data; }
    private:
        const uint32_t m_sessionId;
        Data m_data;
    };
};

struct ConfigurationServiceCallbackHolder : CallbackHolder<IConfigurationServiceCallback, ConfigurationServiceCallbackAdapter, HermesConfigurationServiceCallbacks>
{
    ConfigurationServiceCallbackHolder(const HermesConfigurationServiceCallbacks& callbacks) : CallbackHolder(callbacks)
    {
    }

    ConfigurationServiceCallbackHolder(IConfigurationServiceCallback& callbacks) : CallbackHolder(callbacks)
    {
    }

    void SignalC(uint32_t sessionId, const CurrentConfigurationData& in_data)
    {
        auto* pWrapper = get_raw_wrapper();
        if (pWrapper)
            pWrapper->SignalC(sessionId, in_data);
    }

    void SignalC(uint32_t sessionId, const NotificationData& in_data)
    {
        auto* pWrapper = get_raw_wrapper();
        if (pWrapper)
            pWrapper->SignalC(sessionId, in_data);
    }
};

struct HermesConfigurationService : IConfigurationService, IAcceptorCallback, IConfigurationServiceSessionCallback
{
    HermesConfigurationService(ConfigurationServiceCallbackHolder&& callback) :
        m_service{ *callback },
        m_callback{callback}
    {
        m_service.Inform(0U, "Created");
    }

    virtual ~HermesConfigurationService()
    {
        m_service.Inform(0U, "Deleted");
    }

    void Run() override
    {
        m_service.Log(0U, "RunHermesConfigurationService");

        m_service.Run();
    }
    void Post(std::function<void()>&& fn) override
    {
        m_service.Log(0U, "PostHermesConfigurationService");

        m_service.Post(std::move(fn));
    }
    void Enable(const ConfigurationServiceSettings& in_data) override
    {
        m_service.Log(0U, "EnableHermesConfigurationService");

        m_service.Post([this, settings = in_data]()
        {
            this->DoEnable(settings);
        });
    }
    void Disable(const NotificationData& in_data) override
    {
        m_service.Log(0U, "DisableHermesConfigurationService");

        m_service.Post([this, data = in_data]()
        {
            this->DoDisable(data);
        });
    }
    void Stop() override
    {
        m_service.Log(0U, "StopHermesConfigurationService");

        m_service.Post([this]()
        {
            this->DoStop();
        });
    }
    void Delete() override
    {
        m_service.Log(0U, "DeleteHermesConfigurationService");

        m_service.Stop();
        delete this;
    }

    //C/C++ thunk API
    void Signal(uint32_t sessionId, const CurrentConfigurationData& in_data)
    {
        m_service.Log(sessionId, "SignalHermesCurrentConfiguration");
        m_callback.SignalC(sessionId, in_data);
    }

    void Signal(uint32_t sessionId, const NotificationData& in_data)
    {
        m_service.Log(sessionId, "SignalHermesConfigurationNotification");
        m_callback.SignalC(sessionId, in_data);
    }

private:
    Service m_service;
    ConfigurationServiceSettings m_settings;;

    // we only hold on to the accepting session
    std::unique_ptr<IAcceptor> m_upAcceptor{ CreateAcceptor(m_service, *this) };
    std::map<unsigned, ConfigurationServiceSession> m_sessionMap;

    ConfigurationServiceCallbackHolder m_callback;

    bool m_enabled{ false };


    //================= forwarding calls =========================
    void DoEnable(const ConfigurationServiceSettings& settings)
    {
        m_service.Log(0U, "Enable(", settings, "); m_enabled=", m_enabled, ", m_settings=", m_settings);

        if (m_enabled && m_settings == settings)
            return;

        m_enabled = true;
        m_settings = settings;

        NetworkConfiguration networkConfiguration;
        networkConfiguration.m_port = settings.m_port ? settings.m_port : cCONFIG_PORT;
        networkConfiguration.m_retryDelayInSeconds = settings.m_reconnectWaitTimeInSeconds;
        networkConfiguration.m_checkAlivePeriodInSeconds = 0U;

        m_upAcceptor->StartListening(networkConfiguration);
    }

    void DoDisable(const NotificationData& data)
    {
        m_service.Log(0U, "Disable(", data, "); m_enabled=", m_enabled);

        if (!m_enabled)
            return;

        m_enabled = false;
        m_upAcceptor->StopListening();
        RemoveSessions_(NotificationData(ENotificationCode::eCONNECTION_RESET_BECAUSE_OF_CHANGED_CONFIGURATION, ESeverity::eINFO,
            "Configuration service disabled"));
    }

    void DoStop()
    {
        m_service.Log(0U, "Stop(), sessionCount=", m_sessionMap.size());

        NotificationData notificationData(ENotificationCode::eMACHINE_SHUTDOWN, ESeverity::eINFO, "Configuration service stopped by application");
        RemoveSessions_(notificationData);
        m_service.Stop();
    }

    void RemoveSessions_(const NotificationData& data)
    {
        auto sessionMap = std::move(m_sessionMap);
        m_sessionMap.clear();

        for (auto& entry : sessionMap)
        {
            entry.second.Disconnect(data);
        }
    }

    //================= IAcceptorCallback =========================
    void OnAccepted(std::unique_ptr<IServerSocket>&& upSocket) override
    {
        auto sessionId = upSocket->SessionId();
        m_service.Inform(sessionId, "OnAccepted: ", upSocket->GetConnectionInfo());
#ifdef _WINDOWS
        auto result = m_sessionMap.try_emplace(sessionId, std::move(upSocket), m_service, m_settings, *this);
#else
        auto result = m_sessionMap.emplace(sessionId, ConfigurationServiceSession(std::move(upSocket), m_service, m_settings, *this));
#endif
        if (!result.second)
        {
            // should not really happen, unless someone launches a Denial of Service attack
            m_service.Warn(sessionId, "Duplicate session ID");
            return;
        }
        result.first->second.Connect();
    }

    //================= IConfigurationServiceSessionCallback =========================
    void OnSocketConnected(unsigned sessionId, const ConnectionInfo& data) override { m_callback->OnConnected(sessionId, data); }

    virtual void OnGet(unsigned sessionId, const ConnectionInfo& connectionInfo, const GetConfigurationData&, IGetConfigurationResponse& responder) override
    {
        auto configuration = m_callback->OnGetConfiguration(sessionId, connectionInfo);
        responder.Signal(configuration);
    }

    virtual void OnSet(unsigned sessionId, const ConnectionInfo& connectionInfo,
        const SetConfigurationData& data, ISetConfigurationResponse& responder) override
    {
        auto error = m_callback->OnSetConfiguration(sessionId, connectionInfo, data);

        if (!error)
            return;

        NotificationData notification(ENotificationCode::eCONFIGURATION_ERROR, ESeverity::eERROR, error.m_text);

        responder.Signal(notification);
    }

    virtual void OnDisconnected(unsigned sessionId, const Error& error) override
    {
        // only notify if this was signalled as connected:
        if (!m_sessionMap.erase(sessionId))
            return;

        m_callback->OnDisconnected(sessionId, error);
    }
};

HermesConfigurationService* CreateHermesConfigurationService(const HermesConfigurationServiceCallbacks* pCallbacks)
{
    return new HermesConfigurationService(*pCallbacks);
}

void RunHermesConfigurationService(HermesConfigurationService* pConfigurationService)
{
    pConfigurationService->Run();
}

void PostHermesConfigurationService(HermesConfigurationService* pConfigurationService, HermesVoidCallback voidCallback)
{
    pConfigurationService->Post(CToCpp(voidCallback));
}

void EnableHermesConfigurationService(HermesConfigurationService* pConfigurationService, const HermesConfigurationServiceSettings* pSettings)
{
    auto data = ToCpp(*pSettings);
    pConfigurationService->Enable(data);
}

void SignalHermesCurrentConfiguration(HermesConfigurationService* pConfigurationService, uint32_t sessionId, const HermesCurrentConfigurationData* pConfiguration)
{
    auto data = ToCpp(*pConfiguration);
    pConfigurationService->Signal(sessionId, data);
}

void SignalHermesConfigurationNotification(HermesConfigurationService* pConfigurationService, uint32_t sessionId, const HermesNotificationData* pNotification)
{
    auto data = ToCpp(*pNotification);
    pConfigurationService->Signal(sessionId, data);
}

void DisableHermesConfigurationService(HermesConfigurationService* pConfigurationService, const HermesNotificationData* pData)
{
    auto data = ToCpp(*pData);
    pConfigurationService->Disable(data);
}

void StopHermesConfigurationService(HermesConfigurationService* pConfigurationService)
{
    pConfigurationService->Stop();
}

void DeleteHermesConfigurationService(HermesConfigurationService* pConfigurationService)
{
    pConfigurationService->Delete();
}
