#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    inline float satTanh(float x) { return std::tanh(x); }

    inline float satHardClip(float x)
    {
        return juce::jlimit(-1.0f, 1.0f, x);
    }

    inline float satCubicSoftClip(float x)
    {
        const float a = std::abs(x);
        if (a <= 1.0f)
            return x - (x * x * x) / 3.0f;
        return (x > 0.0f ? 2.0f/3.0f : -2.0f/3.0f);
    }

    inline float satAtan(float x)
    {
        return (2.0f / juce::MathConstants<float>::pi) * std::atan(x);
    }

    inline float satRational(float x)
    {
        return x / (1.0f + std::abs(x));
    }

    inline float satExponential(float x)
    {
        const float a = std::abs(x);
        const float y = 1.0f - std::exp(-a);
        return std::copysign(y, x);
    }

    inline float satAsymTanh(float x)
    {
        constexpr float k = 1.2f;
        constexpr float b = 0.15f;

        const float y  = std::tanh(k * (x + b));
        const float y0 = std::tanh(k * b);

        float z = y - y0;

        const float norm = 1.0f - std::abs(y0);
        if (norm > 1.0e-5f)
            z /= norm;

        return juce::jlimit(-1.0f, 1.0f, z);
    }

    inline float lerp(float a, float b, float m)
    {
        return a + m * (b - a);
    }

    enum class SatType : int
    {
        Tanh = 0,
        HardClip,
        CubicSoftClip,
        Atan,
        Rational,
        Exponential,
        AsymTanh
    };

    inline float applySat(SatType t, float x)
    {
        switch (t)
        {
            case SatType::Tanh:          return satTanh(x);
            case SatType::HardClip:      return satHardClip(x);
            case SatType::CubicSoftClip: return satCubicSoftClip(x);
            case SatType::Atan:          return satAtan(x);
            case SatType::Rational:      return satRational(x);
            case SatType::Exponential:   return satExponential(x);
            case SatType::AsymTanh:      return satAsymTanh(x);
            default:                     return satTanh(x);
        }
    }
}


juce::AudioProcessorValueTreeState::ParameterLayout
SatuMorpherAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"drive", 1},
        "Drive",
        juce::NormalisableRange<float>(0.0f, 36.0f, 0.01f),
        6.0f,
        "dB"
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"morph", 1},
        "Morph",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.5f
    ));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"leftType", 1},
        "Left Type",
        juce::StringArray{
            "tanh",
            "hard clip",
            "cubic soft clip",
            "atan",
            "rational",
            "exponential",
            "asym tanh"
        },
        0
    ));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"rightType", 1},
        "Right Type",
        juce::StringArray{
            "tanh",
            "hard clip",
            "cubic soft clip",
            "atan",
            "rational",
            "exponential",
            "asym tanh"
        },
        6 // asym tanh by default
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"output", 1},
        "Output",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f),
        0.0f,
        "dB"
    ));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"oversampleMode", 1},
        "Oversampling",
        juce::StringArray{"Off", "x2", "x4"},
        0
    ));

    return { params.begin(), params.end() };
}

SatuMorpherAudioProcessor::SatuMorpherAudioProcessor()
: AudioProcessor(BusesProperties()
                 .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                 .withOutput("Output", juce::AudioChannelSet::stereo(), true))
, apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
}

void SatuMorpherAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    auto coeff = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0);
    for (auto& f : dcBlock)
        f.coefficients = coeff;

    oversampling2x = std::make_unique<juce::dsp::Oversampling<float>>(
        2,
        1, // 2^1 = 2x
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR
    );
    oversampling2x->reset();
    oversampling2x->initProcessing((size_t) samplesPerBlock);

    oversampling4x = std::make_unique<juce::dsp::Oversampling<float>>(
        2,
        2, // 2^2 = 4x
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR
    );
    oversampling4x->reset();
    oversampling4x->initProcessing((size_t) samplesPerBlock);
}
void SatuMorpherAudioProcessor::releaseResources() {}

bool SatuMorpherAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    return (in == out) && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

static void processSaturationBlock(
    juce::dsp::AudioBlock<float>& block,
    float drive,
    float morph,
    SatType leftType,
    SatType rightType)
{
    const float makeup = 1.0f / std::sqrt(drive);

    const int numCh = (int) block.getNumChannels();
    const int numSm = (int) block.getNumSamples();

    for (int ch = 0; ch < numCh; ++ch)
    {
        float* data = block.getChannelPointer((size_t) ch);
        for (int i = 0; i < numSm; ++i)
        {
            const float x = data[i] * drive;
            const float a = applySat(leftType, x);
            const float b = applySat(rightType, x);
            data[i] = lerp(a, b, morph) * makeup;
        }
    }
}

void SatuMorpherAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int totalCh = buffer.getNumChannels();
    const int procCh  = juce::jmin(2, totalCh);
    if (procCh <= 0)
        return;

    juce::ScopedNoDenormals noDenormals;

    const float driveDb = apvts.getRawParameterValue("drive")->load();
    const float drive   = juce::Decibels::decibelsToGain(driveDb);

    float morph = apvts.getRawParameterValue("morph")->load();
    morph = juce::jlimit(0.0f, 1.0f, morph);

    const float outDb   = apvts.getRawParameterValue("output")->load();
    const float outGain = juce::Decibels::decibelsToGain(outDb);

    const int leftIdx  = juce::jlimit(0, 6, (int) apvts.getRawParameterValue("leftType")->load());
    const int rightIdx = juce::jlimit(0, 6, (int) apvts.getRawParameterValue("rightType")->load());

    const auto leftType  = (SatType) leftIdx;
    const auto rightType = (SatType) rightIdx;

    const int osMode = juce::jlimit(0, 2, (int) apvts.getRawParameterValue("oversampleMode")->load());

    auto fullBlock = juce::dsp::AudioBlock<float>(buffer);
    auto block     = fullBlock.getSubsetChannelBlock(0, (size_t) procCh);

    if (osMode == 1 && oversampling2x)
    {
        auto osBlock = oversampling2x->processSamplesUp(block);
        processSaturationBlock(osBlock, drive, morph, leftType, rightType);
        oversampling2x->processSamplesDown(block);
    }
    else if (osMode == 2 && oversampling4x)
    {
        auto osBlock = oversampling4x->processSamplesUp(block);
        processSaturationBlock(osBlock, drive, morph, leftType, rightType);
        oversampling4x->processSamplesDown(block);
    }
    else
    {
        processSaturationBlock(block, drive, morph, leftType, rightType);
    }

    // DC-block + Output gain (в обычной частоте)
    for (int ch = 0; ch < procCh; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float y = dcBlock[(size_t) ch].processSample(data[i]);
            data[i] = y * outGain;
        }
    }
}

juce::AudioProcessorEditor* SatuMorpherAudioProcessor::createEditor()
{
    return new SatuMorpherAudioProcessorEditor(*this);
}

void SatuMorpherAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SatuMorpherAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}