/***********************************************************************
Copyright 2018 ASM Assembly Systems GmbH & Co. KG
2025 Europlacer Ltd

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

#include "HermesDataConversion.hpp"

#include <functional>
#include <memory>

namespace Hermes
{

    struct IDownstreamCallback : ITraceCallback
    {
        IDownstreamCallback() = default;

        // callbacks must not be moved or copied!
        IDownstreamCallback(const IDownstreamCallback&) = delete;
        IDownstreamCallback& operator=(const IDownstreamCallback&) = delete;

        virtual void OnConnected(unsigned sessionId, EState, const ConnectionInfo&) = 0;
        virtual void On(unsigned sessionId, const NotificationData&) = 0;
        virtual void On(unsigned /*sessionId*/, const CheckAliveData&) {} // not necessarily interesting, hence not abstract
        virtual void On(unsigned sessionId, const CommandData&) = 0;
        virtual void OnState(unsigned sessionId, EState) = 0;
        virtual void OnDisconnected(unsigned sessionId, EState, const Error&) = 0;

        virtual void On(unsigned sessionId, EState, const ServiceDescriptionData&) = 0;
        virtual void On(unsigned sessionId, EState, const MachineReadyData&) = 0;
        virtual void On(unsigned sessionId, EState, const RevokeMachineReadyData&) = 0;
        virtual void On(unsigned sessionId, EState, const StartTransportData&) = 0;
        virtual void On(unsigned sessionId, EState, const StopTransportData&) = 0;
        virtual void On(unsigned /*sessionId*/, const QueryBoardInfoData&) {} // not necessarily interesting, hence not abstract

    protected:
        virtual ~IDownstreamCallback() = default;
    };

    // Convenience interface for situations in which the same class implements IUpstreamCallback and IDownstreamCallback.
    // Because some function names clash, this adds a level of indirection to different function names, eg.
    // On(unsigned, const ServiceDescriptionData&)
    // becomes:
    // OnUpstream(unsigned, const ServiceDescriptionData&)
    // and
    // OnDownstream(unsigned, const ServiceDescriptionData&)
    //
    // Deriving from UpstreamCallbackHelper and DownstreamCallbackHelper gives unique names for all callback methods.
    // Unambiguous methods, eg. On(unsigned, EState, const BoardAvailableData&) remain unchanged!
    struct DownstreamCallbackHelper : IDownstreamCallback
    {
        virtual void OnDownstreamConnected(unsigned sessionId, EState, const ConnectionInfo&) = 0;
        virtual void OnDownstream(unsigned sessionId, EState, const ServiceDescriptionData&) = 0;
        virtual void OnDownstream(unsigned sessionId, const NotificationData& data) = 0;
        virtual void OnDownstream(unsigned sessionId, const CheckAliveData& data) = 0;
        virtual void OnDownstream(unsigned sessionId, const CommandData& data) = 0;
        virtual void OnDownstreamState(unsigned sessionId, EState) = 0;
        virtual void OnDownstreamDisconnected(unsigned sessionId, EState, const Error&) = 0;
        virtual void OnDownstreamTrace(unsigned sessionId, ETraceType, StringView trace) = 0;

        void OnConnected(unsigned sessionId, EState state, const ConnectionInfo& data) override { OnDownstreamConnected(sessionId, state, data); }
        void On(unsigned sessionId, EState state, const ServiceDescriptionData& data) override { OnDownstream(sessionId, state, data); }
        void On(unsigned sessionId, const NotificationData& data) override { OnDownstream(sessionId, data); }
        void On(unsigned sessionId, const CheckAliveData& data) override { OnDownstream(sessionId, data); }
        void On(unsigned sessionId, const CommandData& data) override { OnDownstream(sessionId, data); }
        void OnState(unsigned sessionId, EState state) override { OnDownstreamState(sessionId, state); }
        void OnDisconnected(unsigned sessionId, EState state, const Error& data) override { OnDownstreamDisconnected(sessionId, state, data); }
        void OnTrace(unsigned sessionId, ETraceType type, StringView data) override { OnDownstreamTrace(sessionId, type, data); }
    };

    //======================= Downstream interface =====================================
    struct IDownstream
    {
        virtual void Run() = 0;
        virtual void Enable(const DownstreamSettings&) = 0;

        virtual void Signal(unsigned sessionId, const ServiceDescriptionData&) = 0;
        virtual void Signal(unsigned sessionId, const BoardAvailableData&) = 0;
        virtual void Signal(unsigned sessionId, const RevokeBoardAvailableData&) = 0;
        virtual void Signal(unsigned sessionId, const TransportFinishedData&) = 0;
        virtual void Signal(unsigned sessionId, const BoardForecastData&) = 0;
        virtual void Signal(unsigned sessionId, const SendBoardInfoData&) = 0;
        virtual void Signal(unsigned sessionId, const NotificationData&) = 0;
        virtual void Signal(unsigned sessionId, const CheckAliveData&) = 0;
        virtual void Signal(unsigned sessionId, const CommandData&) = 0;
        virtual void Reset(const NotificationData&) = 0;

        // raw XML for testing
        virtual void Signal(unsigned sessionId, StringView rawXml) = 0;
        virtual void Reset(StringView rawXml) = 0;

        virtual void Disable(const NotificationData&) = 0;
        virtual void Stop() = 0;
        virtual void Delete() = 0;

        virtual void Post(std::function<void()>&&) = 0;

        template<class F> void Post(F&& func) { Post(std::function<void()>{func}); }

        virtual ~IDownstream() {}
    };



