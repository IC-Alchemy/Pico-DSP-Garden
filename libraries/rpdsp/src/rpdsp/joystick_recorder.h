#pragma once

#include <array>
#include <cstddef>

namespace rpdsp {

// Fixed-buffer joystick motion recorder/playback helper. Callers decide the
// sampling rate and coordinate mapping; this class owns only the capture loop.
template <size_t MaxPositions>
class JoystickRecorder {
 public:
  // One sampled joystick point.
  struct Position {
    float x;
    float y;
  };

  void reset() {
    recording_ = false;
    playing_ = false;
    length_ = 0;
    index_ = 0;
    current_ = {0.0f, 0.0f};
  }

  void startRecording() {
    // Recording replaces the previous loop and cancels playback until stopped.
    recording_ = true;
    playing_ = false;
    length_ = 0;
    index_ = 0;
  }

  void stopRecording(bool autoPlay = true) {
    recording_ = false;
    index_ = 0;
    // Empty recordings stay idle; non-empty recordings can begin looping immediately.
    playing_ = autoPlay && length_ > 0;
  }

  void startPlayback() {
    if (length_ > 0) {
      playing_ = true;
      index_ = 0;
    }
  }

  void stopPlayback() { playing_ = false; }

  void record(float x, float y) {
    if (!recording_)
      return;

    if (length_ >= MaxPositions) {
      // A full buffer commits the captured loop and switches to playback by default.
      stopRecording(true);
      return;
    }

    positions_[length_] = {x, y};
    current_ = positions_[length_];
    ++length_;
  }

  Position processPlayback() {
    if (playing_ && length_ > 0) {
      current_ = positions_[index_];
      ++index_;
      // Playback is circular so callers can poll this once per control/audio tick.
      if (index_ >= length_)
        index_ = 0;
    }
    return current_;
  }

  [[nodiscard]] bool isRecording() const { return recording_; }
  [[nodiscard]] bool isPlaying() const { return playing_; }
  [[nodiscard]] size_t length() const { return length_; }
  [[nodiscard]] size_t index() const { return index_; }

 private:
  std::array<Position, MaxPositions> positions_{};
  Position current_ = {0.0f, 0.0f};
  bool recording_ = false;
  bool playing_ = false;
  size_t length_ = 0;
  size_t index_ = 0;
};

}  // namespace rpdsp
