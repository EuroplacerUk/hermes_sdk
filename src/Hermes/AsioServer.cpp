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
#include "AsioSocket.h"
#include "IService.h"
#include "MessageSerialization.h"
#include "StringBuilder.h"
#include "HermesChrono.hpp"

#include <HermesData.hpp>

#include <boost/asio.hpp>

#include <mutex>

namespace asio = boost::asio;

namespace Hermes
{
    using AsioSocketSp = std::shared_ptr<AsioSocket>;

    struct ServerSocket : IServerSocket
    {
        AsioSocketSp m_spSocket;

        ServerSocket(AsioSocketSp&& spSocket)
            : m_spSocket(std::move(spSocket))
        {}

        ~ServerSocket()
        {
            m_spSocket->Close();
        }

        // implementation of IServerSocket:
        unsigned SessionId() const
        {
            return m_spSocket->m_sessionId;
        }

        const ConnectionInfo& GetConnectionInfo() const override
        {
            return m_spSocket->m_connectionInfo;
        }

        const NetworkConfiguration& GetConfiguration() const override
        {
            return m_spSocket->m_configuration;
        }

        void Connect(std::weak_ptr<void> wpOwner, ISocketCallback& callback) override
        {
            m_spSocket->m_wpOwner = std::move(wpOwner);
            m_spSocket->m_pCallback = &callback;
            asio::post(m_spSocket->m_asioService, [spSocket = m_spSocket]()
            {
                if (spSocket->Closed())
                    return;
                spSocket->m_pCallback->OnConnected(spSocket->m_connectionInfo);
            });
            m_spSocket->StartReceiving();
        }

        void Send(std::string&& message) override
        {
            m_spSocket->Send(std::move(message));
        }

        asio::awaitable<void> SendAsync(std::string&& message) override
        {
            return m_spSocket->SendAsync(std::move(message));
        }

        void Close() override
        {
            m_spSocket->Close();
        }
    };

    struct AcceptorResources
    {
        asio::system_timer m_timer;
        asio::ip::tcp::acceptor m_acceptor;
        bool m_closed = false;

        AcceptorResources(asio::io_context& asioService) :
            m_timer(asioService),
            m_acceptor(asioService)
        {}
    };

    struct AsioAcceptor : IAcceptor
    {
        unsigned m_sessionId = 1U;
        IAsioService& m_service;
        asio::io_context& m_asioService{m_service.GetUnderlyingService()};
        Optional<NetworkConfiguration> m_optionalConfiguration;
        IAcceptorCallback& m_callback;
        std::shared_ptr<AcceptorResources> m_spResources{std::make_shared<AcceptorResources>(m_asioService)};

        AsioAcceptor(IAsioService& asioService, IAcceptorCallback& callback) :
            m_service(asioService),
            m_callback(callback)
        {}

        ~AsioAcceptor()
        {
            Close_();
        }

        // implementation of IAcceptor:
        void StartListening(const NetworkConfiguration& configuration) override
        {
            m_service.Log(m_sessionId, "Start Listening(", configuration, "), currentConfig=", m_optionalConfiguration);

            if (m_optionalConfiguration && *m_optionalConfiguration == configuration)
                return;

            m_optionalConfiguration = configuration;
            boost::system::error_code ecDummy;
            m_spResources->m_acceptor.cancel(ecDummy);
            m_spResources->m_acceptor.close(ecDummy);

            asio::co_spawn(m_asioService.get_executor(), ListenAsync(), asio::detached);
            //Listen_();
        }

        void StopListening() override
        {
            m_optionalConfiguration.reset();
            boost::system::error_code ecDummy;
            m_spResources->m_acceptor.close(ecDummy);
        }

        // internals
    private:

        asio::awaitable<void> ListenAsync()
        {
            auto spResources{ m_spResources };      //Assert ownership

            if (m_spResources->m_closed)
                co_return;

            if (!m_optionalConfiguration)
                co_return;

            auto& configuration = *m_optionalConfiguration;

            while (!co_await TryListenAsync(configuration))
            {
                m_spResources->m_timer.expires_after(Hermes::GetSeconds(configuration.m_retryDelayInSeconds));
                try
                {
                    co_await m_spResources->m_timer.async_wait();
                }
                catch (const boost::system::system_error&)
                {
                    co_return;
                }
                
            }


        }

        asio::awaitable<bool> TryListenAsync(const NetworkConfiguration& configuration)
        {
            m_service.Log(m_sessionId, "ListenAsync on ", configuration.m_hostName, ", port=", configuration.m_port);

            if (m_spResources->m_closed)
                co_return true;

            if (!configuration.m_hostName.empty())
            {
                asio::ip::tcp::resolver resolver(m_asioService);
                boost::system::error_code ec;
                co_await resolver.async_resolve(asio::ip::tcp::v4(), configuration.m_hostName, "", asio::redirect_error(ec));
                if (ec)
                {
                    Alarm(ec, "Unable to open accept port ", configuration.m_port);
                    co_return false;
                }
            }

            asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), configuration.m_port);

            boost::system::error_code ec;
            m_spResources->m_acceptor.open(asio::ip::tcp::v4(), ec);
            if (ec)
            {
                Alarm(ec, "Unable to open accept port ", configuration.m_port);
                co_return false;
            }

            m_spResources->m_acceptor.bind(endpoint, ec);
            if (ec)
            {
                Alarm(ec, "Unable to bind to accept port ", configuration.m_port);
                co_return false;
            }

