#include "PluginProcessor.h"
#include <ui/PanelEditor.h>

namespace
{
constexpr const char* paramIds[] = { "body_wave", "body_tune", "body_pitch_amount", "body_pitch_decay", "body_pitch_shape",
                                     "body_amp_attack", "body_amp_hold", "body_amp_decay", "body_amp_shape",
                                     "body_drive", "body_filter_cutoff", "body_filter_res", "body_filter_env",
                                     "body_sweep2_amount", "body_sweep2_decay", "body_start",
                                     "play_velocity", "play_key_track", "body_tail_anchor",
                                     "click_source", "click_level", "click_tune", "click_decay", "click_hp", "click_lp",
                                     "top_source", "top_level", "top_tune", "top_decay", "top_hp", "top_lp",
                                     "click_start", "top_start" };
}

// C3 = 60. Takes "F#0", "C1", "Bb2" or a plain MIDI note number.
int PluginProcessor::noteFromText (const juce::String& text)
{
    const auto s = text.trim();
    const auto letter = juce::String ("CDEFGAB").indexOfChar (juce::CharacterFunctions::toUpperCase (s[0]));
    if (letter < 0)
        return s.getIntValue();

    static constexpr int semitones[] = { 0, 2, 4, 5, 7, 9, 11 };
    auto note = semitones[letter];
    auto rest = s.substring (1);
    if (rest.startsWithChar ('#') || rest.startsWithChar ('b'))
    {
        note += rest[0] == '#' ? 1 : -1;
        rest = rest.substring (1);
    }
    return (rest.getIntValue() + 2) * 12 + note;
}

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KikkuhParams", createLayout())
{
    for (size_t i = 0; i < raw.size(); ++i)
        raw[i] = apvts.getRawParameterValue (paramIds[i]);

    // The wavetables take a moment to build: do it here, never on the audio thread.
    dsp::MorphOsc::warmUp();
    dsp::LayerVoice::warmUp();
}

