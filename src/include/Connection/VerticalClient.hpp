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
    struct IVerticalClientCallback : ITraceCallback
    {
        IVerticalClientCallback() = default;

        // callbacks must not be moved or copied!
        IVerticalClientCallback(const IVerticalClientCallback&) = delete;
        IVerticalClientCallback& operator=(const IVerticalClientCallback&) = delete;

        virtual void OnConnected(unsigned sessionId, EVerticalState, const ConnectionInfo&) = 0;
        virtual void On(unsigned sessionId, EVerticalState, const SupervisoryServiceDescriptionData&) = 0;
        virtual void On(unsigned /*sessionId*/, const BoardArrivedData&) {}; // not necessarily interesting, hence not abstract
        virtual void On(unsigned /*sessionId*/, const BoardDepartedData&) {} // not necessarily interesting, hence not abstract
        virtual void On(unsigned sessionId, const QueryWorkOrderInfoData&) = 0;
        virtual void On(unsigned sessionId, const ReplyWorkOrderInfoData&) = 0;
        virtual void On(unsigned /*sessionId*/, const CurrentConfigurationData&) {} // not necessarily interesting, hence not abstract
        virtual void On(unsigned /*sessionId*/, const SendHermesCapabilitiesData&) {} // not necessarily interesting, hence not abstract
        virtual void On(unsigned sessionId, const NotificationData&) = 0;
        virtual void On(unsigned /*sessionId*/, const CheckAliveData&) {} // not necessarily interesting, hence not abstract
        virtual void OnDisconnected(unsigned sessionId, EVerticalState, const Error&) = 0;

    protected:
        virtual ~IVerticalClientCallback() = default;
    };

    //======================= VerticalClient interface =====================================
    struct IVerticalClient
    {
        virtual void Run() = 0;
        virtual void Post(std::function<void()>&&) = 0;
        template<class F> void Post(F&& fn) { Post(std::function<void()>{fn});}
        virtual void Enable(const VerticalClientSettings&) = 0;

        virtual void Signal(unsigned sessionId, const SupervisoryServiceDescriptionData&) = 0;
        virtual void Signal(unsigned sessionId, const GetConfigurationData&) = 0;
        virtual void Signal(unsigned sessionId, const SetConfigurationData&) = 0;
        virtual void Signal(unsigned sessionId, const QueryHermesCapabilitiesData&) = 0;
        virtual void Signal(unsigned sessionId, const SendWorkOrderInfoData&) = 0;
        virtual void Signal(unsigned sessionId, const NotificationData&) = 0;
        virtual void Signal(unsigned sessionId, const CheckAliveData&) = 0;
        virtual void Reset(const NotificationData&) = 0;

        // raw XML for testing
        virtual void Signal(unsigned sessionId, StringView rawXml) = 0;
        virtual void Reset(StringView rawXml) = 0;

        virtual void Disable(const NotificationData&) = 0;
        virtual void Stop() = 0;
        virtual void Delete() = 0;
    };


#ifdef HERMES_CPP_ABI
    struct VerticalClientDeleter {
        void operator()(IVerticalClient* ptr) { ptr->Delete(); }
    };

    template<typename T>
    using VerticalClientPtrT = std::unique_ptr<T, VerticalClientDeleter>;

    typedef VerticalClientPtrT<IVerticalClient> VerticalClientPtr;

    HERMESPROTOCOL_API VerticalClientPtr CreateHermesVerticalClient(IVerticalClientCallback& callbacks);
#else
    class VerticalClient : public IVerticalClient
    {
    public:
        explicit VerticalClient(IVerticalClientCallback&);
        VerticalClient(const VerticalClient&) = delete;
        VerticalClient& operator=(const VerticalClient&) = delete;
        ~VerticalClient() { ::DeleteHermesVerticalClient(m_pImpl); }

        void Run() override;
        template<class F> void Post(F&&);
        void Enable(const VerticalClientSettings&) override;

        void Signal(unsigned sessionId, const SupervisoryServiceDescriptionData&) override;
        void Signal(unsigned sessionId, const GetConfigurationData&) override;
        void Signal(unsigned sessionId, const SetConfigurationData&) override;
        void Signal(unsigned sessionId, const QueryHermesCapabilitiesData&) override;
        void Signal(unsigned sessionId, const SendWorkOrderInfoData&) override;
        void Signal(unsigned sessionId, const NotificationData&) override;
        void Signal(unsigned sessionId, const CheckAliveData&) override;
        void Reset(const NotificationData&) override;

        // raw XML for testing
        void Signal(unsigned sessionId, StringView rawXml) override;
        void Reset(StringView rawXml) override;

        void Disable(const NotificationData&) override;
        void Stop() override;

        void Delete() override { ::DeleteHermesVerticalClient(m_pImpl); m_pImpl = nullptr; }
    private:
        HermesVerticalClient* m_pImpl = nullptr;
    };

    typedef std::unique_ptr<VerticalClient> VerticalClientPtr;

    inline VerticalClientPtr CreateHermesVerticalClient(IVerticalClientCallback& callback) {
        return std::make_unique<VerticalClient>(callback);
    }
#endif
    
}