            m_spResources->m_acceptor.listen(asio::socket_base::max_listen_connections, ec);
            if (ec)
            {
                Alarm(ec, "Unable to listen on accept port ", configuration.m_port);
                co_return false;
            }

            auto spSocket = std::make_shared<AsioSocket>(m_sessionId, configuration, m_service);
            spSocket->m_connectionInfo.m_port = configuration.m_port;
            auto& asioSocket = spSocket->m_socket;

            while (!m_spResources->m_closed)
            {
                co_await m_spResources->m_acceptor.async_accept(asioSocket, asio::redirect_error(ec));
                if (ec)
                {
                    if (ec == asio::error::operation_aborted)   //If aborted, don't retry, so return true
                        co_return true;
                    else
                    {
                        Alarm(ec, "Accept error on port ", m_optionalConfiguration->m_port);
                        co_return false;
                    }  
                }
                if(!co_await AcceptAsync(std::move(spSocket), configuration))
                    co_return false;
            }
            co_return true;
        }

        asio::awaitable<bool> AcceptAsync(AsioSocketSp&& spSocket, const NetworkConfiguration& configuration)
        {
            // we have accepted, so increment the session id:
            m_sessionId = m_sessionId == std::numeric_limits<unsigned>::max() ? 1U : m_sessionId + 1U;

            // does the socket's configuration match the current one
            if (!m_optionalConfiguration || spSocket->m_configuration != *m_optionalConfiguration)
            {
                m_service.Warn(spSocket->m_sessionId, "Configuration of accepted socket no longer matches the current configuration");
                NotificationData notification(ENotificationCode::eCONNECTION_RESET_BECAUSE_OF_CHANGED_CONFIGURATION,
                    ESeverity::eINFO, "ConfigurationChanged");
                std::string xmlString = Serialize(notification);
                co_await spSocket->SendAsync(std::move(xmlString));
                co_return true;     //Just rerun the loop
            }

            asio::ip::tcp::resolver resolver(m_asioService);
            boost::system::error_code ecResolve;
            const auto& endpoint = spSocket->m_socket.remote_endpoint();
            const auto& remoteAddress = endpoint.address();
            spSocket->m_connectionInfo.m_address = remoteAddress.to_string();

            // try and resolve the remote address to a name:
            auto resolveResults = co_await resolver.async_resolve(endpoint, asio::redirect_error(ecResolve));
            if (ecResolve)
            {
                spSocket->Alarm(ecResolve, "Unable to resolve ip address ", endpoint);
            }
            else
            {
                auto itResolved = resolveResults.cbegin();
                spSocket->m_connectionInfo.m_hostName = itResolved->host_name();
            }

            spSocket->m_service.Inform(spSocket->m_sessionId, "OnAccepted ", spSocket->m_connectionInfo);

            if (!configuration.m_hostName.empty())
            {
                auto results = co_await resolver.async_resolve(asio::ip::tcp::v4(), configuration.m_hostName, "", asio::redirect_error(ecResolve));
                if (ecResolve)
                {
                    spSocket->Alarm(ecResolve, "Unable to resolve ", configuration.m_hostName);
                    NotificationData notification(ENotificationCode::eCONFIGURATION_ERROR, ESeverity::eERROR,
                        "Connection only allowed from a hostname which cannot be resolved: " + configuration.m_hostName);
                    std::string xmlString = Serialize(notification);
                    co_await spSocket->SendAsync(std::move(xmlString));
                    spSocket->Close();
                    co_return false;
                }

                auto itAllowedEndpoint = results.cbegin();
                const auto& allowedEndpoint = itAllowedEndpoint->endpoint();
                const auto& allowedAddress = allowedEndpoint.address();
                if (remoteAddress != allowedAddress)
                {
                    std::ostringstream oss;
                    oss << "Remote host does not match allowed host " << configuration.m_hostName
                        << ",\nAllowed resolved hostname=" << itAllowedEndpoint->host_name()
                        << ",\nAllowed address=" << allowedAddress.to_string()
                        << ",\nRemote resolved hostname=" << spSocket->m_connectionInfo.m_hostName
                        << ",\nRemote address=" << spSocket->m_connectionInfo.m_address;
                    const std::string& text = oss.str();
                    m_service.Warn(m_sessionId, text);

                    NotificationData notification(ENotificationCode::eCONFIGURATION_ERROR, ESeverity::eWARNING, text);
                    std::string xmlString = Serialize(notification);
                    co_await spSocket->SendAsync(std::move(xmlString));
                    spSocket->Close();
                    co_return true;
                }
            }

            m_callback.OnAccepted(std::make_unique<ServerSocket>(std::move(spSocket)));
            co_return true;
        }

        void Close_()
        {
            if (m_spResources->m_closed)
                return;

            m_spResources->m_closed = true;
            m_service.Log(m_sessionId, "Close Acceptor");

            boost::system::error_code ecDummy;
            m_spResources->m_acceptor.close(ecDummy);
            try
            {
                m_spResources->m_timer.cancel();
            }
            catch (const boost::system::system_error&) {}
        }

        template<class... Ts>
        Error Alarm(const boost::system::error_code& ec, const Ts&... trace)
        {
            return Hermes::Alarm(m_service, m_sessionId, ec, trace...);
        }

    };
}
std::unique_ptr<Hermes::IAcceptor> Hermes::CreateAcceptor(IAsioService& asioService, IAcceptorCallback& callback)
{
    return std::make_unique<AsioAcceptor>(asioService, callback);
}