PluginProcessor::~PluginProcessor()
{
    loader.removeAllJobs (true, 10000);
    cancelPendingUpdate();
    for (size_t i = 0; i < 2; ++i)
    {
        delete pendingSample[i].load();
        delete retiredSample[i].load();
        delete currentSample[i];
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createLayout()
{
    using Float = juce::AudioParameterFloat;
    using Attributes = juce::AudioParameterFloatAttributes;
    // ENG-06a: a host sets the knob, and a state stores the value, so the knob a value reloads to must give exactly that
    // value back. A skewed range's float round trip can land an ulp off (clap-validator reports it), so the knob is the float
    // among the neighbours of the formula's answer whose value is exactly the one asked for.
    const auto ranged = [] (float low, float high, float centre = 0.0f) {
        const auto skew = centre > 0.0f ? std::log (0.5) / std::log ((centre - low) / static_cast<double> (high - low)) : 1.0;
        const auto toValue = [skew] (float start, float end, float knob) {
            return static_cast<float> (start + (end - start) * std::pow (static_cast<double> (knob), 1.0 / skew));
        };
        return juce::NormalisableRange<float> (
            low, high, toValue,
            [skew, toValue] (float start, float end, float value) {
                const auto guess = static_cast<float> (std::pow ((value - start) / static_cast<double> (end - start), skew));
                auto knob = guess;
                for (int i = 0; i < 8 && toValue (start, end, knob) < value; ++i)
                    knob = std::nextafter (knob, 2.0f);
                for (int i = 0; i < 8 && toValue (start, end, knob) > value; ++i)
                    knob = std::nextafter (knob, -1.0f);
                return juce::exactlyEqual (toValue (start, end, knob), value) ? knob : guess;
            },
            [] (float start, float end, float value) { return juce::jlimit (start, end, value); });
    };

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto add = [&] (const char* id, const char* name, juce::NormalisableRange<float> range, float def, const char* unit, int decimals) {
        // Fixed decimals, so the text survives value -> text -> value -> text (clap-validator).
        layout.add (std::make_unique<Float> (juce::ParameterID { id, 1 }, name, range, def,
                                             Attributes().withLabel (unit).withStringFromValueFunction ([decimals] (float v, int) { if (decimals == 0)
                                                                                                         return juce::String (juce::roundToInt (v));
                                                                                                     const auto text = juce::String (v, decimals);
                                                                                                     return text.containsOnly ("-0.") ? text.replace ("-", "") : text; })));
    };

    add ("body_wave", "Wave", ranged (0.0f, 3.0f), 0.0f, "", 2);
    add ("body_start", "Start", ranged (0.0f, 90.0f), 90.0f, "deg", 0);
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "body_tune", 1 }, "Tune", 24, 60, 33,
        juce::AudioParameterIntAttributes()
            .withStringFromValueFunction ([] (int v, int) { return juce::String (dsp::bodyNoteName (v)); })
            .withValueFromStringFunction (&PluginProcessor::noteFromText)));
    add ("body_pitch_amount", "Pitch Amount", ranged (0.0f, 72.0f), 60.0f, "st", 1);
    add ("body_pitch_decay", "Pitch Decay", ranged (1.0f, 1000.0f, 60.0f), 40.0f, "ms", 1);
    add ("body_pitch_shape", "Pitch Shape", ranged (-100.0f, 100.0f), 30.0f, "%", 0);
    add ("body_sweep2_amount", "Sweep 2 Amount", ranged (0.0f, 24.0f), 6.0f, "st", 1);
    add ("body_sweep2_decay", "Sweep 2 Decay", ranged (1.0f, 2000.0f, 200.0f), 300.0f, "ms", 1);
    add ("body_amp_attack", "Attack", ranged (0.0f, 50.0f, 5.0f), 0.0f, "ms", 1);
    add ("body_amp_hold", "Hold", ranged (0.0f, 500.0f, 50.0f), 0.0f, "ms", 1);
    add ("body_amp_decay", "Amp Decay", ranged (10.0f, 2000.0f, 200.0f), 400.0f, "ms", 1);
    add ("body_amp_shape", "Amp Shape", ranged (-100.0f, 100.0f), 0.0f, "%", 0);
    add ("body_drive", "Drive", ranged (0.0f, 24.0f), 6.0f, "dB", 1);
    add ("body_filter_cutoff", "Cutoff", ranged (20.0f, 20000.0f, 632.0f), 20000.0f, "Hz", 0);
    add ("body_filter_res", "Resonance", ranged (0.0f, 100.0f), 0.0f, "%", 0);
    add ("body_filter_env", "Env Amount", ranged (0.0f, 8.0f), 0.0f, "oct", 1);
    add ("play_velocity", "Velocity", ranged (0.0f, 100.0f), 100.0f, "%", 0);
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "play_key_track", 1 }, "Key Track", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "body_tail_anchor", 1 }, "Tail Anchor", true));

    // LAYER-01/02: CLICK and TOP, the same knobs with their own defaults.
    for (const auto* prefix : { "click", "top" })
    {
        const bool isClick = juce::String (prefix) == "click";
        const juce::String id (prefix), name (isClick ? "Click " : "Top ");
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id + "_source", 1 }, name + "Source", juce::StringArray { "Tone", "Noise", "Sample" }, isClick ? 0 : 1));
        layout.add (std::make_unique<Float> (juce::ParameterID { id + "_level", 1 }, name + "Level", ranged (-60.0f, 6.0f), -60.0f,
                                             Attributes().withLabel ("dB")
                                                 .withStringFromValueFunction ([] (float v, int) { return v < -59.95f ? juce::String ("Off") : juce::String (v, 1).replace ("-0.0", "0.0"); })
                                                 .withValueFromStringFunction ([] (const juce::String& text) { return text.trim().equalsIgnoreCase ("Off") ? -60.0f : text.getFloatValue(); })));
        add ((id + "_tune").toRawUTF8(), (name + "Tune").toRawUTF8(), ranged (-24.0f, 24.0f), 0.0f, "st", 1);
        add ((id + "_decay").toRawUTF8(), (name + "Decay").toRawUTF8(), ranged (1.0f, 2000.0f, 50.0f), isClick ? 4.0f : 30.0f, "ms", 1);
        add ((id + "_hp").toRawUTF8(), (name + "High-pass").toRawUTF8(), ranged (20.0f, 20000.0f, 632.0f), isClick ? 20.0f : 1000.0f, "Hz", 0);
        add ((id + "_lp").toRawUTF8(), (name + "Low-pass").toRawUTF8(), ranged (20.0f, 20000.0f, 632.0f), 20000.0f, "Hz", 0);
        add ((id + "_start").toRawUTF8(), (name + "Start").toRawUTF8(), ranged (0.0f, 1000.0f, 50.0f), 0.0f, "ms", 1);
    }
    return layout;
}

