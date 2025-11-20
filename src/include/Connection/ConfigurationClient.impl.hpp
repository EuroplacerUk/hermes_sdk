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

#include "ConfigurationClient.hpp"

namespace Hermes
{
    //================= Configuration client implementation =======================
    inline std::pair<CurrentConfigurationData, Error> GetConfiguration(StringView hostName, unsigned timeoutInSeconds,
        ITraceCallback* pTraceCallback)
    {
        HermesGetConfigurationCallbacks callbacks{};

        callbacks.m_traceCallback.m_pData = pTraceCallback;
        callbacks.m_traceCallback.m_pCall = [](void* pTraceCallback, unsigned sessionId, EHermesTraceType type,
            HermesStringView trace)
            {
                if (!pTraceCallback)
                    return;
                static_cast<ITraceCallback*>(pTraceCallback)->OnTrace(sessionId, ToCpp(type), ToCpp(trace));
            };

        CurrentConfigurationData configuration;
        callbacks.m_currentConfigurationCallback.m_pData = &configuration;
        callbacks.m_currentConfigurationCallback.m_pCall = [](void* pData, uint32_t /*sessionId*/,
            const HermesCurrentConfigurationData* pConfiguration)
            {
                *static_cast<CurrentConfigurationData*>(pData) = ToCpp(*pConfiguration);
            };

        Error error;
        callbacks.m_errorCallback.m_pData = &error;
        callbacks.m_errorCallback.m_pCall = [](void* pData, const HermesError* pError)
            {
                if (!pData)
                    return;
                *static_cast<Error*>(pData) = ToCpp(*pError);
            };

        ::GetHermesConfiguration(ToC(hostName), timeoutInSeconds, &callbacks);
        return{ configuration, error };
    }

    inline Error Hermes::SetConfiguration(StringView hostName, const SetConfigurationData& configuration,
        unsigned timeoutInSeconds,
        Hermes::CurrentConfigurationData* out_pConfiguration, // resulting configuration
        std::vector<NotificationData>* out_pNotifications, // out: notification data
        ITraceCallback* pTraceCallback)
    {
        // reset the output by default
        if (out_pConfiguration)
        {
            *out_pConfiguration = Hermes::CurrentConfigurationData();
        }
        if (out_pNotifications)
        {
            out_pNotifications->clear();
        }

        HermesSetConfigurationCallbacks callbacks{};

        callbacks.m_traceCallback.m_pData = pTraceCallback;
        callbacks.m_traceCallback.m_pCall = [](void* pTraceCallback, unsigned sessionId, EHermesTraceType type,
            HermesStringView trace)
            {
                if (!pTraceCallback)
                    return;
                static_cast<ITraceCallback*>(pTraceCallback)->OnTrace(sessionId, ToCpp(type), ToCpp(trace));
            };

        callbacks.m_notificationCallback.m_pData = out_pNotifications;
        callbacks.m_notificationCallback.m_pCall = [](void* pData, uint32_t /*sessionId*/, const HermesNotificationData* pNotification)
            {
                if (!pData)
                    return;
                static_cast<std::vector<NotificationData>*>(pData)->emplace_back(ToCpp(*pNotification));
            };

        Error error;
        callbacks.m_errorCallback.m_pData = &error;
        callbacks.m_errorCallback.m_pCall = [](void* pData, const HermesError* pError)
            {
                *static_cast<Error*>(pData) = ToCpp(*pError);
            };

        callbacks.m_currentConfigurationCallback.m_pData = out_pConfiguration;
        callbacks.m_currentConfigurationCallback.m_pCall = [](void* pData, uint32_t /*sessionId*/,
            const HermesCurrentConfigurationData* pConfiguration)
            {
                if (!pData)
                    return;
                *static_cast<CurrentConfigurationData*>(pData) = ToCpp(*pConfiguration);
            };

        Converter2C<SetConfigurationData> converter(configuration);
        ::SetHermesConfiguration(ToC(hostName), converter.CPointer(), timeoutInSeconds, &callbacks);
        return error;
    }

}