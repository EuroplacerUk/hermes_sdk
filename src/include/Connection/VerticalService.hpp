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
    struct IVerticalServiceCallback : ITraceCallback
    {
        IVerticalServiceCallback() = default;

        // callbacks must not be moved or copied!
        IVerticalServiceCallback(const IVerticalServiceCallback&) = delete;
        IVerticalServiceCallback& operator=(const IVerticalServiceCallback&) = delete;

        virtual void OnConnected(unsigned sessionId, EVerticalState, const ConnectionInfo&) = 0;
        virtual void On(unsigned sessionId, EVerticalState, const SupervisoryServiceDescriptionData&) = 0;
        virtual void On(unsigned sessionId, const GetConfigurationData&, const ConnectionInfo&) = 0;
        virtual void On(unsigned sessionId, const SetConfigurationData&, const ConnectionInfo&) = 0;
        virtual void On(unsigned /*sessionId*/, const SendWorkOrderInfoData&) {}; // not necessarily interesting, hence not abstract
        virtual void On(unsigned sessionId, const NotificationData&) = 0;
        virtual void On(unsigned sessionId, const QueryHermesCapabilitiesData&) = 0;
        virtual void On(unsigned /*sessionId*/, const CheckAliveData&) {} // not necessarily interesting, hence not abstract
        virtual void OnDisconnected(unsigned sessionId, EVerticalState, const Error&) = 0;

    protected:
        virtual ~IVerticalServiceCallback() = default;
    };

    //======================= VerticalService interface =====================================  
    struct IVerticalService
    {
        virtual void Run() = 0;

        virtual void Post(std::function<void()>&&) = 0;
        template<class F> void Post(F&& f) { Post(std::function<void()>{std::move(f)}); }
        virtual void Enable(const VerticalServiceSettings&) = 0;

        virtual void Signal(unsigned sessionId, const SupervisoryServiceDescriptionData&) = 0;
        virtual void Signal(unsigned sessionId, const BoardArrivedData&) = 0; // only to a specific client
        virtual void Signal(const BoardArrivedData&) = 0; // to all clients that have specified FeatureBoardTracking
        virtual void Signal(unsigned sessionId, const BoardDepartedData&) = 0; // only to a specific client
        virtual void Signal(const BoardDepartedData&) = 0; // to all clients that have specified FeatureBoardTracking
        virtual void Signal(unsigned sessionId, const QueryWorkOrderInfoData&) = 0;
        virtual void Signal(unsigned sessionId, const ReplyWorkOrderInfoData&) = 0;
        virtual void Signal(unsigned sessionId, const SendHermesCapabilitiesData&) = 0;
        virtual void Signal(unsigned sessionId, const CurrentConfigurationData&) = 0;
        virtual void Signal(unsigned sessionId, const NotificationData&) = 0;
        virtual void Signal(unsigned sessionId, const CheckAliveData&) = 0;
        virtual void ResetSession(unsigned sessionId, const NotificationData&) = 0;

        virtual void Disable(const NotificationData&) = 0;
        virtual void Stop() = 0;

        virtual void Delete() = 0;
    };

#ifdef HERMES_CPP_ABI
    struct VerticalServiceDeleter {
        void operator()(IVerticalService* ptr) { ptr->Delete(); }
    };

    template<typename T>
    using VerticalServicePtrT = std::unique_ptr<T, VerticalServiceDeleter>;

    typedef VerticalServicePtrT<IVerticalService> VerticalServicePtr;

    HERMESPROTOCOL_API VerticalServicePtr CreateHermesVerticalService(IVerticalServiceCallback& callbacks);
#else
    class VerticalService : public IVerticalService
    {
    public:
        explicit VerticalService(IVerticalServiceCallback&);
        VerticalService(const VerticalService&) = delete;
        VerticalService& operator=(const VerticalService&) = delete;
        ~VerticalService() { ::DeleteHermesVerticalService(m_pImpl); }

        void Run() override;
        void Post(std::function<void()>&&) override;
        void Enable(const VerticalServiceSettings&) override;

        void Signal(unsigned sessionId, const SupervisoryServiceDescriptionData&) override;
        void Signal(unsigned sessionId, const BoardArrivedData&) override; // only to a specific client
        void Signal(const BoardArrivedData&) override; // to all clients that have specified FeatureBoardTracking
        void Signal(unsigned sessionId, const BoardDepartedData&) override; // only to a specific client
        void Signal(const BoardDepartedData&) override; // to all clients that have specified FeatureBoardTracking
        void Signal(unsigned sessionId, const QueryWorkOrderInfoData&) override;
        void Signal(unsigned sessionId, const ReplyWorkOrderInfoData&) override;
        void Signal(unsigned sessionId, const SendHermesCapabilitiesData&) override;
        void Signal(unsigned sessionId, const CurrentConfigurationData&) override;
        void Signal(unsigned sessionId, const NotificationData&) override;
        void Signal(unsigned sessionId, const CheckAliveData&) override;
        void ResetSession(unsigned sessionId, const NotificationData&) override;

        void Disable(const NotificationData&) override;
        void Stop() override;

        void Delete() override { ::DeleteHermesVerticalService(m_pImpl); m_pImpl = nullptr; }
    private:
        HermesVerticalService* m_pImpl = nullptr;
    };

    typedef std::unique_ptr<VerticalService> VerticalServicePtr;

    inline VerticalServicePtr CreateHermesVerticalService(IVerticalServiceCallback& callback) {
        return std::make_unique<VerticalService>(callback);
    }

#endif
}