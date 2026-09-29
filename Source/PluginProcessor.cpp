#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace IDs
{
    static constexpr const char* root = "root";
    static constexpr const char* type = "type";
    static constexpr const char* capo = "capo";
    static constexpr const char* pattern = "pattern";
    static constexpr const char* mode = "mode";
    static constexpr const char* strumSpeed = "strumSpeed";
    static constexpr const char* humanize = "humanize";
    static constexpr const char* palm = "palm";
    static constexpr const char* dynamics = "dynamics";
    static constexpr const char* tone = "tone";
    static constexpr const char* room = "room";
    static constexpr const char* tight = "tight";
    static constexpr const char* output = "output";
}

JGKAcousticAudioProcessor::JGKAcousticAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout JGKAcousticAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterInt>(IDs::root, "Chord Root", 0, 11, 0));
    p.push_back(std::make_unique<juce::AudioParameterInt>(IDs::type, "Chord Type", 0, 7, 0));
    p.push_back(std::make_unique<juce::AudioParameterInt>(IDs::capo, "Capo", 0, 12, 0));
    p.push_back(std::make_unique<juce::AudioParameterInt>(IDs::pattern, "Pattern", 0, 7, 2));
    p.push_back(std::make_unique<juce::AudioParameterInt>(IDs::mode, "Mode", 0, 2, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::strumSpeed, "Strum Speed", 0.0f, 1.0f, 0.36f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::humanize, "Humanize", 0.0f, 1.0f, 0.20f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::palm, "Palm Mute", 0.0f, 1.0f, 0.05f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::dynamics, "Dynamics", 0.0f, 1.0f, 0.58f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::tone, "Tone", 0.0f, 1.0f, 0.62f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::room, "Room", 0.0f, 1.0f, 0.14f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::tight, "Tight Loose", 0.0f, 1.0f, 0.30f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(IDs::output, "Output", -18.0f, 6.0f, -5.0f));
    return { p.begin(), p.end() };
}

void JGKAcousticAudioProcessor::prepareToPlay(double sr, int samplesPerBlock)
{
    currentSampleRate = sr;
    engine.prepare(sr, samplesPerBlock);
    samplesUntilNextStep = 0.0;
    currentStep = 0;
    transportWasPlaying = false;
    heldAutoChord.clear();
}

bool JGKAcousticAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

JGKAcousticAudioProcessor::Pattern JGKAcousticAudioProcessor::getPattern(int i) const
{
    Pattern p;
    p.steps.fill(-1);
    p.divisions = 8;
    p.length = 8;

    switch (juce::jlimit(0, 7, i))
    {
        case 0: p.steps = {0,0,0,0,0,0,0,0,-1,-1,-1,-1,-1,-1,-1,-1}; break;
        case 1: p.steps = {0,-1,0,1,-1,1,0,1,-1,-1,-1,-1,-1,-1,-1,-1}; break;
        case 2: p.steps = {0,-1,0,1,-1,1,0,1,-1,-1,-1,-1,-1,-1,-1,-1}; break;
        case 3: p.steps = {0,-1,-1,1,0,-1,1,-1,-1,-1,-1,-1,-1,-1,-1,-1}; break;
        case 4: p.steps = {0,1,0,1,0,1,0,1,-1,-1,-1,-1,-1,-1,-1,-1}; break;
        case 5: p.steps = {2,-1,2,1,2,-1,2,1,-1,-1,-1,-1,-1,-1,-1,-1}; break;
        case 6: p.steps = {0,-1,1,-1,0,1,-1,1,-1,-1,-1,-1,-1,-1,-1,-1}; break;
        case 7:
            p.divisions = 16;
            p.length = 16;
            p.steps = {0,-1,1,0,-1,1,0,1,0,-1,1,-1,0,1,0,1};
            break;
    }
    return p;
}

std::vector<int> JGKAcousticAudioProcessor::makeChordNotes(int root, int type) const
{
    static const int intervals[8][4] = {
        {0,4,7,-1}, {0,3,7,-1}, {0,4,7,10}, {0,4,7,11},
        {0,3,7,10}, {0,2,7,-1}, {0,5,7,-1}, {0,2,4,7}
    };

    const int base = 48 + juce::jlimit(0, 11, root);
    std::vector<int> notes;
    for (int i = 0; i < 4; ++i)
    {
        const int iv = intervals[juce::jlimit(0, 7, type)][i];
        if (iv >= 0)
            notes.push_back(base + iv);
    }
    return notes;
}