dsp::LayerParams PluginProcessor::readLayer (size_t first, size_t start) const
{
    return { raw[first]->load(), raw[first + 1]->load(), raw[first + 2]->load(), raw[first + 3]->load(), raw[first + 4]->load(), raw[first + 5]->load(),
             raw[start]->load() };
}

dsp::BodyParams PluginProcessor::readParams() const
{
    return { raw[0]->load(), raw[1]->load(), raw[2]->load(), raw[3]->load(), raw[4]->load(),
             raw[5]->load(), raw[6]->load(), raw[7]->load(), raw[8]->load(),
             raw[9]->load(), raw[10]->load(), raw[11]->load(), raw[12]->load(),
             raw[13]->load(), raw[14]->load(), raw[15]->load(), raw[18]->load() };
}

void PluginProcessor::prepareToPlay (double newRate, int)
{
    voice.setParams (readParams());
    voice.prepare (newRate);
    click.setParams (readLayer (19, 31));
    top.setParams (readLayer (25, 32));
    click.prepare (newRate);
    top.prepare (newRate);
    sampleRate = newRate;
    loop.reset();
}

// FAST-01: the loop follows the host's PPQ while the transport runs, and its tempo while it stands still.
void PluginProcessor::beginLoop (int numSamples, bool on)
{
    bool rolling = false;
    double ppq = 0.0, bpm = 0.0;
    if (auto* head = getPlayHead())
        if (const auto position = head->getPosition())
        {
            const auto ppqPosition = position->getPpqPosition();
            const auto tempo = position->getBpm();
            bpm = tempo ? *tempo : 0.0;
            ppq = ppqPosition ? *ppqPosition : 0.0;
            rolling = position->getIsPlaying() && ppqPosition && std::isfinite (ppq) && tempo && bpm >= 1.0;
        }
    loop.begin (on, numSamples, sampleRate, rolling, ppq, bpm);
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    voice.setParams (readParams());
    click.setParams (readLayer (19, 31));
    top.setParams (readLayer (25, 32));

    auto render = [&] (int start, int end) {
        for (int i = start; i < end; ++i)
        {
            const auto sample = voice.next() + click.next() + top.next();
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.getWritePointer (ch)[i] = sample;
        }
    };

    // FAST-01: the pad and the loop are heard live only; a bounce never holds them.
    const auto live = ! isNonRealtime();
    const auto kick = [&] (float level, int note) {
        voice.trigger (level, note);
        click.trigger (level, takeSample (0));
        top.trigger (level, takeSample (1));
        ++kicks;
    };
    if (padPressed.exchange (false) && live)
        kick (1.0f, -1);

    beginLoop (buffer.getNumSamples(), live && loopOn);
    int loopHit = loop.next();
    // Loop hits up to `limit`, in order; one at the same sample as a MIDI note goes first.
    int position = 0;
    const auto playLoop = [&] (int limit) {
        for (; loopHit >= 0 && loopHit <= limit; loopHit = loop.next())
        {
            render (position, juce::jmax (position, loopHit));
            position = juce::jmax (position, loopHit);
            kick (1.0f, -1);
            ++loopHits;
        }
    };

    for (const auto metadata : midi)
    {
        const auto offset = juce::jlimit (position, buffer.getNumSamples(), metadata.samplePosition);
        playLoop (offset);
        render (position, offset);
        position = offset;

        const auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            // PLAY-01/02: both knobs are read here, at the note-on, and never smoothed.
            const auto level = 1.0f - raw[16]->load() * 0.01f * (1.0f - message.getFloatVelocity());
            kick (level, raw[17]->load() > 0.5f ? juce::jlimit (24, 60, message.getNoteNumber()) : -1);
        }
    }
    playLoop (buffer.getNumSamples());
    render (position, buffer.getNumSamples());
}

// LAYER-04b: a new sample is taken at a note-on, so a ringing layer keeps the one it started with. Nothing is freed here.
const dsp::LayerSample* PluginProcessor::takeSample (size_t section)
{
    if (retiredSample[section].load() == nullptr)
        if (auto* next = pendingSample[section].exchange (nullptr))
        {
            retiredSample[section].store (currentSample[section]);
            currentSample[section] = next;
        }
    return currentSample[section];
}

