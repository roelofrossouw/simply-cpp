#pragma once

#include <algorithm>

namespace sc {
    /// How much a module writes to stderr, as syslog's levels: a message is written when its level
    /// is at or below the one set. A library's warnings (malformed input it skips, say) are
    /// warning; set error to keep only errors.
    ///
    ///     sc::image::log_level(sc::log_level::error);   // no "Corrupt JPEG data" warnings
    enum class log_level : int { emergency, alert, critical, error, warning, notice, info, debug };

    /// A syslog-style number (0 to 7, as a configuration file has it) as a log_level; numbers
    /// outside that range are taken as the nearest one.
    constexpr log_level log_level_from(const int level) {
        return static_cast<log_level>(std::clamp(level, 0, 7));
    }
}
