#pragma once
#include <JuceHeader.h>
#include "GuitarEngine.h"
#include <array>
#include <vector>

class JGKAcousticAudioProcessor : public juce::AudioProcessor
{
public:
    JGKAcousticAudioProcessor();
    ~JGKAcousticAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Acoustic Guitar by JGK"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    struct Pattern
    {
        std::array<int, 16> steps {};
        int divisions = 8;
        int length = 8;
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    Pattern getPattern(int index) const;
    std::vector<int> makeChordNotes(int root, int type) const;
    void syncEngineParameters(bool mutedStep = false, bool up = false);
    void triggerSelectedChord(bool up, bool muted, float intensity);
    void releaseSelectedChord();
    void handleManualMidi(const juce::MidiBuffer& midi);

    GuitarEngine engine;
    double currentSampleRate = 44100.0;
    double samplesUntilNextStep = 0.0;
    int currentStep = 0;
    bool transportWasPlaying = false;
    std::vector<int> heldAutoChord;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JGKAcousticAudioProcessor)
};