bool PluginProcessor::canLoadSample (const juce::File& file)
{
    return file.hasFileExtension ("wav;aif;aiff;flac");
}

bool PluginProcessor::loadSample (int section, const juce::StringArray& paths)
{
    juce::Array<juce::File> files;
    for (const auto& path : paths)
        if (canLoadSample (juce::File (path)))
            files.add (juce::File (path));
    if (files.isEmpty())
        return false;

    ++loadsInFlight;
    loader.addJob ([this, section, files] {
        Loaded result;
        result.section = section;
        result.name = files[0].getFileName();
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        for (const auto& file : files)
            if (const std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file)); reader != nullptr && reader->sampleRate > 0.0)
            {
                // LAYER-04: mono as the mean of the channels, at the file's own rate, cut at 3 s.
                const auto length = static_cast<int> (std::min<juce::int64> (reader->lengthInSamples, juce::roundToInt (3.0 * reader->sampleRate)));
                const auto channels = static_cast<int> (reader->numChannels);
                juce::AudioBuffer<float> buffer (channels, juce::jmax (length, 1));
                if (length > 0 && reader->read (&buffer, 0, length, 0, true, true))
                {
                    result.data.assign (static_cast<size_t> (length), 0.0f);
                    for (int ch = 0; ch < channels; ++ch)
                        for (int i = 0; i < length; ++i)
                            result.data[static_cast<size_t> (i)] += buffer.getSample (ch, i) / static_cast<float> (channels);
                    result.ok = true;
                    result.name = file.getFileName();
                    result.rate = reader->sampleRate;
                    break;
                }
            }
        {
            const juce::ScopedLock lock (loadedLock);
            loaded.push_back (std::move (result));
        }
        if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) // none in a test or a render tool: waitForSampleLoads publishes
            triggerAsyncUpdate();
        --loadsInFlight;
    });
    return true;
}

void PluginProcessor::waitForSampleLoads()
{
    while (loadsInFlight > 0)
        juce::Thread::sleep (1);
    cancelPendingUpdate();
    handleAsyncUpdate(); // not through the message loop: a caller without one (a test, a render tool) would never see it
}

juce::String PluginProcessor::getSampleText (int section) const
{
    const juce::ScopedLock lock (heldLock);
    return held[static_cast<size_t> (section)].text;
}

// Message thread: a finished load becomes the section's sample and sets its Source and, at Off, its Level.
void PluginProcessor::handleAsyncUpdate()
{
    std::vector<Loaded> done;
    {
        const juce::ScopedLock lock (loadedLock);
        done.swap (loaded);
    }
    for (auto& result : done)
    {
        const auto prefix = result.section == 0 ? "click" : "top";
        if (! result.ok)
        {
            const juce::ScopedLock lock (heldLock);
            held[static_cast<size_t> (result.section)].text = "Cannot read " + result.name;
            continue;
        }
        publishSample (result.section, result.name, std::move (result.data), result.rate, result.name);
        auto* source = apvts.getParameter (juce::String (prefix) + "_source");
        source->setValueNotifyingHost (source->convertTo0to1 (2.0f));
        auto* level = apvts.getParameter (juce::String (prefix) + "_level");
        if (level->convertFrom0to1 (level->getValue()) < -59.95f)
            level->setValueNotifyingHost (level->convertTo0to1 (-6.0f));
    }
}

