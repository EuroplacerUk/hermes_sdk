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

#include "DownstreamSession.h"

#include "Network.h"
#include "DownstreamSerializer.h"
#include "DownstreamStateMachine.h"
#include <HermesData.hpp>
#include "IService.h"
#include "StringBuilder.h"

#include "Network.h"

namespace Hermes
{
    namespace asio = boost::asio;
    namespace Implementation
    {
        namespace Downstream
        {
            struct Session::Impl : IStateMachineCallback, std::enable_shared_from_this<Impl>
            {
                Impl(std::unique_ptr<IServerSocket>&& upSocket, IAsioService& service, const DownstreamSettings& configuration) :
                    m_id(upSocket->SessionId()),
                    m_service(service),
                    m_configuration(configuration),
                    m_upSocket(std::move(upSocket)),
                    m_cbSelf{*this},
                    m_pCallback{nullptr}
                {
                    auto sessionId = m_id;
                    m_service.Log(m_id, "DownstreamSession()");

                    m_upSerializer = CreateSerializer(sessionId, service, *m_upSocket);
                    m_upStateMachine = CreateStateMachine(sessionId, service, *m_upSerializer, configuration.m_checkState);
                }

                ~Impl()
                {
                    m_service.Log(m_id, "~DownstreamSession()");
                }

                void Connect(CallbackReference<ISessionCallback>&& callback)
                {
                    m_pCallback = std::move(callback);
                    m_upStateMachine->Connect(shared_from_this(), m_cbSelf);
                }
                void Disconnect()
                {
                    m_pCallback = nullptr;
                    m_upStateMachine->Disconnect();
                }

                //============= implementation of IStateMachineCallback ============
                void OnSocketConnected(EState state, const ConnectionInfo& connectionInfo) override
                {
                    m_peerConnectionInfo = connectionInfo;
                    if (!m_pCallback)
                        return;

                    m_pCallback->OnSocketConnected(m_id, state, connectionInfo);
                }

                template<class DataT> void Signal_(const DataT& data, StringView rawXml)
                {
                    m_upStateMachine->Signal(data, rawXml);
                }

                template<class DataT> void On_(EState state, const DataT& data)
                {
                    if (!m_pCallback)
                        return;

                    m_pCallback->On(m_id, state, data);
                }

                void On(EState state, const ServiceDescriptionData& data)
                {
                    m_optionalPeerServiceDescriptionData = data;
                    On_(state, data);
                }

                void On(EState state, const MachineReadyData& data) override { On_(state, data); }
                void On(EState state, const RevokeMachineReadyData& data) override { On_(state, data); }
                void On(EState state, const StartTransportData& data) override { On_(state, data); }
                void On(EState state, const StopTransportData& data) override { On_(state, data); }
                void On(EState state, const QueryBoardInfoData& data) override { On_(state, data); }
                void On(EState state, const NotificationData& data) override { On_(state, data); }
                void On(EState state, const CommandData& data) override { On_(state, data); }
                void On(EState state, const CheckAliveData& data) override { On_(state, data); }
                void OnState(EState state) override
                {
                    if (!m_pCallback)
                        return;

                    m_pCallback->OnState(m_id, state);
                }

                void OnDisconnected(EState state, const Error& data) override
                {
                    if (!m_pCallback)
                        return;

                    auto* pCallback = m_pCallback.get_raw();
                    m_pCallback = nullptr;
                    pCallback->OnDisconnected(m_id, state, data);
                }

                unsigned Id() const { return m_id; }
                const Optional<ServiceDescriptionData>& OptionalPeerServiceDescriptionData() const { return m_optionalPeerServiceDescriptionData; }
                const ConnectionInfo& PeerConnectionInfo() const { return m_peerConnectionInfo; }
            private:
                unsigned m_id;
                IAsioService& m_service;
                DownstreamSettings m_configuration;

                std::unique_ptr<IServerSocket> m_upSocket;
                std::unique_ptr<ISerializer> m_upSerializer;
                std::unique_ptr<IStateMachine> m_upStateMachine;
                Optional<ServiceDescriptionData> m_optionalPeerServiceDescriptionData;
                ConnectionInfo m_peerConnectionInfo;

                bool m_hasServiceDescriptionData{ false };

                CallbackLifetime<IStateMachineCallback> m_cbSelf;
                CallbackReference<ISessionCallback> m_pCallback;
            };


            Session::Session(std::unique_ptr<IServerSocket>&& upSocket, IAsioService& service, const DownstreamSettings& configuration, ISessionCallback& callback) :
                m_callback{callback}
            {
                m_spImpl = std::make_shared<Impl>(std::move(upSocket), service, configuration);
            }

            Session::~Session()
            {
            }

            unsigned Session::Id() const { return m_spImpl->Id(); }
            const Optional<ServiceDescriptionData>& Session::OptionalPeerServiceDescriptionData() const { return m_spImpl->OptionalPeerServiceDescriptionData(); }
            const ConnectionInfo& Session::PeerConnectionInfo() const { return m_spImpl->PeerConnectionInfo(); }

            void Session::Connect()
            {
                m_spImpl->Connect(m_callback);
            }

            void Session::Signal(const ServiceDescriptionData& data, StringView rawXml) { m_spImpl->Signal_(data, rawXml); }
            void Session::Signal(const BoardAvailableData& data, StringView rawXml) { m_spImpl->Signal_(data, rawXml); }
            void Session::Signal(const RevokeBoardAvailableData& data, StringView rawXml) { m_spImpl->Signal_(data, rawXml); }
            void Session::Signal(const TransportFinishedData& data, StringView rawXml) { m_spImpl->Signal_(data, rawXml); }
            void Session::Signal(const BoardForecastData& data, StringView rawXml) { m_spImpl->Signal_(data, rawXml); }
            void Session::Signal(const SendBoardInfoData& data, StringView rawXml) { m_spImpl->Signal_(data, rawXml); }
            void Session::Signal(const NotificationData& data, StringView rawXml) { m_spImpl->Signal_(data, rawXml); }
            void Session::Signal(const CommandData& data, StringView rawXml) { m_spImpl->Signal_(data, rawXml); }
            void Session::Signal(const CheckAliveData& data, StringView rawXml) { m_spImpl->Signal_(data, rawXml); }

            void Session::Disconnect()
            {
                m_spImpl->Disconnect();
            }
        }
    }
}