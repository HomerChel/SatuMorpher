#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LampChoice.h"


class SatuMorpherAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SatuMorpherAudioProcessorEditor (SatuMorpherAudioProcessor&);
    ~SatuMorpherAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    SatuMorpherAudioProcessor& audioProcessor;

    juce::Image woodImage;
    juce::Image logoImage;

    juce::Slider driveSlider;
    juce::Label  driveLabel;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> driveAttachment;

    juce::Slider morphSlider;
    juce::Label  morphLabel;
    std::unique_ptr<Attachment> morphAttachment;

    juce::Label leftTypeLabel;
    juce::Label rightTypeLabel;

    LampChoice leftLamp;
    LampChoice rightLamp;

    std::atomic<float>* leftTypeParam  = nullptr;
    std::atomic<float>* rightTypeParam = nullptr;

    juce::Slider mixSlider;
    juce::Label  mixLabel;
    std::unique_ptr<Attachment> mixAttachment;

    juce::Slider outputSlider;
    juce::Label  outputLabel;
    std::unique_ptr<Attachment> outputAttachment;

    juce::ComboBox oversampleBox;
    juce::Label oversampleLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oversampleAttachment;

    juce::ImageButton logoButton;
    void showAbout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SatuMorpherAudioProcessorEditor)
};