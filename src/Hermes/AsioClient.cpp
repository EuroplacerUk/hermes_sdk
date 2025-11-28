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
        
        void Connect(std::weak_ptr<void> wpOwner, CallbackReference<ISocketCallback>&& callback) override
        {
            m_socket.ConnectOwner(std::move(wpOwner), std::move(callback));
            AsyncConnect_();
        }

        void Send(StringView message) override 
        { 
            m_socket.Send(message); 
        }

        void Close() override 
        { 
            m_socket.Close(); 
        }

        //================= implementation ===============================
    private:

        std::shared_ptr<ClientSocket> shared_from_this()
        {
            return std::shared_ptr<ClientSocket>(m_socket.shared_from_this(), this);
        }

        void AsyncConnect_()
        {
            if (m_socket.Closed())
            {
                m_socket.m_service.Log(m_socket.m_sessionId, "AsyncConnect_, but already closed");
                return;
            }

            m_socket.m_service.Log(m_socket.m_sessionId, "AsyncConnect_ to host=", 
                m_socket.m_configuration.m_hostName, " on port=", m_socket.m_configuration.m_port);

            asio::ip::tcp::resolver resolver(m_socket.m_asioService);

            boost::system::error_code ec;
            auto resolveResults = resolver.resolve(asio::ip::tcp::v4(), m_socket.m_configuration.m_hostName, "", ec);
            if (ec)
            {
                m_socket.Alarm(ec, "Unable to resolve ", m_socket.m_configuration.m_hostName);
                RetryLater_();
                return;
            }
                
            auto itEndpoint = resolveResults.cbegin();
            asio::ip::tcp::endpoint endpoint(*itEndpoint);
            endpoint.port(m_socket.m_configuration.m_port);

            m_socket.m_connectionInfo.m_address = endpoint.address().to_string();
            m_socket.m_connectionInfo.m_port = endpoint.port();
            m_socket.m_connectionInfo.m_hostName = itEndpoint->host_name();

            m_socket.m_service.Log(m_socket.m_sessionId, "Connecting to ", m_socket.m_connectionInfo, " ...");
            m_socket.m_socket.async_connect(endpoint, 
                [spThis = shared_from_this()](const boost::system::error_code& ec)
            {
                if (spThis->m_socket.Closed())
                    return;

                spThis->OnConnected_(ec);
            });
        }

        void OnConnected_(const boost::system::error_code& ec)
        {
            if (ec)
            {
                m_socket.Alarm(ec, "Unable to connect to ", m_socket.m_connectionInfo);
                RetryLater_();
                return;
            }

            m_socket.OnConnected();
            m_socket.StartReceiving();
        }

        void RetryLater_()
        {
            if (m_socket.Closed())
                return;

            m_socket.m_timer.expires_after(Hermes::GetSeconds(m_socket.m_configuration.m_retryDelayInSeconds));
            m_socket.m_timer.async_wait([spThis = shared_from_this()](const boost::system::error_code& ec)
            {
                if (spThis->m_socket.Closed())
                    return;

                if (ec) // we were canccelled
                    return;

                spThis->AsyncConnect_();
            });
        }
    };
}

std::unique_ptr<Hermes::IClientSocket> Hermes::CreateClientSocket(unsigned sessionId,
    const NetworkConfiguration& configuration, IAsioService& asioService)
{
    return std::make_unique<ClientSocket>(sessionId, configuration, asioService);
}
