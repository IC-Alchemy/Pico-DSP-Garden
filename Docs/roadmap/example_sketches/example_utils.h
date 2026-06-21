// Provides small host-only helpers for rendering example processors to stereo 16-bit WAV files.

#pragma once

#include "rp2350_audio_dsp/dsp/algorithm.h"
#include "rp2350_audio_dsp/dsp/audio_block.h"
#include "rp2350_audio_dsp/dsp/config.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>

namespace example {

constexpr float kSampleRate = rpdsp::kDefaultSampleRate;
constexpr size_t kBlockSize = rpdsp::kDefaultBlockSize;

inline std::int16_t floatToInt16(float sample) {
  const float clipped = rpdsp::clamp(sample, -1.0f, 1.0f);
  return static_cast<std::int16_t>(clipped * 32767.0f);
}

class WavWriter {
 public:
  bool open(const char* path, std::uint32_t sampleRate) {
    stream_.open(path, std::ios::binary);
    if (!stream_) {
      return false;
    }
    sampleRate_ = sampleRate;
    dataBytes_ = 0;
    writeHeader(0);
    return true;
  }

  void writeSample(float left, float right) {
    const std::int16_t l = floatToInt16(left);
    const std::int16_t r = floatToInt16(right);
    stream_.write(reinterpret_cast<const char*>(&l), sizeof(l));
    stream_.write(reinterpret_cast<const char*>(&r), sizeof(r));
    dataBytes_ += 4;
  }

  bool close() {
    if (!stream_) {
      return false;
    }
    // Rewrite the header once the final data byte count is known.
    stream_.seekp(0, std::ios::beg);
    writeHeader(dataBytes_);
    stream_.close();
    return true;
  }

 private:
  void writeU16(std::uint16_t value) {
    stream_.put(static_cast<char>(value & 0xffu));
    stream_.put(static_cast<char>((value >> 8u) & 0xffu));
  }

  void writeU32(std::uint32_t value) {
    stream_.put(static_cast<char>(value & 0xffu));
    stream_.put(static_cast<char>((value >> 8u) & 0xffu));
    stream_.put(static_cast<char>((value >> 16u) & 0xffu));
    stream_.put(static_cast<char>((value >> 24u) & 0xffu));
  }

  void writeHeader(std::uint32_t dataBytes) {
    // Little-endian stereo PCM16 WAV header.
    stream_.write("RIFF", 4);
    writeU32(36u + dataBytes);
    stream_.write("WAVE", 4);
    stream_.write("fmt ", 4);
    writeU32(16);
    writeU16(1);
    writeU16(2);
    writeU32(sampleRate_);
    writeU32(sampleRate_ * 4u);
    writeU16(4);
    writeU16(16);
    stream_.write("data", 4);
    writeU32(dataBytes);
  }

  std::ofstream stream_;
  std::uint32_t sampleRate_ = 48000;
  std::uint32_t dataBytes_ = 0;
};

template <typename Processor>
bool renderToWav(Processor& processor, const char* path, float seconds) {
  WavWriter wav;
  if (!wav.open(path, static_cast<std::uint32_t>(kSampleRate))) {
    return false;
  }

  rpdsp::DefaultAudioBlock block;
  const size_t totalFrames = static_cast<size_t>(seconds * kSampleRate);
  size_t written = 0;
  while (written < totalFrames) {
    const size_t frames = std::min<size_t>(rpdsp::DefaultAudioBlock::kCapacity, totalFrames - written);
    block.setFrameCount(frames);
    processor.process(block);
    for (size_t i = 0; i < block.frames(); ++i) {
      wav.writeSample(block.left()[i], block.right()[i]);
    }
    written += frames;
  }

  return wav.close();
}

}  // namespace example