void JGKAcousticAudioProcessor::syncEngineParameters(bool mutedStep, bool up)
{
    const float speed = apvts.getRawParameterValue(IDs::strumSpeed)->load();
    const float human = apvts.getRawParameterValue(IDs::humanize)->load();
    const float userPalm = apvts.getRawParameterValue(IDs::palm)->load();
    const float tone = apvts.getRawParameterValue(IDs::tone)->load();
    const float room = apvts.getRawParameterValue(IDs::room)->load();
    const float output = apvts.getRawParameterValue(IDs::output)->load();
    const int capo = (int) apvts.getRawParameterValue(IDs::capo)->load();
    const int mode = (int) apvts.getRawParameterValue(IDs::mode)->load();

    float strumMs = juce::jmap(speed, 8.0f, 65.0f);
    if (mode == 1)
        strumMs = juce::jmax(42.0f, strumMs * 1.55f);

    engine.setCapo(capo);
    engine.setStrumMs(strumMs);
    engine.setStrumUp(up);
    engine.setHumanize(human);
    engine.setPalmMute(mutedStep ? juce::jmax(0.72f, userPalm) : userPalm);
    engine.setTone(tone);
    engine.setRoom(room);
    engine.setOutputDb(output);
}

void JGKAcousticAudioProcessor::triggerSelectedChord(bool up, bool muted, float intensity)
{
    const int root = (int) apvts.getRawParameterValue(IDs::root)->load();
    const int type = (int) apvts.getRawParameterValue(IDs::type)->load();
    const auto notes = makeChordNotes(root, type);

    if (notes != heldAutoChord)
    {
        releaseSelectedChord();
        heldAutoChord = notes;
    }

    syncEngineParameters(muted, up);

    const float dynamics = apvts.getRawParameterValue(IDs::dynamics)->load();
    const float velocity = juce::jlimit(0.15f, 1.0f, 0.44f + dynamics * 0.46f) * intensity;

    for (int n : heldAutoChord)
        engine.noteOn(n, velocity, 0);
}

void JGKAcousticAudioProcessor::releaseSelectedChord()
{
    for (int n : heldAutoChord)
        engine.noteOff(n, 0);
    heldAutoChord.clear();
}

void JGKAcousticAudioProcessor::handleManualMidi(const juce::MidiBuffer& midi)
{
    syncEngineParameters(false, false);
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (message.isNoteOn())
            engine.noteOn(message.getNoteNumber(), message.getFloatVelocity(), metadata.samplePosition);
        else if (message.isNoteOff())
            engine.noteOff(message.getNoteNumber(), metadata.samplePosition);
    }
}

void JGKAcousticAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int mode = (int) apvts.getRawParameterValue(IDs::mode)->load();

    if (mode == 2)
    {
        if (!heldAutoChord.empty())
            releaseSelectedChord();
        handleManualMidi(midi);
        engine.process(buffer, numSamples);
        transportWasPlaying = false;
        return;
    }

    double bpm = 120.0;
    bool isPlaying = false;
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            isPlaying = pos->getIsPlaying();
            if (auto b = pos->getBpm())
                bpm = juce::jlimit(20.0, 400.0, *b);
        }
    }

    if (isPlaying && !transportWasPlaying)
    {
        currentStep = 0;
        samplesUntilNextStep = 0.0;
    }

    if (!isPlaying && transportWasPlaying)
    {
        releaseSelectedChord();
        currentStep = 0;
        samplesUntilNextStep = 0.0;
    }

    if (isPlaying)
    {
        const auto pattern = getPattern((int) apvts.getRawParameterValue(IDs::pattern)->load());
        const double quarter = currentSampleRate * 60.0 / bpm;
        const double stepSamples = quarter * (4.0 / (double) pattern.divisions);

        samplesUntilNextStep -= (double) numSamples;
        while (samplesUntilNextStep <= 0.0)
        {
            const int action = pattern.steps[(size_t) currentStep];
            if (action >= 0)
                triggerSelectedChord(action == 1, action == 2, action == 2 ? 0.82f : 1.0f);

            currentStep = (currentStep + 1) % pattern.length;
            samplesUntilNextStep += stepSamples;
        }
    }
    else
    {
        syncEngineParameters(false, false);
    }

    engine.process(buffer, numSamples);
    transportWasPlaying = isPlaying;
}

void JGKAcousticAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void JGKAcousticAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* JGKAcousticAudioProcessor::createEditor()
{
    return new JGKAcousticAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JGKAcousticAudioProcessor();
}
