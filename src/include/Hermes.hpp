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
#pragma once

#include "HermesDataConversion.hpp"

#include <functional>
#include <memory>

// These are lightweight C++ wrappers around the Hermes C Api: definitions
#include "Connection/Downstream.hpp"
#include "Connection/Upstream.hpp"
#include "Connection/ConfigurationService.hpp"
#include "Connection/ConfigurationClient.hpp"
#include "Connection/VerticalService.hpp"
#include "Connection/VerticalClient.hpp"

// These are lightweight C++ wrappers around the Hermes C Api: implementations
#include "Connection/Downstream.impl.hpp"
#include "Connection/Upstream.impl.hpp"
#include "Connection/ConfigurationService.impl.hpp"
#include "Connection/ConfigurationClient.impl.hpp"
#include "Connection/VerticalService.impl.hpp"
#include "Connection/VerticalClient.impl.hpp"



