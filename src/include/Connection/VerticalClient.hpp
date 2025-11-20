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
    struct IVerticalClientCallback
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

        virtual void OnTrace(unsigned sessionId, ETraceType, StringView trace) = 0;

    protected:
        virtual ~IVerticalClientCallback() = default;
    };

    //======================= VerticalClient interface =====================================
    class VerticalClient
    {
    public:
        explicit VerticalClient(IVerticalClientCallback&);
        VerticalClient(const VerticalClient&) = delete;
        VerticalClient& operator=(const VerticalClient&) = delete;
        ~VerticalClient() { ::DeleteHermesVerticalClient(m_pImpl); }

        void Run();
        template<class F> void Post(F&&);
        void Enable(const VerticalClientSettings&);

        void Signal(unsigned sessionId, const SupervisoryServiceDescriptionData&);
        void Signal(unsigned sessionId, const GetConfigurationData&);
        void Signal(unsigned sessionId, const SetConfigurationData&);
        void Signal(unsigned sessionId, const QueryHermesCapabilitiesData&);
        void Signal(unsigned sessionId, const SendWorkOrderInfoData&);
        void Signal(unsigned sessionId, const NotificationData&);
        void Signal(unsigned sessionId, const CheckAliveData&);
        void Reset(const NotificationData&);

        // raw XML for testing
        void Signal(unsigned sessionId, StringView rawXml);
        void Reset(StringView rawXml);

        void Disable(const NotificationData&);
        void Stop();

    private:
        HermesVerticalClient* m_pImpl = nullptr;
    };

    
}