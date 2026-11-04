#include "Measure.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <numbers>
#include <random>

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

// HARN-05: a kick made from BODY's own curve (specs/modules/body.md), so the expected numbers come
// from the spec and not from the DSP.
namespace
{
constexpr auto checkRate = 48000.0;

double bodyCurve (double t, double decay, double shape)
{
    return t >= decay ? 0.0 : std::pow (1.0 - t / decay, std::pow (3.0, 1.0 + shape / 100.0));
}

struct KnownKick
{
    double tuneHz = 55.0, amount = 36.0, pitchDecay = 0.1, pitchShape = 0.0, ampDecay = 2.0, ampShape = -100.0, second = 0.3, third = 0.1;

    double hz (double t) const { return tuneHz * std::pow (2.0, amount * bodyCurve (t, pitchDecay, pitchShape) / 12.0); }

    std::vector<float> render (double seconds) const
    {
        std::vector<float> x (static_cast<size_t> (seconds * checkRate));
        double phase = 0.0;
        for (size_t i = 0; i < x.size(); ++i)
        {
            const auto t = static_cast<double> (i) / checkRate;
            x[i] = static_cast<float> (bodyCurve (t, ampDecay, ampShape) * (std::sin (phase) + second * std::sin (2.0 * phase) + third * std::sin (3.0 * phase)));
            phase += 2.0 * std::numbers::pi * hz (t) / checkRate;
        }
        return x;
    }
};

double cents (double a, double b)
{
    return std::abs (1200.0 * std::log2 (a / b));
}

double shapeExponent (double shape)
{
    return std::pow (3.0, 1.0 + shape / 100.0);
}
}

TEST_CASE ("harn_05a_time_zero_is_the_first_sample_above_minus_60_db")
{
    std::vector<float> x (400, 0.0f);
    x[100] = 0.0005f; // below -60 dB of the peak
    x[150] = 0.002f;
    x[200] = 1.0f;
    CHECK (measure::onset (x) == 150);
    CHECK (measure::onset (std::vector<float> (10, 0.0f)) == -1);
}

TEST_CASE ("harn_05b_pitch_follows_a_known_sweep_with_harmonics")
{
    const auto check = [] (KnownKick kick, std::vector<double> times)
    {
        const auto x = kick.render (1.0);
        for (auto t : times)
        {
            INFO ("at " << t << " s, expected " << kick.hz (t) << " Hz");
            CHECK (cents (measure::pitchAt (x, checkRate, t), kick.hz (t)) < 50.0);
        }
    };

    check (KnownKick {}, { 0.005, 0.01, 0.02, 0.03, 0.05, 0.075, 0.1, 0.15, 0.2, 0.3, 0.5 });
    check (KnownKick { .tuneHz = 45.0, .amount = 60.0, .pitchDecay = 0.04 }, { 0.005, 0.01, 0.02, 0.03, 0.05, 0.1, 0.3 });
    check (KnownKick { .tuneHz = 65.4, .amount = 24.0, .pitchDecay = 0.3, .pitchShape = 40.0 }, { 0.005, 0.02, 0.05, 0.1, 0.2, 0.5 });
}

TEST_CASE ("harn_05b_pitch_on_a_steady_tail_is_within_half_a_percent")
{
    const auto x = KnownKick {}.render (1.0);
    CHECK (measure::pitchAt (x, checkRate, 0.5) == Catch::Approx (55.0).epsilon (0.005));
    CHECK (measure::tailHz (x, checkRate) == Catch::Approx (55.0).epsilon (0.005));
}

TEST_CASE ("harn_05b_no_pitch_below_minus_40_db")
{
    const auto x = KnownKick { .ampDecay = 0.2, .ampShape = 0.0 }.render (1.0);
    CHECK (measure::pitchAt (x, checkRate, 0.02) > 0.0);
    CHECK_FALSE (measure::pitchAt (x, checkRate, 0.19) > 0.0); // (0.05)^3 is -78 dB
    CHECK_FALSE (measure::pitchAt (x, checkRate, 0.5) > 0.0);
}

