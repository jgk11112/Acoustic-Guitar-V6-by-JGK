#include "PluginEditor.h"
#include <JGKAssets.h>

namespace {
    constexpr float W = 1536.0f, H = 1024.0f;
    struct KnobDef { float x,y,w,h; const char* id; };
    const KnobDef knobs[] = {
        {117, 529, 116, 120, "strumSpeed"}, {274, 529, 116, 120, "humanize"},
        {430, 529, 116, 120, "palm"},       {585, 529, 116, 120, "dynamics"},
        {743, 529, 116, 120, "tone"},       {896, 529, 116, 120, "room"},
        {1051,529, 116, 120, "tight"},      {1211,529, 116, 120, "output"}
    };
}

JGKAcousticAudioProcessorEditor::JGKAcousticAudioProcessorEditor(JGKAcousticAudioProcessor& p)
: AudioProcessorEditor(&p), processor(p)
{
    skin = juce::ImageFileFormat::loadFrom(JGKAssets::plugin_skin_png, JGKAssets::plugin_skin_pngSize);
    setResizable(true, true);
    setResizeLimits(900, 600, 1800, 1200);
    setSize(1229, 819);
    startTimerHz(20);
}

juce::Rectangle<float> JGKAcousticAudioProcessorEditor::scaledRect(float x,float y,float w,float h) const
{
    const float sx = getWidth()/W, sy = getHeight()/H;
    return {x*sx,y*sy,w*sx,h*sy};
}

void JGKAcousticAudioProcessorEditor::paint(juce::Graphics& g)
{
    if (skin.isValid()) g.drawImage(skin, getLocalBounds().toFloat());
    else g.fillAll(juce::Colours::black);

    const int root = getInt("root"), type = getInt("type"), pattern = getInt("pattern"), mode = getInt("mode"), capo = getInt("capo");

    // Warm glow around active root/type buttons while preserving the original picture.
    const float rootX[12] = {70,132,196,261,323,385,70,132,196,261,323,385};
    const float rootY[12] = {178,178,178,178,178,178,226,226,226,226,226,226};
    for(int i=0;i<12;++i) drawSelection(g, scaledRect(rootX[i], rootY[i], 52, 34), i==root);

    const float typeX[8] = {476,544,614,682,476,544,614,682};
    const float typeY[8] = {178,178,178,178,226,226,226,226};
    for(int i=0;i<8;++i) drawSelection(g, scaledRect(typeX[i], typeY[i], 58, 34), i==type);

    // Dynamic text overlays: the concept artwork is static, these show the real engine state.
    g.setColour(juce::Colour::fromRGB(238,194,100));
    g.setFont(juce::FontOptions(22.0f * getWidth()/W, juce::Font::bold));
    g.drawFittedText(rootName(root) + " " + typeName(type), scaledRect(766, 180, 220, 36).toNearestInt(), juce::Justification::centred, 1);
    g.setFont(juce::FontOptions(17.0f * getWidth()/W, juce::Font::bold));
    g.drawFittedText(capo == 0 ? "OFF" : juce::String(capo), scaledRect(1293, 185, 90, 34).toNearestInt(), juce::Justification::centred, 1);
    g.drawFittedText(patternName(pattern), scaledRect(107, 753, 260, 36).toNearestInt(), juce::Justification::centred, 1);

    const char* modeNames[] = {"STRUM","FINGERPICK","MIDI"};
    for(int i=0;i<3;++i)
    {
        auto r = scaledRect(407 + i*242, 668, 220, 52);
        drawSelection(g, r, i==mode);
        if(i==mode)
        {
            g.setColour(juce::Colour::fromRGB(255,218,139));
            g.setFont(juce::FontOptions(15.0f*getWidth()/W, juce::Font::bold));
            g.drawText(modeNames[i], r, juce::Justification::centred);
        }
    }
}

void JGKAcousticAudioProcessorEditor::drawSelection(juce::Graphics& g, juce::Rectangle<float> r, bool selected) const
{
    if (!selected) return;
    g.setColour(juce::Colour::fromRGBA(240,184,70,52));
    g.fillRoundedRectangle(r, 5.0f);
    g.setColour(juce::Colour::fromRGBA(255,210,100,210));
    g.drawRoundedRectangle(r.reduced(1.0f), 5.0f, 1.7f);
}

void JGKAcousticAudioProcessorEditor::resized() {}

