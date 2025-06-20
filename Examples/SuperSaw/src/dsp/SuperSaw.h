#pragma once
#ifndef DSY_HYPERSAW_H
#define DSY_HYPERSAW_H

#include <cmath>
#include <cstdlib>

namespace daisysp
{
/** @brief Hypersaw Oscillator based on the Roland JP-8000 Super Saw.
 * @author Ben Sergent, based on research by Adam Szabo.
 * @date June 2024
 *
 * This class implements a hypersaw oscillator as detailed in Adam Szabo's paper,
 * "How to Emulate the Super Saw". It consists of seven sawtooth oscillators:
 * one central oscillator and six detuned oscillators.
 *
 * The implementation includes:
 * - Non-linear detuning curve for authentic frequency spreading.
 * - Unique mix control that adjusts the balance between the center and side oscillators.
 * - A pitch-tracked high-pass filter to replicate the original's tonal character.
 * - Free-running oscillators with randomized phase on trigger for a dynamic, evolving sound.
 */
class Hypersaw
{
  public:
    Hypersaw() {}
    ~Hypersaw() {}

    /** Initializes the Hypersaw module.
     * * @param sample_rate The audio sample rate in Hz.
     */
    void Init(float sample_rate)
    {
        sample_rate_ = sample_rate;
        freq_         = 100.f; // Default frequency
        detune_       = 0.5f;
        mix_          = 0.5f;

        for(int i = 0; i < 7; i++)
        {
            oscs_[i].Init(sample_rate_);
            oscs_[i].SetWaveform(Oscillator::WAVE_SAW);
        }

        hpf_.Init(sample_rate_);
        hpf_.SetRes(0.1f); // Low resonance as it's for spectral shaping
        hpf_.SetDrive(0.f);

        Trigger(); // Set initial random phases
        UpdateCoefficients();
    }

    /** Processes the oscillator and returns the next sample.
     *
     * @return The next floating point sample of the oscillator.
     */
    float Process()
    {
        float sum = 0.f;

        // Process side oscillators
        float side_sum = 0.f;
        side_sum += oscs_[0].Process();
        side_sum += oscs_[1].Process();
        side_sum += oscs_[2].Process();
        side_sum += oscs_[4].Process();
        side_sum += oscs_[5].Process();
        side_sum += oscs_[6].Process();

        // Add side oscillators with their gain
        sum += side_sum * side_gain_;
        
        // Add center oscillator with its gain
        sum += oscs_[3].Process() * center_gain_;

        return hpf_.Process(sum / 4.5f);
    }

    /** Sets the fundamental frequency of the oscillator.
     * * @param freq The frequency in Hz.
     */
    void SetFreq(float freq)
    {
        freq_ = freq;
        UpdateCoefficients();
    }

    /** Sets the detune amount.
     *
     * The detune parameter is passed through a non-linear curve
     * to approximate the behavior of the original hardware.
     * * @param detune A value from 0.0 to 1.0.
     */
    void SetDetune(float detune)
    {
        detune_ = fclamp(detune, 0.f, 1.f);
        UpdateCoefficients();
    }

    /** Sets the mix between the center and side oscillators.
     *
     * As mix increases, the side oscillators get louder and the center
     * oscillator gets quieter, following specific curves from the paper.
     *
     * @param mix A value from 0.0 to 1.0.
     */
    void SetMix(float mix)
    {
        mix_ = fclamp(mix, 0.f, 1.f);
        UpdateCoefficients();
    }
    /** Sets the base waveform for all seven oscillators simultaneously.
     *
     * @param waveform The desired waveform (WAVE_SAW or WAVE_RAMP).
     */
    void SetAllWaveforms(Waveform waveform)
    {
        uint8_t dsy_wave = (waveform == WAVE_SAW) ? Oscillator::WAVE_SAW : Oscillator::WAVE_RAMP;
        for (int i = 0; i < 7; i++)
        {
            oscs_[i].SetWaveform(dsy_wave);
        }
    }

    /** Sets the base waveform for a single specified oscillator.
     *
     * @param index The index of the oscillator to modify (0-6).
     * @param waveform The desired waveform (WAVE_SAW or WAVE_RAMP).
     */
    void SetWaveform(int index, Waveform waveform)
    {
        if (index >= 0 && index < 7)
        {
            uint8_t dsy_wave = (waveform == WAVE_SAW) ? Oscillator::WAVE_SAW : Oscillator::WAVE_RAMP;
            oscs_[index].SetWaveform(dsy_wave);
        }
    }


    /** Simulates a new note trigger by randomizing the phase of each oscillator.
     * This creates the "free-running" characteristic of the original.
     */
    void Trigger()
    {
        for(int i = 0; i < 7; i++)
        {
            oscs_[i].SetPhase(static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
        }
    }


  private:
    /** Updates the internal frequencies and gains based on the public parameters.
     */
    void UpdateCoefficients()
    {
        // 1. Calculate gains from the mix parameter
        // Center oscillator gain (linear decrease)
        center_gain_ = -0.55366f * mix_ + 0.99785f;
        
        // Side oscillators gain (parabolic curve)
        side_gain_ = -0.73764f * mix_ * mix_ + 1.2841f * mix_ + 0.044372f;


        // 2. Calculate the non-linear detune amount
        // This uses Horner's method to evaluate the 11th-degree polynomial
        // found in the research paper. It's more efficient than using std::pow.
        float x = detune_;
        float scaled_detune = x * (0.6717417634f 
                                + x * (-24.1878824391f 
                                + x * (404.270393888f 
                                + x * (-3425.0836591318f 
                                + x * (17019.9518580080f 
                                + x * (-53046.9642751875f 
                                + x * (106649.679158292f 
                                + x * (-138150.6761080548f 
                                + x * (111363.4808729368f 
                                + x * (-50818.8652045924f 
                                + x * 10028.7312891634f))))))))));
        scaled_detune += 0.0030115596f;
        scaled_detune = fclamp(scaled_detune, 0.f, 1.f);

        // 3. Set frequencies for all oscillators
        oscs_[3].SetFreq(freq_); // Center oscillator

        for(int i = 0; i < 6; i++)
        {
            int osc_idx = (i < 3) ? i : i + 1; // Skip center osc index 3
            float detune_factor = 1.0f + scaled_detune * detune_ratios_[i];
            oscs_[osc_idx].SetFreq(freq_ * detune_factor);
        }

        // 4. Set the high-pass filter cutoff to track the fundamental frequency
        hpf_.SetFreq(freq_);
    }

    // Array of 7 sawtooth oscillators
    Oscillator oscs_[7];
    // Pitch-tracked high-pass filter
    Svf        hpf_;

    float sample_rate_;
    float freq_;
    float detune_;
    float mix_;

    float side_gain_;
    float center_gain_;
    
    // Detune ratios for the 6 side oscillators, derived from the paper.
    // Values are for osc 1, 2, 3, 5, 6, 7 relative to the center osc (4).
    static constexpr float detune_ratios_[6] = {
        -0.11002313f, -0.06288439f, -0.01952356f,
         0.01991221f,  0.06216538f,  0.10745242f
    };
};

} // namespace daisysp

#endif // DSY_HYPERSAW_H