TEST_CASE ("harn_05c_harmonics_of_a_known_tone")
{
    const std::vector<double> levels { 1.0, 0.5, 0.25, 0.1, 0.05, 0.02, 0.01, 0.005 };
    std::vector<float> x (48000);
    for (size_t i = 0; i < x.size(); ++i)
        for (size_t h = 0; h < levels.size(); ++h)
            x[i] += static_cast<float> (levels[h] * std::sin (2.0 * std::numbers::pi * 55.0 * static_cast<double> (h + 1) * static_cast<double> (i) / checkRate));

    const auto db = measure::harmonicsDb (x, checkRate, 55.0, 0.0, 1.0);
    REQUIRE (db.size() == 7);
    for (size_t h = 0; h < db.size(); ++h)
    {
        INFO ("harmonic " << h + 2);
        CHECK (db[h] == Catch::Approx (20.0 * std::log10 (levels[h + 1])).margin (1.0));
    }
}

TEST_CASE ("harn_05c_a_tail_shorter_than_8_periods_gives_no_harmonics")
{
    const auto x = KnownKick {}.render (1.0);
    CHECK (measure::harmonicsDb (x, checkRate, 55.0, 0.5, 0.6).empty()); // 5.5 periods
    CHECK (measure::harmonicsDb (x, checkRate, 55.0, 0.5, 0.65).size() == 7); // 8.25 periods
}

TEST_CASE ("harn_05c_tail_stretch_runs_from_within_a_semitone_to_minus_40_db")
{
    // Amount 36 at k = 3: 36 g = 1 at t = D (1 - (1/36)^(1/3)) = 0.0697 s. Amp at k = 3, D = 1 s:
    // g = 0.01 at t = 1 - 0.01^(1/3) = 0.785 s.
    const auto x = KnownKick { .ampDecay = 1.0, .ampShape = 0.0, .second = 0.0, .third = 0.0 }.render (1.2);
    const auto stretch = measure::tailStretch (x, checkRate);
    CHECK (stretch.from == Catch::Approx (0.0697).margin (0.015));
    CHECK (stretch.to == Catch::Approx (0.785).margin (0.02));
}

TEST_CASE ("harn_05d_peak_time")
{
    std::vector<float> x (480, 0.1f);
    x[144] = -0.9f;
    CHECK (measure::peakSeconds (x, checkRate) == Catch::Approx (0.003).margin (1.0 / checkRate));
}

TEST_CASE ("harn_05d_band_shares_place_known_tones")
{
    const auto burst = [] (double hz, double from, double to)
    {
        std::vector<float> x (static_cast<size_t> (0.3 * checkRate), 0.0f);
        for (auto i = static_cast<size_t> (from * checkRate); i < static_cast<size_t> (to * checkRate); ++i)
            x[i] = static_cast<float> (std::sin (2.0 * std::numbers::pi * hz * static_cast<double> (i) / checkRate));
        return x;
    };

    CHECK (measure::bandShares (burst (2000.0, 0.0, 0.005), checkRate, 0.0, 0.005)[2] > 90.0);
    CHECK (measure::bandShares (burst (600.0, 0.02, 0.05), checkRate, 0.02, 0.05)[1] > 90.0);
    CHECK (measure::bandShares (burst (100.0, 0.05, 0.2), checkRate, 0.05, 0.2)[0] > 90.0);
    CHECK (measure::bandShares (burst (8000.0, 0.005, 0.02), checkRate, 0.005, 0.02)[3] > 90.0);

    const auto shares = measure::bandShares (burst (2000.0, 0.0, 0.005), checkRate, 0.0, 0.005);
    CHECK (shares[0] + shares[1] + shares[2] + shares[3] == Catch::Approx (100.0).margin (0.01));
}

TEST_CASE ("harn_05e_time_below_a_level_for_good")
{
    // A straight line from 1 to 0 over 1 s: 10^(dB / 20) of the level is left at t = 1 - that.
    const auto x = KnownKick { .tuneHz = 1000.0, .amount = 0.0, .ampDecay = 1.0, .ampShape = -100.0, .second = 0.0, .third = 0.0 }.render (1.2);
    CHECK (measure::secondsBelowDb (x, checkRate, -6.0) == Catch::Approx (0.499).margin (0.005));
    CHECK (measure::secondsBelowDb (x, checkRate, -20.0) == Catch::Approx (0.9).margin (0.005));
    CHECK (measure::secondsBelowDb (x, checkRate, -40.0) == Catch::Approx (0.99).margin (0.005));
    CHECK (measure::secondsBelowDb (x, checkRate, -60.0) == Catch::Approx (0.999).margin (0.005));
}

