#include "Measure.h"
#include <Sketch.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <numbers>

// HARN-08: the sketch voice, for the listening rounds. Expected values come from the spec's formulas.
namespace
{
const auto rate48 = 48000.0;

double db (double ratio)
{
    return 20.0 * std::log10 (ratio);
}

sketch::Params with (std::initializer_list<std::pair<const char*, const char*>> keys)
{
    sketch::Params p;
    for (const auto& [key, value] : keys)
    {
        INFO (key);
        REQUIRE (sketch::set (p, key, value));
    }
    return p;
}

std::vector<float> sine192 (double hz, double level = 1.0, double seconds = 0.5)
{
    std::vector<float> x (static_cast<size_t> (seconds * 192000.0));
    for (size_t i = 0; i < x.size(); ++i)
        x[i] = static_cast<float> (level * std::sin (2.0 * std::numbers::pi * hz * static_cast<double> (i) / 192000.0));
    return x;
}

double hz (double midi)
{
    return 440.0 * std::pow (2.0, (midi - 69.0) / 12.0);
}

// A steady A0: no sweep, a long hold.
const std::initializer_list<std::pair<const char*, const char*>> steady { { "body_pitch_amount", "0" }, { "body_amp_hold", "500" }, { "body_amp_decay", "100" } };

sketch::Params steadyWith (std::initializer_list<std::pair<const char*, const char*>> keys)
{
    auto p = with (steady);
    for (const auto& [key, value] : keys)
        REQUIRE (sketch::set (p, key, value));
    return p;
}

// Level of a sine through the sketch's shaping stage at 192 kHz, after the settling.
double filterGainDb (const sketch::Params& p, double toneHz)
{
    const auto level = 0.01;
    const auto out = sketch::shape (p, sine192 (toneHz, level));
    return db (measure::toneLevel (out, 192000.0, toneHz, 0.2, 0.5) / level);
}
}

TEST_CASE ("harn_08a_plain_means_body")
{
    auto compare = [] (const sketch::Params& p) {
        dsp::BodyVoice voice;
        voice.setParams (p.body);
        voice.prepare (rate48);
        voice.trigger (1.0f);
        const auto sketchOut = sketch::renderRaw (p, rate48, 3.0);
        float top = 0.0f, difference = 0.0f;
        for (const auto s : sketchOut)
        {
            const auto b = voice.next();
            top = std::max (top, std::abs (b));
            difference = std::max (difference, std::abs (b - s));
        }
        CHECK (top > 0.5f);
        CHECK (db (std::max (difference, 1.0e-9f) / top) < -90.0);
    };

    SECTION ("defaults") { compare ({}); }
    SECTION ("away from them")
    {
        compare (with ({ { "body_wave", "1.5" }, { "body_tune", "40" }, { "body_pitch_amount", "60" }, { "body_pitch_decay", "40" },
                         { "body_pitch_shape", "30" }, { "body_amp_attack", "2.5" }, { "body_amp_hold", "20" }, { "body_amp_decay", "300" }, { "body_amp_shape", "-20" } }));
    }
}

TEST_CASE ("body_09b_and_10a_the_voice_does_what_the_sketch_did")
{
    const auto difference = [] (sketch::Params p, float amount, float decayMs, float start) {
        p.sweep2Amount = amount;
        p.sweep2DecayMs = decayMs;
        p.startPhase = start;
        p.body.sweep2Amount = amount;
        p.body.sweep2DecayMs = decayMs;
        p.body.startDegrees = start;
        dsp::BodyVoice voice;
        voice.setParams (p.body);
        voice.prepare (rate48);
        voice.trigger (1.0f);
        float largest = 0.0f;
        for (const auto s : sketch::renderRaw (p, rate48, 1.0))
            largest = std::max (largest, std::abs (voice.next() - s));
        return largest;
    };
    const auto kick = with ({ { "body_pitch_amount", "60" }, { "body_pitch_decay", "40" }, { "body_pitch_shape", "30" } });

    // The sketch without a second stage is the render without this stage (BODY-09b): sample-identical.
    CHECK (difference (kick, 0.0f, 300.0f, 0.0f) == 0.0f);
    CHECK (db (difference (kick, 6.0f, 300.0f, 90.0f)) < -90.0);
}

