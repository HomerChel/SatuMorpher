#include "PluginEditor.h"

namespace
{
    class AboutComponent : public juce::Component
    {
    public:
        AboutComponent()
            : site ("sintezafx.com", juce::URL ("https://sintezafx.com"))
            , email ("alex@sintezafx.com", juce::URL ("mailto:alex@sintezafx.com"))
        {
            line1.setText ("Created out of boredom and interest", juce::dontSendNotification);
            line1.setJustificationType (juce::Justification::centred);
            addAndMakeVisible (line1);

            line2.setText ("Send your tracks made with my plugin to", juce::dontSendNotification);
            line2.setJustificationType (juce::Justification::centred);
            addAndMakeVisible (line2);

            addAndMakeVisible (email);

            line3.setText ("and I'll feature them on the plugin page", juce::dontSendNotification);
            line3.setJustificationType (juce::Justification::centred);
            addAndMakeVisible (line3);

            addAndMakeVisible (site);
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (14);

            site.setBounds (r.removeFromTop (22));
            r.removeFromTop (10);

            line1.setBounds (r.removeFromTop (20));
            r.removeFromTop (10);

            line2.setBounds (r.removeFromTop (20));
            email.setBounds (r.removeFromTop (22));
            line3.setBounds (r.removeFromTop (20));
        }

    private:
        juce::Label line1, line2, line3;
        juce::HyperlinkButton site;
        juce::HyperlinkButton email;
    };
} // namespace


SatuMorpherAudioProcessorEditor::SatuMorpherAudioProcessorEditor (SatuMorpherAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize(680, 320);

    woodImage = juce::ImageCache::getFromMemory(BinaryData::wood_png, BinaryData::wood_pngSize);
    logoImage = juce::ImageCache::getFromMemory(BinaryData::logo_png, BinaryData::logo_pngSize);

    logoButton.setImages (false, true, true,
                          logoImage, 1.0f, juce::Colours::transparentBlack,
                          logoImage, 1.0f, juce::Colours::transparentBlack,
                          logoImage, 1.0f, juce::Colours::transparentBlack);

    logoButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    logoButton.setTooltip ("About");
    logoButton.onClick = [this] { showAbout(); };

    addAndMakeVisible (logoButton);

    driveSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    driveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 24);
    addAndMakeVisible(driveSlider);

    driveLabel.setText("Drive (dB)", juce::dontSendNotification);
    driveLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(driveLabel);

    driveAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "drive", driveSlider);

    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 24);
    mixSlider.setTextValueSuffix(" %");
    addAndMakeVisible(mixSlider);

    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(mixLabel);

    mixAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "mix", mixSlider);

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
    
    // Bottom row: Drive | Mix | Output
    const int gap = 12;
    auto bot = botRow.reduced(0, 0);

    const int thirdW = (bot.getWidth() - 2 * gap) / 3;

    auto driveArea = bot.removeFromLeft(thirdW);
    bot.removeFromLeft(gap);
    auto mixArea = bot.removeFromLeft(thirdW);
    bot.removeFromLeft(gap);
    auto outArea = bot;

    driveLabel.setBounds(driveArea.removeFromTop(22));
    driveSlider.setBounds(driveArea.reduced(10, 6));

    mixLabel.setBounds(mixArea.removeFromTop(22));
    mixSlider.setBounds(mixArea.reduced(10, 6));

    outputLabel.setBounds(outArea.removeFromTop(22));
    outputSlider.setBounds(outArea.reduced(10, 6));

    // Oversampling selector (bottom-left)
    const int pad = 16;
    const int h   = 24;
    const int y   = getHeight() - pad - h;

    oversampleLabel.setBounds(pad, y, 24, h);
    oversampleBox.setBounds(pad + 30, y, 80, h);

    const int logoPad = 10;
    const int logoH = 33;
    const int logoW = 120;

    logoButton.setBounds (getWidth() - logoPad - logoW,
                        getHeight() - logoPad - logoH,
                        logoW, logoH);

}

void SatuMorpherAudioProcessorEditor::timerCallback()
{
    if (leftTypeParam)
        leftLamp.setSelectedIndex((int)leftTypeParam->load());

    if (rightTypeParam)
        rightLamp.setSelectedIndex((int)rightTypeParam->load());
}

void SatuMorpherAudioProcessorEditor::showAbout()
{
    auto content = std::make_unique<AboutComponent>();
    content->setSize (420, 160);

    juce::CallOutBox::launchAsynchronously (std::move (content),
                                           logoButton.getScreenBounds(),
                                           nullptr);
}
