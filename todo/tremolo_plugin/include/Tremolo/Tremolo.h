#pragma once

namespace tremolo {
enum class ApplySmoothing { no, yes };

class Tremolo {
public:
  enum class LfoWaveform : size_t {
    sine = 0,
    triangle = 1,
  };

  Tremolo() {
    for (auto& lfo : lfos) {
      lfo.setFrequency(5.f /* Hz */, true);
    }
  }

  void prepare(double sampleRate, int expectedMaxFramesPerBlock) {
    const juce::dsp::ProcessSpec processSpec {
      .sampleRate = sampleRate,
      .maximumBlockSize = static_cast<juce::uint32>(expectedMaxFramesPerBlock),
      .numChannels = 1u,
    };

    for (auto& lfo : lfos) {
      lfo.prepare(processSpec);
    }
  }

  void setLfoWaveform(LfoWaveform waveform) {
    jassert(waveform == LfoWaveform::sine || waveform == LfoWaveform::triangle);

    lfoToSet = waveform;

  }

  void process(juce::AudioBuffer<float>& buffer) noexcept {
    // actual updating of the LFO waveform happens in process()
    // to keep setLfoWaveform() idempotent
    updateLfoWaveform();

    // for each frame
    for (const auto frameIndex : std::views::iota(0, buffer.getNumSamples())) {
      const auto lfoValue = getNextLfoValue();
      constexpr auto modulationDepth = 0.4f;
      const auto modulationValue = modulationDepth * lfoValue + 1.f;

      // for each channel sample in the frame
      for (const auto channelIndex :
           std::views::iota(0, buffer.getNumChannels())) {
        // get the input sample
        const auto inputSample = buffer.getSample(channelIndex, frameIndex);

        const auto outputSample = (1.f - modulationDepth) + (modulationDepth * (0.5f * (lfoValue + 1)));

        // set the output sample
        buffer.setSample(channelIndex, frameIndex, outputSample);
      }
    }
  }

  void reset() noexcept {
    for (auto& lfo : lfos) {
      lfo.reset();
    }
  }

private:
  // You should put class members and private functions here
  static float triangleFromPhase(float phase) {
    const auto ft = phase / juce::MathConstants<float>::twoPi;
    return 4.f * std::abs(ft - std::floor(ft + 0.5f)) - 1.f;
  }

  std::array<juce::dsp::Oscillator<float>, 2u> lfos{
    juce::dsp::Oscillator<float>{[](auto phase){ return std::sin(phase); }},
    juce::dsp::Oscillator<float>{[](auto phase){ return triangleFromPhase(phase); }}
  };

  LfoWaveform currentLfo = LfoWaveform::triangle;
  LfoWaveform lfoToSet = currentLfo;

  float getNextLfoValue() {
    // 0 to get just the generated value otherwise it would be added to the input
    return lfos[juce::toUnderlyingType(currentLfo)].processSample(0.f);
  }

  void updateLfoWaveform() {
    if (currentLfo != lfoToSet) {
      currentLfo = lfoToSet;
    }
  }
};
}  // namespace tremolo