TEST_CASE ("harn_08a_set_rejects_what_the_spec_does_not_list")
{
    sketch::Params p;
    CHECK_FALSE (sketch::set (p, "body_nonsense", "1"));
    CHECK_FALSE (sketch::set (p, "tune_cents", "101"));
    CHECK_FALSE (sketch::set (p, "tune_cents", "abc"));
    CHECK_FALSE (sketch::set (p, "body_pitch_amount", "73"));
    CHECK_FALSE (sketch::set (p, "start_phase", "91"));
    CHECK_FALSE (sketch::set (p, "click_kind", "thunder"));
    CHECK_FALSE (sketch::set (p, "sat_kind", "fuzz"));
    CHECK_FALSE (sketch::set (p, "sat_place", "nowhere"));
    CHECK_FALSE (sketch::set (p, "filter_kind", "comb"));
    CHECK (sketch::set (p, "body_tune", "F#0"));
    CHECK (p.body.tune == Catch::Approx (30.0f));
    CHECK (sketch::set (p, "body_tune", "33"));
}

TEST_CASE ("harn_08b_decimation")
{
    SECTION ("1 kHz passes within 0.1 dB")
    {
        const auto out = sketch::decimate (sine192 (1000.0));
        CHECK (std::abs (db (measure::toneLevel (out, rate48, 1000.0, 0.1, 0.4))) < 0.1);
    }
    SECTION ("30 kHz falls at least 90 dB")
    {
        const auto out = sketch::decimate (sine192 (30000.0));
        CHECK (db (std::max (measure::peakBetween (out, rate48, 0.1, 0.4), 1.0e-9f)) < -90.0);
    }
    SECTION ("a kick starts on the same sample within one, and at the same level")
    {
        const sketch::Params p;
        const auto direct = sketch::renderRaw (p, rate48, 0.5);
        const auto down = sketch::render48k (p, 0.5);
        REQUIRE (down.size() == direct.size());
        CHECK (std::abs (measure::onset (down) - measure::onset (direct)) <= 1);
        CHECK (std::abs (db (static_cast<double> (measure::peak (down)) / measure::peak (direct))) < 0.1);
    }
}

TEST_CASE ("harn_08c_tuning_between_notes")
{
    const auto out = sketch::render48k (steadyWith ({ { "tune_cents", "-22" } }), 0.6);
    const auto cents = 1200.0 * std::log2 (measure::frequency (out, rate48, 0.1, 0.5) / 55.0);
    CHECK (cents == Catch::Approx (-22.0).margin (3.0));
}

TEST_CASE ("harn_08c_a_sweep_in_two_stages_is_the_sum_of_two_curves")
{
    const auto p = with ({ { "body_pitch_amount", "12" }, { "body_pitch_decay", "50" }, { "sweep2_amount", "3" }, { "sweep2_decay", "650" }, { "body_amp_decay", "2000" } });
    const auto out = sketch::render48k (p, 1.5);
    for (const auto ms : { 10.0, 75.0, 300.0, 500.0 })
    {
        INFO (ms);
        auto midi = 33.0 + 3.0 * dsp::bodyCurve (ms / 650.0, 0.0);
        if (ms < 50.0)
            midi += 12.0 * dsp::bodyCurve (ms / 50.0, 0.0);
        CHECK (1200.0 * std::abs (std::log2 (measure::pitchAt (out, rate48, ms / 1000.0) / hz (midi))) < 50.0);
    }
    CHECK (measure::frequency (out, rate48, 0.8, 1.2) == Catch::Approx (55.0).margin (55.0 * 0.005));
}