// Message thread. An empty `data` clears the section: the audio thread plays silence.
void PluginProcessor::publishSample (int section, const juce::String& name, std::vector<float> data, double rate, const juce::String& text)
{
    const auto s = static_cast<size_t> (section);
    auto* audio = new dsp::LayerSample { data, rate };
    {
        const juce::ScopedLock lock (heldLock);
        held[s] = { name, text, std::move (data), rate };
    }
    delete retiredSample[s].exchange (nullptr);
    delete pendingSample[s].exchange (audio); // one the audio thread never took
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return layouts.getMainInputChannelSet().isDisabled()
           && (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono());
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PanelEditor (*this);
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement xml ("KikkuhState");
    xml.setAttribute ("version", stateVersion);
    auto params = apvts.copyState().createXml();
    // ENG-06a: nine significant digits bring every value back exactly; the default writing rounds 20.0000019 to 20.0.
    for (auto* param : params->getChildIterator())
        if (auto* value = apvts.getRawParameterValue (param->getStringAttribute ("id")))
            param->setAttribute ("value", juce::String::formatted ("%.9g", static_cast<double> (value->load())));
    xml.addChildElement (params.release());

    // LAYER-04g: each section's sample goes with the project, as base64 of its little-endian float32 audio.
    const juce::ScopedLock lock (heldLock);
    for (size_t i = 0; i < 2; ++i)
        if (! held[i].data.empty())
        {
            auto* child = xml.createNewChildElement ("Sample");
            child->setAttribute ("section", i == 0 ? "click" : "top");
            child->setAttribute ("name", held[i].name);
            child->setAttribute ("rate", held[i].rate);
            child->setAttribute ("data", juce::Base64::toBase64 (held[i].data.data(), held[i].data.size() * sizeof (float)));
        }
    copyXmlToBinary (xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName ("KikkuhState"))
        return;

    migrate (*xml, xml->getIntAttribute ("version", 1));

    // A state without parameters (VS0-01) restores every parameter to its default.
    const auto* saved = xml->getChildByName (apvts.state.getType());
    apvts.replaceState (saved != nullptr ? juce::ValueTree::fromXml (*saved) : juce::ValueTree (apvts.state.getType()));

    // ENG-06a: replaceState skips a value that is only approximately the current one, so the exact one is set here.
    if (saved != nullptr)
        for (auto* param : saved->getChildIterator())
            if (auto* p = apvts.getParameter (param->getStringAttribute ("id")))
                if (const auto value = static_cast<float> (param->getDoubleAttribute ("value")); ! juce::exactlyEqual (p->convertFrom0to1 (p->getValue()), value))
                    p->setValueNotifyingHost (p->convertTo0to1 (value));

    // LAYER-04g/h: the samples; one that cannot be decoded clears its section and the rest of the state loads on.
    for (size_t i = 0; i < 2; ++i)
    {
        auto* child = xml->getChildByAttribute ("section", i == 0 ? "click" : "top");
        if (child == nullptr || ! child->hasTagName ("Sample"))
        {
            publishSample (static_cast<int> (i), {}, {}, 0.0, "Drop a sample");
            continue;
        }
        const auto name = child->getStringAttribute ("name");
        juce::MemoryOutputStream bytes;
        std::vector<float> samples;
        const auto rate = child->getDoubleAttribute ("rate");
        if (juce::Base64::convertFromBase64 (bytes, child->getStringAttribute ("data")) && bytes.getDataSize() > 0
            && bytes.getDataSize() % sizeof (float) == 0 && rate > 0.0)
        {
            samples.resize (bytes.getDataSize() / sizeof (float));
            std::memcpy (samples.data(), bytes.getData(), bytes.getDataSize());
        }
        const auto text = samples.empty() ? "Missing: " + name : name;
        publishSample (static_cast<int> (i), name, std::move (samples), rate, text);
    }
}

void PluginProcessor::migrate (juce::XmlElement& xml, int fromVersion)
{
    // Later versions rewrite older states here, one step per version; a state without knobs takes the defaults.
    const auto addMissing = [&] (std::initializer_list<const char*> ids) {
        if (auto* params = xml.getChildByName (apvts.state.getType()))
            for (const auto* id : ids)
            {
                bool found = false;
                for (auto* child : params->getChildIterator())
                    found = found || child->getStringAttribute ("id") == id;
                if (! found)
                {
                    auto* param = params->createNewChildElement ("PARAM");
                    param->setAttribute ("id", id);
                    param->setAttribute ("value", 0.0);
                }
            }
    };
    // v2 (BODY-08..10): the defaults changed, so a project saved earlier keeps the sound it had.
    if (fromVersion < 2)
        addMissing ({ "body_sweep2_amount", "body_start", "body_drive" });
    // VS2-02: Start did not exist, and 0 is its default.
    addMissing ({ "click_start", "top_start" });
    // v3 (BODY-07): the same for the tail anchor, which is on by default.
    if (fromVersion < 3)
        addMissing ({ "body_tail_anchor" });
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
