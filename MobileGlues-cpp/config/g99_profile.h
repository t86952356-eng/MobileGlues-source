// Experimental G99 profile. LGPL-2.1-only; see the repository LICENSE.
#ifndef MOBILEGLUES_G99_PROFILE_H
#define MOBILEGLUES_G99_PROFILE_H
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>

namespace g99_profile {
inline std::string normalized(const std::string& value) {
    std::string result;
    for (unsigned char c : value) {
        if (std::isalnum(c)) result += static_cast<char>(std::tolower(c));
    }
    return result;
}
inline bool soc_matches(const std::string& raw) {
    // MT6789 is a FAMILY identifier, not proof of the retail G99 name; G99
    // derivatives may share it. Require the Mali-G57 GPU independently below.
    const auto value = normalized(raw);
    return (value.rfind("mt6789", 0) == 0 &&
               (value.size() == 6 || !std::isdigit(static_cast<unsigned char>(value[6])))) ||
           value == "heliog99" || value == "mediatekheliog99" || value == "g99";
}
inline bool eligible(const std::string& gpu, const std::string& soc,
                     const std::string& board, const std::string& hardware) {
    const auto renderer = normalized(gpu);
    const bool mali_g57 = renderer.rfind("malig57", 0) == 0 &&
        (renderer.size() == 7 || !std::isdigit(static_cast<unsigned char>(renderer[7])));
    return mali_g57 && (soc_matches(soc) || soc_matches(board) || soc_matches(hardware));
}
inline int cache_mib(int configured) { return std::clamp(configured, 64, 128); }
inline int cache_batch(bool active) { return active ? 64 : 16; }
inline bool cache_flush_due(bool active, int pending, int64_t elapsed_ns) {
    return pending > 0 && (pending >= cache_batch(active) || elapsed_ns >= 5000000000LL);
}
} // namespace g99_profile
#endif