#ifdef HERMES_CPP_ABI
    struct DownstreamDeleter {
        void operator()(IDownstream* ptr) { ptr->Delete(); }
    };

    template<typename T>
    using DownstreamPtrT = std::unique_ptr<T, DownstreamDeleter>;

    typedef DownstreamPtrT<IDownstream> DownstreamPtr;

    HERMESPROTOCOL_API DownstreamPtr CreateHermesDownstream(uint32_t laneId, IDownstreamCallback& callback);
#else
    class Downstream : public IDownstream
    {
    public:
        explicit Downstream(unsigned laneId, IDownstreamCallback&);
        Downstream(const Downstream&) = delete;
        Downstream& operator=(const Downstream&) = delete;
        ~Downstream() { Delete(); }

        void Run() override;
        void Enable(const DownstreamSettings&) override;

        void Signal(unsigned sessionId, const ServiceDescriptionData&) override;
        void Signal(unsigned sessionId, const BoardAvailableData&) override;
        void Signal(unsigned sessionId, const RevokeBoardAvailableData&) override;
        void Signal(unsigned sessionId, const TransportFinishedData&) override;
        void Signal(unsigned sessionId, const BoardForecastData&) override;
        void Signal(unsigned sessionId, const SendBoardInfoData&) override;
        void Signal(unsigned sessionId, const NotificationData&) override;
        void Signal(unsigned sessionId, const CheckAliveData&) override;
        void Signal(unsigned sessionId, const CommandData&) override;
        void Reset(const NotificationData&) override;

        // raw XML for testing
        void Signal(unsigned sessionId, StringView rawXml) override;
        void Reset(StringView rawXml) override;

        void Disable(const NotificationData&) override;
        void Stop() override;
        void Delete() override { ::DeleteHermesDownstream(m_pImpl); m_pImpl = nullptr; }

        void Post(std::function<void()>&&) override;

    private:
        HermesDownstream* m_pImpl = nullptr;
    };

    typedef std::unique_ptr<Downstream> DownstreamPtr;

    inline DownstreamPtr CreateHermesDownstream(uint32_t laneId, IDownstreamCallback& callback) {
        return std::make_unique<Downstream>(laneId, callback);
    }


#endif
}