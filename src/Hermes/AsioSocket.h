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
#pragma once

#include "Network.h"
#include "IService.h"
#include "Network.h"
#include "MessageSerialization.h"
#include "StringBuilder.h"
#include "HermesChrono.hpp"

#include <HermesData.hpp>

#include <boost/asio.hpp>
#include "boost_win32_patch.hpp"

#include <memory>
#include <span>

namespace asio = boost::asio;

namespace Hermes
{
    template<class... Ts>
    Error Alarm(IAsioService& service, unsigned sessionId, const boost::system::error_code& ec, const Ts&... trace)
    {
        //This fixes an issue with UTF-8 in Windows leading to garbled error messages
        auto msg = boost_patch::ErrorMessageKludge(ec);
        return service.Alarm(sessionId, EErrorCode::eNETWORK_ERROR, trace..., ": ", msg, '(', ec.value(), ')');
    }

    template<class... Ts>
    void Info(IAsioService& service, unsigned sessionId, const boost::system::error_code& ec, const Ts&... trace)
    {
        service.Inform(sessionId, trace..., ": ", ec.message(), '(', ec.value(), ')');
    }

    template<class DataT, size_t length>
    class alignas(length) CircularBuffer
    {
    private:
        std::array<DataT, length> pBuffer;
        std::atomic<DataT*> pHead;
        std::atomic<DataT*> pReadTail;
        std::atomic<DataT*> pWriteTail;
    public:
        CircularBuffer()
            :pBuffer{}, pHead{ BufBegin() }, pReadTail{ BufBegin() }, pWriteTail{ BufBegin() }
        {

        }

        void Write(std::span<DataT> data)
        {
            size_t length = data.size();
            if (length > pBuffer.size())
                throw boost::system::system_error{ asio::error::message_size };

            DataT* writeStart, * writeEnd;
            do
            {
                writeStart = pWriteTail.load();
                writeEnd = Advance(writeStart, length);
            } while (!pWriteTail.compare_exchange_weak(writeStart, writeEnd));

            DataT* testHead = pHead.load();
            if ((writeStart < testHead || writeEnd < writeStart) && writeEnd > testHead)
                throw boost::system::system_error{ asio::error::no_buffer_space };

            DataT* beginBufferPtr = writeStart;
            //We can write the buffer
            if (writeEnd < writeStart)
            {
                //Goes round the end
                std::span<DataT> firstSegment = data.subspan(0, BufEnd() - writeStart);
                std::copy(firstSegment.begin(), firstSegment.end(), writeStart);
                data = data.subspan(BufEnd() - writeStart);
                beginBufferPtr = BufBegin();
            }
            std::copy(data.begin(), data.end(), beginBufferPtr);
            //Write finished, now we can advance the read tail... but only if it's our turn
            //If several writes are in flight, this waits for the first sequentially to complete, advancing its pointer, and so on
            while (!pReadTail.compare_exchange_weak(writeStart, writeEnd));

        }

        std::span<DataT> Read()
        {
            DataT* head = pHead.load();
            DataT* tail = pReadTail.load();
            if (tail < head)
                tail = BufEnd();
            return std::span<DataT>{head, tail};
        }

        void Advance(std::span<DataT> consumed)
        {
            DataT* head = consumed.data();
            assert(head == pHead.load());        //Only one reader supported at a time
            DataT* advance = Advance(consumed.data(), consumed.size());
            if (!pHead.compare_exchange_strong(head, advance))
                throw boost::system::system_error{ asio::error::interrupted };
        }

    private:
        constexpr DataT* BufBegin()
        {
            return pBuffer.data();
        }
        constexpr DataT* BufEnd()
        {
            return pBuffer.data() + pBuffer.size();
        }

        DataT* Advance(DataT* base, size_t length)
        {
            DataT* result = base + length;
            if (result >= BufEnd())
            {
                result -= pBuffer.size();
            }
            return result;
        }
    };

    struct AsioSocket
    {
        unsigned m_sessionId;
        std::weak_ptr<void> m_wpOwner;
        IAsioService& m_service;
        asio::io_context& m_asioService{ m_service.GetUnderlyingService() };
        asio::ip::tcp::socket m_socket{ m_asioService };
        asio::system_timer m_timer{ m_asioService };
        ISocketCallback* m_pCallback{ nullptr };
        NetworkConfiguration m_configuration;
        ConnectionInfo m_connectionInfo;
        std::unique_ptr<CircularBuffer<char, 4096>> m_sendBuffer;
        bool m_closed{ false };

        explicit AsioSocket(unsigned sessionId,
            const NetworkConfiguration& configuration, IAsioService& service) :
            m_sessionId(sessionId),
            m_service(service),
            m_configuration(configuration),
            m_sendBuffer(std::make_unique<CircularBuffer<char, 4096>>())
        {
        }

        AsioSocket(const AsioSocket&) = delete;
        AsioSocket& operator=(const AsioSocket&) = delete;

        ~AsioSocket()
        {
            Close_();
        }

        void StartReceiving()
        {
            assert(m_pCallback);
            asio::co_spawn(m_asioService.get_executor(), ReceiveAsync(), asio::detached);
            asio::co_spawn(m_asioService.get_executor(), CheckAliveAsync(), asio::detached);
            //AsyncReceive_();
        }

