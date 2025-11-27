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
    struct IConfigurationServiceCallback : ITraceCallback
    {
        IConfigurationServiceCallback() = default;

        // callbacks must not be moved or copied!
        IConfigurationServiceCallback(const IConfigurationServiceCallback&) = delete;
        IConfigurationServiceCallback& operator=(const IConfigurationServiceCallback&) = delete;

        virtual void OnConnected(unsigned sessionId, const ConnectionInfo&) = 0;
        virtual Error OnSetConfiguration(unsigned sessionId, const ConnectionInfo&, const SetConfigurationData&) = 0;
        virtual CurrentConfigurationData OnGetConfiguration(unsigned sessionId, const ConnectionInfo&) = 0;
        virtual void OnDisconnected(unsigned sessionId, const Error&) = 0;

    protected:
        virtual ~IConfigurationServiceCallback() = default;
    };

    struct IConfigurationService
    {
        virtual void Run() = 0;
        virtual void Post(std::function<void()>&&) = 0;
        template<class F> void Post(F&& fn) { Post(std::function<void()>{fn}); }
        virtual void Enable(const ConfigurationServiceSettings&) = 0;
        virtual void Disable(const NotificationData&) = 0;
        virtual void Stop() = 0;
        virtual void Delete() = 0;
    };

    //======================= ConfigurationService interface =====================================
    class ConfigurationService : IConfigurationService
    {
    public:
        explicit ConfigurationService(IConfigurationServiceCallback& callback);
        ConfigurationService(const ConfigurationService&) = delete;
        ConfigurationService& operator=(const ConfigurationService&) = delete;
        ~ConfigurationService() { ::DeleteHermesConfigurationService(m_pImpl); }

        void Run() override;
        void Post(std::function<void()>&&) override;
        void Enable(const ConfigurationServiceSettings&) override;
        void Disable(const NotificationData&) override;
        void Stop() override;
        void Delete() override { ::DeleteHermesConfigurationService(m_pImpl); m_pImpl = nullptr; }
    private:
        HermesConfigurationService* m_pImpl = nullptr;
        IConfigurationServiceCallback& m_callback;
    };
}