TEST_CASE ("harn_05e_pitch_fit_gives_known_values_back")
{
    for (const auto& kick : { KnownKick { .tuneHz = 50.0, .amount = 36.0, .pitchDecay = 0.1, .pitchShape = 0.0 },
                              KnownKick { .tuneHz = 55.0, .amount = 48.0, .pitchDecay = 0.15, .pitchShape = 30.0 },
                              KnownKick { .tuneHz = 45.0, .amount = 24.0, .pitchDecay = 0.06, .pitchShape = -50.0 } })
    {
        const auto fit = measure::fitPitch (kick.render (1.0), checkRate);
        INFO ("amount " << kick.amount << ", decay " << kick.pitchDecay << ", shape " << kick.pitchShape);
        CHECK (cents (fit.tuneHz, kick.tuneHz) < 100.0);
        CHECK (fit.amount == Catch::Approx (kick.amount).margin (1.0));
        CHECK (fit.decaySeconds == Catch::Approx (kick.pitchDecay).epsilon (0.1));
        CHECK (shapeExponent (fit.shape) == Catch::Approx (shapeExponent (kick.pitchShape)).epsilon (0.1));
        CHECK (fit.errorCents < 50.0);
    }
}

TEST_CASE ("harn_05e_amp_fit_gives_known_values_back")
{
    for (const auto& kick : { KnownKick { .amount = 0.0, .ampDecay = 0.4, .ampShape = 0.0 },
                              KnownKick { .amount = 0.0, .ampDecay = 0.8, .ampShape = 30.0 },
                              KnownKick { .amount = 0.0, .ampDecay = 0.25, .ampShape = -50.0 } })
    {
        const auto fit = measure::fitAmp (kick.render (1.5), checkRate);
        INFO ("decay " << kick.ampDecay << ", shape " << kick.ampShape);
        CHECK (fit.decaySeconds == Catch::Approx (kick.ampDecay).epsilon (0.1));
        CHECK (shapeExponent (fit.shape) == Catch::Approx (shapeExponent (kick.ampShape)).epsilon (0.1));
        CHECK (fit.errorDb < 3.0);
    }
}

TEST_CASE ("harn_05f_side_level_relative_to_mid")
{
    const auto mid = KnownKick {}.render (0.5);
    auto left = mid, right = mid;
    CHECK (measure::sideToMidDb (left, right, checkRate, 0.0, 0.5) < -100.0);

    for (size_t i = 0; i < mid.size(); ++i)
    {
        left[i] = 1.1f * mid[i];
        right[i] = 0.9f * mid[i];
    }
    CHECK (measure::sideToMidDb (left, right, checkRate, 0.0, 0.5) == Catch::Approx (-20.0).margin (0.1));
}

TEST_CASE ("harn_05b_a_loud_noise_click_does_not_stop_the_measurement")
{
    // Louder than the kick itself, as a sampled click can be. Must finish, and the tail stays readable.
    auto x = KnownKick { .tuneHz = 50.0, .ampDecay = 0.6, .ampShape = 0.0 }.render (0.8);
    std::minstd_rand noise (1);
    for (size_t i = 0; i < 96; ++i)
        x[i] += 2.0f * (static_cast<float> (noise()) / static_cast<float> (std::minstd_rand::max()) * 2.0f - 1.0f);

    CHECK (measure::tailHz (x, checkRate) == Catch::Approx (50.0).epsilon (0.005));
    for (auto t : { 0.005, 0.01, 0.02, 0.05, 0.1, 0.3 })
        CHECK (std::isfinite (measure::pitchAt (x, checkRate, t)));
    CHECK (measure::fitPitch (x, checkRate).tuneHz == Catch::Approx (50.0).epsilon (0.005));
}

