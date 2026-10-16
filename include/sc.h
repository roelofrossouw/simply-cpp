#pragma once

// All of sc-core but config.h and params.h, which bring in nlohmann/json, plus the common standard
// headers.
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

#include <core.h>
#include <utf8.h>
#include <date.h>
#include <datetime.h>
#include <base64.h>
#include <rest.h>
#include <color.h>
#include <timer.h>
#include <matrix.h>
#include <geometry.h>
#include <ollama.h>
#include <dual.h>
#include <ip_address.h>
#include <ip_endpoint.h>
#include <ip_endpoints.h>
#include <console.h>
#include <percent.h>