TEST_CASE ("harn_08d_hard_start")
{
    const auto over1k = [] (const std::vector<float>& x, double from, double to) {
        const auto s = measure::bandShares (x, rate48, from, to);
        return s[2] + s[3];
    };

    SECTION ("start_phase 90 starts at full level")
    {
        const auto out = sketch::render48k (with ({ { "start_phase", "90" } }), 0.5);
        CHECK (db (measure::largestStepBetween (out, rate48, 0.0, 0.001) / measure::peak (out)) > -10.0);
    }
    SECTION ("the plain kick does not")
    {
        const auto out = sketch::render48k ({}, 0.5);
        CHECK (db (measure::largestStepBetween (out, rate48, 0.0, 0.001) / measure::peak (out)) < -20.0);
        CHECK (over1k (out, 0.0, 0.005) < 1.0);
    }
    for (const auto* kind : { "tone", "noise" })
    {
        DYNAMIC_SECTION ("a click of kind " << kind)
        {
            const std::initializer_list<std::pair<const char*, const char*>> kick { { "body_amp_decay", "1000" } };
            auto p = with (kick);
            REQUIRE (sketch::set (p, "click_kind", kind));
            REQUIRE (sketch::set (p, "click_hz", "3000"));
            REQUIRE (sketch::set (p, "click_ms", "5"));
            REQUIRE (sketch::set (p, "click_db", "6"));
            const auto out = sketch::render48k (p, 0.5);
            const auto plain = sketch::render48k (with (kick), 0.5);
            CHECK (over1k (out, 0.0, 0.005) > 5.0);
            CHECK (std::abs (measure::bandShares (out, rate48, 0.05, 0.2)[0] - measure::bandShares (plain, rate48, 0.05, 0.2)[0]) < 1.0);
        }
    }
    SECTION ("noise is the same every time (ENG-03)")
    {
        const auto p = with ({ { "click_kind", "noise" }, { "click_ms", "5" }, { "click_db", "0" } });
        CHECK (sketch::render48k (p, 0.05) == sketch::render48k (p, 0.05));
    }
}

TEST_CASE ("harn_08e_a_dent_lowers_the_level_where_it_says")
{
    const auto plain = sketch::render48k (steadyWith ({ { "start_phase", "90" } }), 0.1);
    const auto out = sketch::render48k (steadyWith ({ { "start_phase", "90" }, { "dip_at", "1.5" }, { "dip_width", "1" }, { "dip_db", "-10" } }), 0.1);
    const auto ratioAt = [&] (double ms) {
        const auto i = static_cast<size_t> (ms * 48.0);
        return db (std::abs (out[i] / plain[i]));
    };
    CHECK (ratioAt (1.5) == Catch::Approx (-10.0).margin (1.0));
    for (const auto ms : { 3.0, 4.0 }) // a 55 Hz cosine is far from zero here
        CHECK (std::abs (ratioAt (ms)) < 1.0);
    for (const auto ms : { 5.0, 7.0, 10.0 })
        CHECK (std::abs (db (measure::peakBetween (out, rate48, ms / 1000.0, ms / 1000.0 + 0.001) / measure::peakBetween (plain, rate48, ms / 1000.0, ms / 1000.0 + 0.001))) < 1.0);
}

