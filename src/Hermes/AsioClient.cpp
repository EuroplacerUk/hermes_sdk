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
#include "Network.h"

#include "AsioSocket.h"

#include <HermesData.hpp>
#include "IService.h"

#include <boost/asio.hpp>

#include <memory>

namespace asio = boost::asio;

namespace Hermes
{
    struct ClientSocket : IClientSocket
    {
        AsioSocket m_socket;

        ClientSocket(unsigned sessionId, const NetworkConfiguration& configuration, 
            IAsioService& asioService) :
            m_socket(sessionId, configuration, asioService)
        {}

        ~ClientSocket()
        {
            m_socket.Close();
        }

        //========================= implementation of IClientSocket ============================
        unsigned SessionId() const
        {
            return m_socket.m_sessionId;
        }

        const ConnectionInfo& GetConnectionInfo() const override
        {
            return m_socket.m_connectionInfo;
        }

        const NetworkConfiguration& GetConfiguration() const override
        {
            return m_socket.m_configuration;
        }
        
        void Connect(std::weak_ptr<void> wpOwner, ISocketCallback& callback) override
        {
            assert(!m_socket.m_pCallback);
            m_socket.m_wpOwner = wpOwner;
            m_socket.m_pCallback = &callback;
            asio::co_spawn(m_socket.m_asioService.get_executor(), ConnectAsync(), asio::detached);
        }

        void Send(std::string&& message) override 
        { 
            m_socket.Send(std::move(message)); 
        }

        asio::awaitable<void> SendAsync(std::string&& message) override
        {
            return m_socket.SendAsync(std::move(message));
        }

        void Close() override 
        { 
            m_socket.Close(); 
        }

        //================= implementation ===============================
    private:

        std::shared_ptr<ClientSocket> shared_from_this()
        {
            return std::shared_ptr<ClientSocket>(m_socket.m_wpOwner.lock(), this);
        }

        asio::awaitable<void> ConnectAsync()
        {
            auto strong_ref{ m_socket.m_wpOwner.lock() };
            if (!strong_ref) co_return;

            auto& configuration = m_socket.m_configuration;

            try
            {
                while (!co_await TryConnectAsync(configuration))
                {
                    //Retry later
                    m_socket.m_timer.expires_after(Hermes::GetSeconds(m_socket.m_configuration.m_retryDelayInSeconds));
                    co_await m_socket.m_timer.async_wait();
                }
            }
            catch (const boost::system::system_error&)
            {
                if(m_socket.Closed())
                    m_socket.m_service.Log(m_socket.m_sessionId, "ConnectAsync, but already closed");
            }
        }

        asio::awaitable<bool> TryConnectAsync(const Hermes::NetworkConfiguration& configuration)
        {
            if (m_socket.Closed())
            {
                auto err = asio::error::make_error_code(asio::error::connection_aborted);
                throw boost::system::system_error{ err };
            }

            auto callback = m_socket.m_pCallback;
            assert(callback);
            if (!callback)
            {
                m_socket.m_service.Alarm(m_socket.m_sessionId, Hermes::EErrorCode::eIMPLEMENTATION_ERROR,
                    "Connected, but no callback");
                co_return true;     //No handling of this condition, so don't retry
            }

            m_socket.m_service.Log(m_socket.m_sessionId, "ConnectAsync to host=", configuration.m_hostName, " on port=", configuration.m_port);
            asio::ip::tcp::resolver resolver(m_socket.m_asioService);

            boost::system::error_code ec;
            auto resolveResults = co_await resolver.async_resolve(asio::ip::tcp::v4(), configuration.m_hostName, "", asio::redirect_error(ec));
            if (ec)
            {
                m_socket.Alarm(ec, "Unable to resolve ", m_socket.m_configuration.m_hostName);
                co_return false;
            }

            auto itEndpoint = resolveResults.cbegin();
            asio::ip::tcp::endpoint endpoint(*itEndpoint);
            endpoint.port(m_socket.m_configuration.m_port);

            m_socket.m_connectionInfo.m_address = endpoint.address().to_string();
            m_socket.m_connectionInfo.m_port = endpoint.port();
            m_socket.m_connectionInfo.m_hostName = itEndpoint->host_name();

            m_socket.m_service.Log(m_socket.m_sessionId, "Connecting to ", m_socket.m_connectionInfo, " ...");

            co_await m_socket.m_socket.async_connect(endpoint, asio::redirect_error(ec));
            if (ec)
            {
                m_socket.Alarm(ec, "Unable to connect to ", m_socket.m_connectionInfo);
                co_return false;
            }


            m_socket.m_service.Inform(m_socket.m_sessionId, "OnConnected ", m_socket.m_connectionInfo);
            callback->OnConnected(m_socket.m_connectionInfo);
            m_socket.StartReceiving();

            co_return true;
        }
    };
}

std::unique_ptr<Hermes::IClientSocket> Hermes::CreateClientSocket(unsigned sessionId,
    const NetworkConfiguration& configuration, IAsioService& asioService)
{
    return std::make_unique<ClientSocket>(sessionId, configuration, asioService);
}
