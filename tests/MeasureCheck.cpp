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

TEST_CASE ("harn_01_tone_and_alias_measurements_read_known_signals")
{
    const auto rate = 48000.0;
    const auto tone = [rate] (size_t i, double hz) { return std::sin (2.0 * std::numbers::pi * hz * static_cast<double> (i) / rate); };

    std::vector<float> harmonics (24000, 0.0f), withAlias (24000, 0.0f);
    for (size_t i = 0; i < harmonics.size(); ++i)
    {
        harmonics[i] = static_cast<float> (0.5 * tone (i, 4186.0) + 0.25 * tone (i, 8372.0));
        withAlias[i] = harmonics[i] + static_cast<float> (0.005 * tone (i, 5000.0));
    }

    CHECK (measure::toneLevel (harmonics, rate, 4186.0, 0.0, 0.5) == Catch::Approx (0.5).epsilon (0.01));
    CHECK (measure::toneLevel (harmonics, rate, 8372.0, 0.0, 0.5) == Catch::Approx (0.25).epsilon (0.01));
    CHECK (measure::toneLevel (harmonics, rate, 5000.0, 0.0, 0.5) < 1.0e-4);
    CHECK (measure::inharmonicDb (harmonics, rate, 4186.0) < -85.0);

    // 0.005 against 0.5 and 0.25: 10 log (0.005^2 / (0.5^2 + 0.25^2)) = -41 dB.
    CHECK (measure::inharmonicDb (withAlias, rate, 4186.0) == Catch::Approx (-41.0).margin (1.0));

    CHECK (measure::peakBetween (harmonics, rate, 0.0, 0.5) > 0.5f);
    std::vector<float> ramp { 0.0f, 0.1f, 0.2f, 0.0f, 0.9f, 0.0f, 0.0f };
    CHECK (measure::peakBetween (ramp, 1.0, 1.0, 3.0) == Catch::Approx (0.2f));
    CHECK (measure::largestStepBetween (ramp, 1.0, 0.0, 3.0) == Catch::Approx (0.1f));
    CHECK (measure::lastSound (ramp) == 4);
    CHECK (measure::lastSound (std::vector<float> (4, 0.0f)) == -1);
}