TEST_CASE ("harn_08f_saturation_has_the_harmonics_of_its_kind")
{
    const auto harmonics = [] (const char* kind) {
        const auto out = sketch::render48k (steadyWith ({ { "sat_kind", kind }, { "sat_drive", "12" } }), 0.5);
        const auto one = measure::toneLevel (out, rate48, 55.0, 0.1, 0.45);
        struct H
        {
            double second, third, dc;
        };
        double mean = 0.0;
        const auto from = static_cast<size_t> (0.1 * rate48), count = static_cast<size_t> (0.3454545 * rate48); // whole periods of 55 Hz
        for (size_t i = from; i < from + count; ++i)
            mean += static_cast<double> (out[i]) / static_cast<double> (count);
        return H { db (measure::toneLevel (out, rate48, 110.0, 0.1, 0.45) / one), db (measure::toneLevel (out, rate48, 165.0, 0.1, 0.45) / one), db (std::max (std::abs (mean), 1.0e-9) / one) };
    };

    for (const auto* kind : { "soft", "hard" })
    {
        const auto h = harmonics (kind);
        INFO (kind);
        CHECK (h.third > -40.0);
        CHECK (h.second < -60.0);
    }
    const auto asym = harmonics ("asym");
    CHECK (asym.second > -40.0);
    CHECK (asym.dc < -60.0);
}

TEST_CASE ("harn_08f_the_place_decides_whether_the_tail_stays_saturated")
{
    const auto drop = [] (const char* place) {
        const auto p = with ({ { "body_pitch_amount", "0" }, { "body_amp_decay", "2000" }, { "body_amp_shape", "-100" }, { "sat_kind", "soft" }, { "sat_drive", "12" }, { "sat_place", place } });
        const auto out = sketch::render48k (p, 2.0);
        const auto thirdDb = [&] (double from, double to) { return db (measure::toneLevel (out, rate48, 165.0, from, to) / measure::toneLevel (out, rate48, 55.0, from, to)); };
        return thirdDb (0.1, 0.3) - thirdDb (1.6, 1.8);
    };
    CHECK (std::abs (drop ("before_amp")) < 3.0);
    CHECK (drop ("after_amp") > 10.0);
}

TEST_CASE ("harn_08f_saturation_does_not_alias")
{
    const auto check = [] (const char* kind, double toneHz) {
        INFO (kind);
        const auto p = with ({ { "sat_kind", kind }, { "sat_drive", "24" } });
        const auto out = sketch::decimate (sketch::shape (p, sine192 (toneHz)));
        CHECK (measure::inharmonicDb (out, rate48, toneHz, 4800) < -80.0);
    };
    check ("soft", 5000.0);
    check ("asym", 5000.0);
    check ("hard", 200.0);
}

TEST_CASE ("harn_08g_filter_slopes_and_cutoff")
{
    const auto slope = [] (const char* kind) {
        const auto p = with ({ { "filter_kind", kind }, { "filter_cutoff", "500" } });
        return filterGainDb (p, 2000.0) - filterGainDb (p, 4000.0);
    };
    CHECK (slope ("svf12") == Catch::Approx (12.0).margin (3.0));
    CHECK (slope ("svf24") == Catch::Approx (24.0).margin (3.0));
    CHECK (slope ("ladder") == Catch::Approx (24.0).margin (3.0));

    for (const auto* kind : { "svf12", "svf24" })
    {
        INFO (kind);
        const auto p = with ({ { "filter_kind", kind }, { "filter_cutoff", "1000" }, { "filter_res", "0" } });
        CHECK (filterGainDb (p, 900.0) > -3.0);
        CHECK (filterGainDb (p, 1100.0) < -3.0);
    }
}

TEST_CASE ("harn_08g_the_env_opens_the_filter_early_and_not_late")
{
    const auto p = with ({ { "body_wave", "2" }, { "body_pitch_amount", "0" }, { "body_amp_decay", "1000" }, { "filter_kind", "svf24" },
                           { "filter_cutoff", "200" }, { "filter_env", "5" }, { "filter_decay", "100" } });
    const auto out = sketch::render48k (p, 0.6);
    const auto over1k = [&] (double from, double to) {
        const auto s = measure::bandShares (out, rate48, from, to);
        return s[2] + s[3];
    };
    CHECK (over1k (0.0, 0.02) > 1.0);
    CHECK (over1k (0.2, 0.4) < 0.1);
}
