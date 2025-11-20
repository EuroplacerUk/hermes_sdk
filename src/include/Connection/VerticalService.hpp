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
    struct IVerticalServiceCallback
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

        virtual void OnTrace(unsigned sessionId, ETraceType, StringView trace) = 0;

    protected:
        virtual ~IVerticalServiceCallback() = default;
    };

    //======================= VerticalService interface =====================================  
    class VerticalService
    {
    public:
        explicit VerticalService(IVerticalServiceCallback&);
        VerticalService(const VerticalService&) = delete;
        VerticalService& operator=(const VerticalService&) = delete;
        ~VerticalService() { ::DeleteHermesVerticalService(m_pImpl); }

        void Run();
        template<class F> void Post(F&&);
        void Enable(const VerticalServiceSettings&);

        void Signal(unsigned sessionId, const SupervisoryServiceDescriptionData&);
        void Signal(unsigned sessionId, const BoardArrivedData&); // only to a specific client
        void Signal(const BoardArrivedData&); // to all clients that have specified FeatureBoardTracking
        void Signal(unsigned sessionId, const BoardDepartedData&); // only to a specific client
        void Signal(const BoardDepartedData&); // to all clients that have specified FeatureBoardTracking
        void Signal(unsigned sessionId, const QueryWorkOrderInfoData&);
        void Signal(unsigned sessionId, const ReplyWorkOrderInfoData&);
        void Signal(unsigned sessionId, const SendHermesCapabilitiesData&);
        void Signal(unsigned sessionId, const CurrentConfigurationData&);
        void Signal(unsigned sessionId, const NotificationData&);
        void Signal(unsigned sessionId, const CheckAliveData&);
        void Signal(unsigned sessionId, const CommandData&);
        void ResetSession(unsigned sessionId, const NotificationData&);

        void Disable(const NotificationData&);
        void Stop();

    private:
        HermesVerticalService* m_pImpl = nullptr;
    };


}