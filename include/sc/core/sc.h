#pragma once

// All of sc-core but config.h and params.h, which bring in nlohmann/json, plus the common standard
// headers.
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

#include <sc/core/core.h>
#include <sc/core/utf8.h>
#include <sc/core/date.h>
#include <sc/core/datetime.h>
#include <sc/core/base64.h>
#include <sc/core/color.h>
#include <sc/core/timer.h>
#include <sc/core/matrix.h>
#include <sc/core/geometry.h>
#include <sc/core/transform.h>
#include <sc/core/dual.h>
#include <sc/core/ip_address.h>
#include <sc/core/ip_endpoint.h>
#include <sc/core/ip_endpoints.h>
#include <sc/core/console.h>
#include <sc/core/percent.h>
#include <sc/core/distance.h>
