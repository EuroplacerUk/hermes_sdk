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
    //======================= ConfigurationClient interface =====================================
    struct ITraceCallback
    {
        virtual void OnTrace(unsigned sessionId, ETraceType, StringView trace) = 0;

    protected:
        ~ITraceCallback() {}
    };

    std::pair<CurrentConfigurationData, Error> GetConfiguration(StringView hostName, // host (e.g. ip address) to connect with
        unsigned timeoutInSeconds, // timeout to connect
        ITraceCallback* // optional
    );

    Error SetConfiguration(StringView hostName, // host (e.g. ip address) to connect with
        const Hermes::SetConfigurationData&, // configuration to set
        unsigned timeoutInSeconds, // timeout to connect
        Hermes::CurrentConfigurationData*, // optional out: resulting configuration
        std::vector<NotificationData>*, // optional out: notification data
        ITraceCallback* // optional
    );

}