void JGKAcousticAudioProcessorEditor::setIntParam(const char* id, int value)
{
    if(auto* p = processor.apvts.getParameter(id))
    {
        p->beginChangeGesture();
        auto& rp = dynamic_cast<juce::RangedAudioParameter&>(*p);
        p->setValueNotifyingHost(rp.convertTo0to1((float)value));
        p->endChangeGesture();
    }
}

void JGKAcousticAudioProcessorEditor::setFloatParamFromY(const char* id, float yNorm)
{
    if(auto* p = processor.apvts.getParameter(id))
    {
        auto& rp = dynamic_cast<juce::RangedAudioParameter&>(*p);
        p->setValueNotifyingHost(juce::jlimit(0.0f,1.0f,yNorm));
    }
}

int JGKAcousticAudioProcessorEditor::getInt(const char* id) const
{
    return (int) processor.apvts.getRawParameterValue(id)->load();
}
float JGKAcousticAudioProcessorEditor::getFloat(const char* id) const
{
    return processor.apvts.getRawParameterValue(id)->load();
}

void JGKAcousticAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    const float x = e.position.x * W/getWidth(), y = e.position.y * H/getHeight();

    const float rootX[12] = {70,132,196,261,323,385,70,132,196,261,323,385};
    const float rootY[12] = {178,178,178,178,178,178,226,226,226,226,226,226};
    for(int i=0;i<12;++i) if(juce::Rectangle<float>(rootX[i],rootY[i],52,34).contains(x,y)) { setIntParam("root",i); repaint(); return; }

    const float typeX[8] = {476,544,614,682,476,544,614,682};
    const float typeY[8] = {178,178,178,178,226,226,226,226};
    for(int i=0;i<8;++i) if(juce::Rectangle<float>(typeX[i],typeY[i],58,34).contains(x,y)) { setIntParam("type",i); repaint(); return; }

    if (juce::Rectangle<float>(1260,160,60,70).contains(x,y)) { setIntParam("capo", juce::jmax(0,getInt("capo")-1)); return; }
    if (juce::Rectangle<float>(1375,160,60,70).contains(x,y)) { setIntParam("capo", juce::jmin(12,getInt("capo")+1)); return; }

    for(int i=0;i<3;++i) if(juce::Rectangle<float>(407+i*242,668,220,52).contains(x,y)) { setIntParam("mode",i); repaint(); return; }

    // Pattern list approx positions from the concept art.
    for(int i=0;i<8;++i) if(juce::Rectangle<float>(75,791+i*24,300,23).contains(x,y)) { setIntParam("pattern",i); repaint(); return; }

    for(int i=0;i<8;++i)
    {
        if(juce::Rectangle<float>(knobs[i].x,knobs[i].y,knobs[i].w,knobs[i].h).contains(x,y))
        {
            draggingKnob = i;
            dragStartY = e.position.y;
            if (juce::String(knobs[i].id) == "output")
                dragStartValue = processor.apvts.getParameter(knobs[i].id)->getValue();
            else dragStartValue = getFloat(knobs[i].id);
            processor.apvts.getParameter(knobs[i].id)->beginChangeGesture();
            return;
        }
    }
}

void JGKAcousticAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if(draggingKnob < 0) return;
    const float delta = (dragStartY - e.position.y) / juce::jmax(120.0f, getHeight()*0.28f);
    const float norm = juce::jlimit(0.0f,1.0f,dragStartValue + delta);
    setFloatParamFromY(knobs[draggingKnob].id, norm);
    repaint();
}

void JGKAcousticAudioProcessorEditor::mouseUp(const juce::MouseEvent&)
{
    if(draggingKnob >= 0)
    {
        processor.apvts.getParameter(knobs[draggingKnob].id)->endChangeGesture();
        draggingKnob = -1;
    }
}

juce::String JGKAcousticAudioProcessorEditor::rootName(int r) const
{
    static const char* n[] = {"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"};
    return n[juce::jlimit(0,11,r)];
}
juce::String JGKAcousticAudioProcessorEditor::typeName(int t) const
{
    static const char* n[] = {"MAJOR","MINOR","7","MAJ7","MIN7","SUS2","SUS4","ADD9"};
    return n[juce::jlimit(0,7,t)];
}
juce::String JGKAcousticAudioProcessorEditor::patternName(int p) const
{
    static const char* n[] = {"Straight 8ths","Pop Acoustic","Singer-Songwriter","Slow Ballad","Driving Acoustic","Palm Muted","Folk","16th Strum"};
    return n[juce::jlimit(0,7,p)];
}
