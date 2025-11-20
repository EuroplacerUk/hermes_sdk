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

#pragma once

#include <HermesDataConversion.hpp>
#include "ApiCallback.h"
#include "IService.h"

#include <boost/asio.hpp>

#include <memory>

namespace asio = boost::asio;

namespace Hermes
{
    struct TraceCallbackAdapter : ITraceCallback
    {
        TraceCallbackAdapter(const HermesTraceCallback& callback) :
            m_traceCallback(callback)
        {

        }
        void OnTrace(unsigned sessionId, ETraceType tt, StringView trace)
        {
            m_traceCallback(sessionId, ToC(tt), ToC(trace));
        }

    private:
        ApiCallback<HermesTraceCallback> m_traceCallback;
    };

    struct TraceCallbackHolder : CallbackHolder<ITraceCallback, TraceCallbackAdapter, HermesTraceCallback>
    {
        TraceCallbackHolder(const HermesTraceCallback& callback) : CallbackHolder(callback)
        {
        }

        TraceCallbackHolder(ITraceCallback& callback) : CallbackHolder(callback)
        {
        }
    };

    struct Service : IAsioService
    {
        asio::io_context m_asioService;
        asio::executor_work_guard<asio::io_context::executor_type> m_asioWork{asio::make_work_guard(m_asioService) };
        TraceCallbackHolder m_traceCallback;

        explicit Service(HermesTraceCallback traceCallback) :
            m_traceCallback(traceCallback)
        {}

        explicit Service(ITraceCallback& traceCallback) :
            m_traceCallback(traceCallback)
        {
        }

        ~Service()
        {}

        void Run()
        {
            try
            {
                m_asioService.run();
            }
            catch (const boost::system::system_error& err)
            {
                Trace(ETraceType::eERROR, 0U, BuildString("m_asioService.Run: ", err.what()));
            }
            
        }

        void Stop()
        {
            m_asioService.stop();
        }

        //============== IAsioService ==========================
        void Post(std::function<void()>&& f) override
        {
            asio::post(m_asioService, std::move(f));
        }

        void Trace(ETraceType type, unsigned sessionId, StringView trace) override
        {
            m_traceCallback->OnTrace(sessionId, type, trace);
        }

        boost::asio::io_context& GetUnderlyingService() override
        {
            return m_asioService;
        }
    };
}
