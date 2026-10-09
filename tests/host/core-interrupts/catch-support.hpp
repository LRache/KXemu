#pragma once

#include <catch2/catch_test_macros.hpp>

#include <future>
#include <string>

// REQUIRE with a note that is reported only for this assertion.
#define REQUIRE_MSG(cond, msg) \
    do {                       \
        INFO(msg);             \
        REQUIRE(cond);         \
    } while (false)

template <>
struct Catch::StringMaker<std::future_status> {
    static std::string convert(std::future_status status) {
        switch (status) {
            case std::future_status::ready: return "ready";
            case std::future_status::timeout: return "timeout";
            case std::future_status::deferred: return "deferred";
        }
        return "unknown";
    }
};
