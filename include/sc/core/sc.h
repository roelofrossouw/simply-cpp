#pragma once

// A convenience for programs, tests and examples: all of sc-core but config.h and params.h (which
// bring in nlohmann/json), plus the common standard headers. Library code (a module's public headers
// and sources) never includes it: it includes the headers it uses, so a consumer doesn't get all of
// sc-core through one include.
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