TEST_CASE ("harn_05e_a_click_and_a_glitch_do_not_pull_the_pitch_fit")
{
    const KnownKick kick { .tuneHz = 50.0, .amount = 36.0, .pitchDecay = 0.1, .pitchShape = 0.0 };
    auto x = kick.render (1.0);
    std::minstd_rand noise (1);
    const auto noisy = [&noise] { return static_cast<float> (noise()) / static_cast<float> (std::minstd_rand::max()) * 2.0f - 1.0f; };
    for (size_t i = 0; i < 96; ++i)
        x[i] += 2.0f * noisy();
    for (size_t i = 7200; i < 7392; ++i) // 150 ms
        x[i] += 0.5f * noisy();

    const auto fit = measure::fitPitch (x, checkRate);
    CHECK (cents (fit.tuneHz, kick.tuneHz) < 100.0);
    CHECK (fit.amount == Catch::Approx (kick.amount).margin (1.0));
    CHECK (fit.decaySeconds == Catch::Approx (kick.pitchDecay).epsilon (0.1));
    CHECK (shapeExponent (fit.shape) == Catch::Approx (shapeExponent (kick.pitchShape)).epsilon (0.1));
    CHECK (fit.outliers > 0);
    CHECK (measure::fitPitch (KnownKick {}.render (1.0), checkRate).outliers == 0);
}

TEST_CASE ("harn_05e_a_real_second_stage_of_the_sweep_is_not_an_outlier")
{
    // A fast sweep, then a slow fall of 3 semitones over 600 ms that BODY's one curve cannot follow.
    // Those readings are the finding, so the fit may not leave them out to look better.
    std::vector<float> x (static_cast<size_t> (1.0 * checkRate));
    double phase = 0.0;
    for (size_t i = 0; i < x.size(); ++i)
    {
        const auto t = static_cast<double> (i) / checkRate;
        const auto semitones = 36.0 * bodyCurve (t, 0.08, 0.0) + 3.0 * std::max (0.0, 1.0 - t / 0.6);
        x[i] = static_cast<float> (std::sin (phase));
        phase += 2.0 * std::numbers::pi * 45.0 * std::pow (2.0, semitones / 12.0) / checkRate;
    }
    CHECK (measure::fitPitch (x, checkRate).outliers <= 2);
}

TEST_CASE ("harn_07a_loudness_reads_known_signals")
{
    const auto sineAt = [] (double rate, double hz, double level = 1.0) {
        std::vector<float> x (static_cast<size_t> (rate));
        for (size_t i = 0; i < x.size(); ++i)
            x[i] = static_cast<float> (level * std::sin (2.0 * std::numbers::pi * hz * static_cast<double> (i) / rate));
        return x;
    };

    SECTION ("997 Hz at 0 dBFS on both channels is 0 LUFS, as in BS.1770")
    {
        const auto x = sineAt (48000.0, 997.0);
        CHECK (measure::loudness (x, x, 48000.0) == Catch::Approx (0.0).margin (0.1));
    }
    SECTION ("on one channel it is 3 LU less")
    {
        const auto x = sineAt (48000.0, 997.0);
        CHECK (measure::loudness (x, std::vector<float> (x.size(), 0.0f), 48000.0) == Catch::Approx (-3.01).margin (0.1));
    }
    SECTION ("20 dB down reads 20 LU down")
    {
        const auto x = sineAt (48000.0, 997.0, 0.1);
        CHECK (measure::loudness (x, x, 48000.0) == Catch::Approx (-20.0).margin (0.1));
    }
    SECTION ("the K filter at 48 kHz has the coefficients printed in BS.1770")
    {
        const auto k = measure::kWeighting (48000.0);
        const std::array<double, 5> shelf { 1.53512485958697, -2.69169618940638, 1.19839281085285, -1.69065929318241, 0.73248077421585 };
        const std::array<double, 5> high { 1.0, -2.0, 1.0, -1.99004745483398, 0.99007225036621 };
        for (size_t i = 0; i < 5; ++i)
        {
            CHECK (k.shelf[i] == Catch::Approx (shelf[i]).margin (0.0001));
            CHECK (k.highpass[i] == Catch::Approx (high[i]).margin (0.0001));
        }
    }
    SECTION ("the same sine reads the same at every rate")
    {
        const auto at48 = sineAt (48000.0, 100.0);
        const auto base = measure::loudness (at48, at48, 48000.0);
        for (const auto rate : { 44100.0, 192000.0 })
        {
            const auto x = sineAt (rate, 100.0);
            CHECK (measure::loudness (x, x, rate) == Catch::Approx (base).margin (0.1));
        }
    }
    SECTION ("silence in front changes nothing: the window starts at time zero")
    {
        auto x = sineAt (48000.0, 997.0);
        const auto base = measure::loudness (x, x, 48000.0);
        x.insert (x.begin(), 4800, 0.0f);
        CHECK (measure::loudness (x, x, 48000.0) == Catch::Approx (base).margin (0.1));
    }
}
