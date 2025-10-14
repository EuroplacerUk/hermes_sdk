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
#include "MessageDispatcher.h"
#include "Service.h"
#include "StringBuilder.h"
#include "HermesChrono.hpp"

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

#include <atomic>

namespace asio = boost::asio;

std::atomic<uint32_t> m_configurationTraceId{0};

using namespace Hermes;
using namespace boost::asio::experimental::awaitable_operators;

namespace
{
    using TraceCallback = ApiCallback<HermesTraceCallback>;
    using ErrorCallback = ApiCallback<HermesErrorCallback>;
    using CurrentConfigurationCallback = ApiCallback<HermesCurrentConfigurationCallback>;
    using NotificationCallback = ApiCallback<HermesNotificationCallback>;

    // helper for both set and get:
    struct ConfigurationHelper
    {
        std::string m_traceName;
        std::string m_hostName;
        Service m_service;
        asio::io_context& m_asioService{m_service.GetUnderlyingService()};
        asio::ip::tcp::socket m_socket{m_asioService};
        MessageDispatcher m_dispatcher{0U, m_service};

        unsigned m_timeoutInSeconds = 10U;

        CurrentConfigurationCallback m_configurationCallback;
        NotificationCallback m_notificationCallback;
        ErrorCallback m_errorCallback;
        bool m_moreData;

        static const std::size_t cCHUNK_SIZE = 4096;

        ConfigurationHelper(StringView traceName, StringView  hostName,
            unsigned timeoutInSeconds,
            const CurrentConfigurationCallback& configurationCallback,
            const NotificationCallback& notificationCallback,
            const ErrorCallback& errorCallback,
            const HermesTraceCallback& traceCallback) :
            m_traceName(traceName),
            m_hostName(hostName),
            m_service(traceCallback),
            m_timeoutInSeconds(timeoutInSeconds),
            m_configurationCallback(configurationCallback),
            m_notificationCallback(notificationCallback),
            m_errorCallback(errorCallback)
        {
            m_dispatcher.Add<CurrentConfigurationData>([this](const auto& data) -> Error
            {
                const Converter2C<CurrentConfigurationData> converter (data);
                m_configurationCallback(0U, converter.CPointer());
                m_moreData = false;
                return{};

            });
            m_dispatcher.Add<NotificationData>([this](const auto& data) -> Error
            {
                const Converter2C<NotificationData> converter(data);
                m_notificationCallback(0U, converter.CPointer());
                m_moreData = true;
                return{};
            });
        }

        static void GetHermesConfiguration(StringView hostName, unsigned timeoutInSeconds, const HermesGetConfigurationCallbacks* pCallbacks)
        {
            
            ConfigurationHelper helper("GetHermesConfiguration", hostName, timeoutInSeconds,
                pCallbacks->m_currentConfigurationCallback, NotificationCallback{},
                pCallbacks->m_errorCallback, pCallbacks->m_traceCallback);

            helper.RunOne(helper.GetHermesConfigurationAsync());
        }

        static void SetHermesConfiguration(StringView hostName, const Hermes::SetConfigurationData& data, unsigned timeoutInSeconds, const HermesSetConfigurationCallbacks* pCallbacks)
        {
            ConfigurationHelper helper("SetHermesConfiguration", hostName, timeoutInSeconds,
                pCallbacks->m_currentConfigurationCallback, pCallbacks->m_notificationCallback,
                pCallbacks->m_errorCallback, pCallbacks->m_traceCallback);

            helper.RunOne(helper.SetHermesConfigurationAsync(data));
        }

    private:
        asio::awaitable<void> GetHermesConfigurationAsync()
        {
            if (!co_await ConnectAsync())
                co_return;

            co_await GetConfigurationAsync();
        }

        asio::awaitable<void> SetHermesConfigurationAsync(const Hermes::SetConfigurationData& data)
        {
            if (!co_await ConnectAsync())
                co_return;

            // send the set message:

            const std::string xmlString = Serialize(data);
            boost::system::error_code ec;
            co_await asio::async_write(m_socket, asio::buffer(xmlString.data(), xmlString.size()), asio::redirect_error(ec));
            if (ec)
                co_return GenerateError(EErrorCode::eNETWORK_ERROR, "asio::async_write", ec.message());
            m_service.Trace(ETraceType::eSENT, 0U, xmlString);

            co_await GetConfigurationAsync();
        }

