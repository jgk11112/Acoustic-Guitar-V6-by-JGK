#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class JGKAcousticAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit JGKAcousticAudioProcessorEditor(JGKAcousticAudioProcessor&);
    ~JGKAcousticAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    void timerCallback() override { repaint(); }
    void setIntParam(const char* id, int value);
    void setFloatParamFromY(const char* id, float yNorm);
    int getInt(const char* id) const;
    float getFloat(const char* id) const;
    juce::Rectangle<float> scaledRect(float x, float y, float w, float h) const;
    void drawSelection(juce::Graphics&, juce::Rectangle<float> r, bool selected) const;
    juce::String rootName(int r) const;
    juce::String typeName(int t) const;
    juce::String patternName(int p) const;

    JGKAcousticAudioProcessor& processor;
    juce::Image skin;
    int draggingKnob = -1;
    float dragStartY = 0.0f;
    float dragStartValue = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JGKAcousticAudioProcessorEditor)
};