        void Send(std::string&& message)
        {
            m_sendBuffer->Write(message);
            asio::co_spawn(m_asioService.get_executor(), DoSendAsync(), asio::detached);
        }

        asio::awaitable<void> SendAsync(std::string&& message)
        {
            m_sendBuffer->Write(message);
            return DoSendAsync();
        }

        void Close()
        {
            Close_();
        }

        bool Closed() const
        {
            return m_closed;
        }

        template<class... Ts>
        Error Alarm(const boost::system::error_code& ec, const Ts&... trace)
        {
            return Hermes::Alarm(m_service, m_sessionId, ec, trace...);
        }

        template<class... Ts>
        Error Info(const boost::system::error_code& ec, const Ts&... trace)
        {
            Hermes::Info(m_service, m_sessionId, ec, trace...);
            return Error{};
        }

    private:
        std::atomic_bool m_send_running{ false };

        asio::awaitable<void> DoSendAsync()
        {
            auto strong_ref{ m_wpOwner.lock() };
            if (!strong_ref)
                co_return;

            bool notRunning = false;
            std::span<char> buffer{};

            if (!m_send_running.compare_exchange_strong(notRunning, true))
                co_return;

            do
            {
                if (!buffer.empty())
                {
                    co_await DoSendAsync(buffer);
                    m_sendBuffer->Advance(buffer);
                }

                if (m_closed)
                    break;

                buffer = m_sendBuffer->Read();

            } while (!buffer.empty());

            m_send_running.store(false);
        }

        asio::awaitable<void> DoSendAsync(std::span<char> data)
        {
            StringView message{ data };

            if (m_closed)
                co_return m_service.Log(m_sessionId, "Already closed on Send: ", message);

            boost::system::error_code ec;
            co_await asio::async_write(m_socket, asio::buffer(message.data(), message.size()), asio::redirect_error(ec));
            if (ec)
                co_return DisconnectOnError_(ec, "Cannot write ", message);

            m_service.Trace(ETraceType::eSENT, m_sessionId, message);
            RestartCheckAliveTimer_();
        }

        template<class... Ts>
        void DisconnectOnError_(const boost::system::error_code& ec, const Ts&... trace)
        {
            if (m_closed)
                return;

            // consider closed connections not to be an error:
            bool isError = (asio::error::eof != ec) && (asio::error::connection_reset != ec);
            auto error = isError ? Alarm(ec, trace...) : Info(ec, trace...);

            auto* pCallback = Close_();
            if (!pCallback)
                return;

            m_service.Inform(m_sessionId, "Disconnected");
            pCallback->OnDisconnected(error);
        }

        ISocketCallback* Close_()
        {
            if (m_closed)
                return nullptr;
            m_closed = true;
            m_service.Log(m_sessionId, "Close socket");

            boost::system::error_code ecDummy;
            m_socket.shutdown(asio::socket_base::shutdown_both, ecDummy);
            m_socket.close(ecDummy);
            try
            {
                m_timer.cancel();
            }
            catch (const boost::system::system_error&) {}

            auto* pCallback = m_pCallback;
            m_pCallback = nullptr;
            return pCallback;
        }

        //================ internally used methods, must all be called from the asio service thread =====================
        asio::awaitable<void> ReceiveAsync()
        {
            auto strong_ref{ m_wpOwner.lock() };
            if (!strong_ref)
                co_return;

            std::array<char, 1024> rxBuffer;

            while (!m_closed)
            {
                try
                {
                    m_service.Log(m_sessionId, "ReceiveAsync");
                    size_t size = co_await m_socket.async_receive(asio::buffer(rxBuffer));
                    auto span = std::span{ rxBuffer }.subspan(0, size);
                    StringSpan data{ span };
                    if (!data.empty())
                        m_service.Trace(ETraceType::eRECEIVED, m_sessionId, data);
                    if (m_closed)
                    {
                        m_service.Log(m_sessionId, "Received data, but already closed");
                        co_return;
                    }

                    assert(m_pCallback);
                    if (!m_pCallback)
                    {
                        m_service.Alarm(m_sessionId, Hermes::EErrorCode::eIMPLEMENTATION_ERROR, "Received data, but no callback");
                        co_return;
                    }

                    m_pCallback->OnReceived(data);
                }
                catch (const boost::system::system_error& err)
                {
                    co_return DisconnectOnError_(err.code(), "ReceiveAsync");
                }
            }

        }

        asio::awaitable<void> CheckAliveAsync()
        {
            auto strong_ref{ m_wpOwner.lock() };
            if (!strong_ref)
                co_return;

            while (!m_closed)
            {
                if (!m_configuration.m_checkAlivePeriodInSeconds)
                    co_return;

                try
                {
                    m_timer.expires_after(Hermes::GetSeconds(m_configuration.m_checkAlivePeriodInSeconds));
                    co_await m_timer.async_wait();
                    co_await SendAsync(Serialize(CheckAliveData()));
                }
                catch (const boost::system::error_code&)
                {
                    continue;
                }
            }

            if (!m_configuration.m_checkAlivePeriodInSeconds)
                co_return;
        }

        void RestartCheckAliveTimer_()
        {
            m_timer.cancel();
        }

    };

}

