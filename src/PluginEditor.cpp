#include "PluginEditor.h"

SatuMorpherAudioProcessorEditor::SatuMorpherAudioProcessorEditor (SatuMorpherAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize(680, 320);

    woodImage = juce::ImageCache::getFromMemory(BinaryData::wood_png, BinaryData::wood_pngSize);
    logoImage = juce::ImageCache::getFromMemory(BinaryData::logo_png, BinaryData::logo_pngSize);


    driveSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    driveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 24);
    addAndMakeVisible(driveSlider);

    driveLabel.setText("Drive (dB)", juce::dontSendNotification);
    driveLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(driveLabel);

    driveAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "drive", driveSlider);

    outputSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    outputSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 24);
    addAndMakeVisible(outputSlider);

    outputLabel.setText("Output (dB)", juce::dontSendNotification);
    outputLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outputLabel);

    outputAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "output", outputSlider);

    morphSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    morphSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 24);
    addAndMakeVisible(morphSlider);

    morphLabel.setText("Morph", juce::dontSendNotification);
    morphLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(morphLabel);

    morphAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "morph", morphSlider);

    leftTypeLabel.setText("Left", juce::dontSendNotification);
    leftTypeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(leftTypeLabel);

    rightTypeLabel.setText("Right", juce::dontSendNotification);
    rightTypeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(rightTypeLabel);

    juce::StringArray types{
        "tanh",
        "hard clip",
        "cubic soft clip",
        "atan",
        "rational",
        "exponential",
        "asym tanh"
    };

    leftLamp.setItems(types);
    rightLamp.setItems(types);
    rightLamp.setLampOnRight(true);

    addAndMakeVisible(leftLamp);
    addAndMakeVisible(rightLamp);

    // указатели на параметры (choice хранится как float-индекс)
    leftTypeParam  = audioProcessor.apvts.getRawParameterValue("leftType");
    rightTypeParam = audioProcessor.apvts.getRawParameterValue("rightType");

    // выставим initial selection из параметров
    leftLamp.setSelectedIndex((int)leftTypeParam->load());
    rightLamp.setSelectedIndex((int)rightTypeParam->load());

    // при клике — выставляем параметр через APVTS, чтобы хост видел автоматизацию/undo
    auto* leftParamObj  = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("leftType"));
    auto* rightParamObj = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("rightType"));

    leftLamp.setOnSelect([leftParamObj](int idx)
    {
        if (leftParamObj == nullptr) return;
        const float norm = leftParamObj->convertTo0to1((float)idx);
        leftParamObj->setValueNotifyingHost(norm);
    });

    rightLamp.setOnSelect([rightParamObj](int idx)
    {
        if (rightParamObj == nullptr) return;
        const float norm = rightParamObj->convertTo0to1((float)idx);
        rightParamObj->setValueNotifyingHost(norm);
    });

    oversampleLabel.setText("OS", juce::dontSendNotification);
    oversampleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(oversampleLabel);

    oversampleBox.addItem("Off", 1);
    oversampleBox.addItem("x2", 2);
    oversampleBox.addItem("x4", 3);
    addAndMakeVisible(oversampleBox);

    oversampleAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            audioProcessor.apvts, "oversampleMode", oversampleBox
        );

    startTimerHz(30);
}

void SatuMorpherAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (woodImage.isValid())
    {
        g.drawImageWithin(woodImage, 0, 0, getWidth(), getHeight(),
                        juce::RectanglePlacement::fillDestination);
    }
    else
    {
        g.fillAll(juce::Colours::black);
    }

    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillRect(getLocalBounds());

    if (logoImage.isValid())
    {
        const int pad = 10;

        const int logoW = 130;
        const int logoH = (int) std::round(logoW * ((double) logoImage.getHeight() / (double) logoImage.getWidth()));

        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);

        const int x = getWidth()  - pad - logoW;
        const int y = getHeight() - pad - logoH;

        g.setOpacity(0.9f);
        g.drawImageWithin(logoImage, x, y, logoW, logoH,
                        juce::RectanglePlacement::fillDestination);
        g.setOpacity(1.0f);
    }

    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawFittedText ("SatuMorpher", getLocalBounds().removeFromTop(28), juce::Justification::centred, 1);
}

void SatuMorpherAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(16);

    auto header = area.removeFromTop(28);

    auto content = area;

    const int sideW = 180;
    auto leftCol  = content.removeFromLeft(sideW);
    auto rightCol = content.removeFromRight(sideW);
    auto centerCol = content;

    // --- Left selector
    leftTypeLabel.setBounds(leftCol.removeFromTop(22));
    leftLamp.setBounds(leftCol.removeFromTop(172).reduced(0, 4));

    // --- Right selector
    rightTypeLabel.setBounds(rightCol.removeFromTop(22));
    rightLamp.setBounds(rightCol.removeFromTop(172).reduced(0, 4));

    // --- Center: Morph on top, Drive + Output below side-by-side
    auto topRow = centerCol.removeFromTop(centerCol.getHeight() / 2);
    auto botRow = centerCol;

    // Top row: Morph (full width)
    morphLabel.setBounds(topRow.removeFromTop(22));
    morphSlider.setBounds(topRow.reduced(10, 6));

    // Bottom row: split into two
    auto botLeft  = botRow.removeFromLeft(botRow.getWidth() / 2);
    auto botRight = botRow;

    driveLabel.setBounds(botLeft.removeFromTop(22));
    driveSlider.setBounds(botLeft.reduced(10, 6));

    outputLabel.setBounds(botRight.removeFromTop(22));
    outputSlider.setBounds(botRight.reduced(10, 6));

    const int pad = 16;
    oversampleLabel.setBounds(pad, getHeight() - 28, 24, 20);
    oversampleBox.setBounds(pad + 26, getHeight() - 30, 70, 24);
}

void SatuMorpherAudioProcessorEditor::timerCallback()
{
    if (leftTypeParam)
        leftLamp.setSelectedIndex((int)leftTypeParam->load());

    if (rightTypeParam)
        rightLamp.setSelectedIndex((int)rightTypeParam->load());
}

