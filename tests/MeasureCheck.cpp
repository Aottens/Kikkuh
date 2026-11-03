#include "Measure.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <numbers>

// A broken ruler passes every test: check the measurements themselves on known signals.
TEST_CASE ("harn_01_measurements_read_known_signals")
{
    const auto rate = 48000.0;
    std::vector<float> sine (24000, 0.0f);
    for (size_t i = 0; i < sine.size(); ++i)
        sine[i] = 0.5f * static_cast<float> (std::sin (2.0 * std::numbers::pi * 55.0 * static_cast<double> (i) / rate));

    CHECK (measure::peak (sine) == Catch::Approx (0.5f).epsilon (0.001));
    CHECK (measure::frequency (sine, rate, 0.1, 0.4) == Catch::Approx (55.0).epsilon (0.001));
    CHECK (measure::largestStep (sine) < 0.01f);

    sine[1000] += 0.5f;
    CHECK (measure::largestStep (sine) > 0.4f);

    std::vector<float> delayed (100, 0.0f);
    delayed[42] = 1.0f;
    CHECK (measure::firstSound (delayed) == 42);
    CHECK (measure::firstSound (std::vector<float> (10, 0.0f)) == -1);
}