        void RunOne(asio::awaitable<void>&& task)
        {
            m_asioService.restart();
            asio::co_spawn(m_asioService.get_executor(), std::move(task), asio::detached);
            try
            {
                m_asioService.poll();
            }
            catch (const boost::system::system_error& err)
            {
                return GenerateError(EErrorCode::eNETWORK_ERROR, "asio::run_one", err.what());
            }
        }

        template<class... Ts>
        void GenerateError(EErrorCode errorCode, const Ts&... params)
        {
            auto error = m_service.Alarm(0U, errorCode, params...);
            const Converter2C<Error> converter(error);
            m_errorCallback(converter.CPointer());
        }

        asio::awaitable<bool> ConnectAsync()
        {
            asio::ip::tcp::resolver resolver(m_asioService);

            boost::system::error_code ec;
            auto epResults = co_await resolver.async_resolve(asio::ip::tcp::v4(), m_hostName, "", asio::redirect_error(ec));
            if (ec)
            {
                GenerateError(EErrorCode::eNETWORK_ERROR, "asio::resolve: ", ec.message());
                co_return false;
            }

            auto itEndpoint = epResults.cbegin();
            asio::ip::tcp::endpoint endpoint(*itEndpoint);
            endpoint.port(cCONFIG_PORT);

            asio::system_timer receiveTimer{ m_asioService };
            receiveTimer.expires_after(Hermes::GetSeconds(m_timeoutInSeconds));

            try
            {

                auto result = co_await(m_socket.async_connect(endpoint, asio::use_awaitable) || receiveTimer.async_wait(asio::use_awaitable));

                if (result.index() != 0)
                    throw boost::system::system_error(asio::error::timed_out);
            }
            catch (const boost::system::system_error& err)
            {
                EErrorCode code = (err.code() == asio::error::timed_out) ? EErrorCode::eTIMEOUT : EErrorCode::eNETWORK_ERROR;
                GenerateError(code, "asio::async_connect: ", err.what());
                co_return false;
            }

            co_return true;
        }

        asio::awaitable<void> GetConfigurationAsync()
        {
            // write the get-message
            GetConfigurationData data;
            const std::string msgString = Serialize(data);
            boost::system::error_code ec;
            asio::async_write(m_socket, asio::buffer(msgString.data(), msgString.size()), asio::redirect_error(ec));
            if (ec)
            {
                GenerateError(EErrorCode::eNETWORK_ERROR, "asio::async_write: ", ec.message());
                co_return;
            }
            m_service.Trace(ETraceType::eSENT, 0U, msgString);

            // parallel to receiving, run the timer:
            asio::system_timer receiveTimer{m_asioService};
            receiveTimer.expires_after(Hermes::GetSeconds(m_timeoutInSeconds));

            std::array<char, cCHUNK_SIZE> receivedData;

            try
            {
                m_moreData = true;

                while (m_moreData)
                {
                    std::variant<size_t, std::monostate> result = co_await(m_socket.async_receive(asio::buffer(receivedData), asio::use_awaitable) || receiveTimer.async_wait(asio::use_awaitable));
                    if (result.index() != 0)
                        throw boost::system::system_error(asio::error::timed_out);
                    size_t length = std::get<size_t>(result);

                    std::span<char> span = std::span{ receivedData }.subspan(0, length);
                    auto svResult = StringSpan{ span };

                    m_service.Trace(ETraceType::eRECEIVED, 0U, StringView{ svResult });

                    if (auto error = m_dispatcher.Dispatch(svResult))
                    {
                        m_service.Alarm(0U, EErrorCode::ePEER_ERROR, error.m_text);
                    }
                }
            }
            catch (const boost::system::system_error& err)
            {
                EErrorCode code = (err.code() == asio::error::timed_out) ? EErrorCode::eTIMEOUT : EErrorCode::eNETWORK_ERROR;
                GenerateError(code, "asio::async_receive: ", err.what());
                co_return;
            }
        }
    };
}

void SetHermesConfiguration(HermesStringView hostName, const HermesSetConfigurationData* pConfiguration,
    unsigned timeoutInSeconds, const HermesSetConfigurationCallbacks* pCallbacks)
{
    SetConfigurationData data(ToCpp(*pConfiguration));
    ConfigurationHelper::SetHermesConfiguration(ToCpp(hostName), data, timeoutInSeconds, pCallbacks);

}

void GetHermesConfiguration(HermesStringView  hostName,
    unsigned timeoutInSeconds, const HermesGetConfigurationCallbacks* pCallbacks)
{
    ConfigurationHelper::GetHermesConfiguration(ToCpp(hostName), timeoutInSeconds, pCallbacks);
}

