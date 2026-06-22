# DSP Paper Shortlist for RP2350 Music DSP

This note maps DSP literature to the current library surface: naive saw/pulse/triangle/sine/noise oscillators, second-order B-spline saw/pulse/hard-sync oscillator variants, one-pole and biquad filters, TPT state-variable filter, cubic-capable delay-line effects, Schroeder-style reverb, tanh waveshaping, ADSR/envelope/compressor stages, Karplus-Strong, zero-crossing and YIN pitch detection, and RMS/peak metering.

Target constraints used for screening:

- RP2350-class microcontroller.
- 48 kHz stereo, 32-sample blocks.
- C++17, float processing.
- No heap allocation, locks, I/O, or unbounded loops in `process`.
- Prefer fixed state, compile-time memory, coefficient precomputation, and measurable worst-case cost.

## Best Implementation Bets

### 1. Antialiased Classic Oscillators

Current modules: `SawOscillator`, `PulseOscillator`, `TriangleOscillator`, `SecondOrderBSplineSawOscillator`, `SecondOrderBSplinePulseOscillator`, and `SecondOrderBSplineHardSyncSawOscillator`.

The current saw and pulse are direct discontinuous waveforms, so they will alias heavily at 48 kHz. The best quality-per-cost upgrade is to add antialiased oscillator variants while preserving the existing simple classes.

| Priority | Source | Implementation idea | Fit |
|---|---|---|---|
| High | Tim Stilson and Julius O. Smith, "Alias-Free Digital Synthesis of Classic Analog Waveforms," ICMC 1996. <https://quod.lib.umich.edu/i/icmc/bbp2372.1996.101> | Use BLIT/BLEP as the reference model for discontinuity correction. | Strong theory baseline, but full BLIT/table paths may be heavier than needed for RP2350. |
| High | Vesa Valimaki and Antti Huovilainen, "Antialiasing Oscillators in Subtractive Synthesis," IEEE Signal Processing Magazine, 2007. DOI: <https://doi.org/10.1109/MSP.2007.323276> | Use the paper as a decision map across BLEP, PolyBLEP, DPW, and wavetable approaches. | Most useful design guide for this library. PolyBLEP saw/pulse likely gives the best embedded tradeoff. |
| High | Vesa Valimaki, "Discrete-Time Synthesis of the Sawtooth Waveform with Reduced Aliasing," IEEE Signal Processing Letters, 2005. DOI: <https://doi.org/10.1109/LSP.2004.842271> | Add DPW saw as a very cheap alternative: parabolic waveform, differentiate, frequency normalization. | Very small state and few operations per sample. Good `CheapAntialiasedSaw` candidate. |
| Implemented | Second-order B-spline oscillators (current implementation). | Band-limited oscillators using B-spline kernel smoothing: treat waveforms as integrals of impulses, smear each impulse across 3 samples using quadratic B-spline kernel, then integrate. | Currently implemented as `SecondOrderBSplineSawOscillator`, `SecondOrderBSplinePulseOscillator`, and `HardSyncSaw`. Similar antialiasing quality to PolyBLEP with different tradeoffs: fixed 3-tap kernel vs. per-edge correction, simpler impulse scheduling vs. piecewise polynomial corrections. |
| Medium | Vesa Valimaki, Ju Nam, Julius O. Smith, and Jonathan S. Abel, "Alias-Suppressed Oscillators Based on Differentiated Polynomial Waveforms," IEEE TASLP, 2010. DOI: <https://doi.org/10.1109/TASL.2009.2026507> | Add selectable DPW2/DPW3/DPW4 quality modes with constexpr coefficients. | Gives an explicit quality/CPU ladder without dynamic memory. |
| Medium | Jari Kleimola and Vesa Valimaki, "Reducing Aliasing from Synthetic Audio Signals Using Polynomial Transition Regions," IEEE Signal Processing Letters, 2012. DOI: <https://doi.org/10.1109/LSP.2011.2177819> | Correct discontinuities locally around phase wraps and pulse-width edges. | Useful for PWM and future hard-sync where edge events are known. |

Recommended next change: compare the current B-spline oscillator variants against `PolyBlepSawOscillator`, `PolyBlepPulseOscillator`, and `DpwSawOscillator` candidates. Keep the current naive oscillators for tests, LFOs, and educational examples.

### 2. Filter Stability and Modulation Quality

Current modules: `OnePoleLowpass`, `BiquadLowpass`, `StateVariableFilter`.

The current TPT SVF is the right direction for modulated synth filters. The direct-form biquad is fine for static utility filtering, but coefficient changes and high-frequency tuning need care.

| Priority | Source | Implementation idea | Fit |
|---|---|---|---|
| High | Vadim Zavalishin, "Preserving the LTI System Topology in s- to z-Plane Transforms," 2008. <https://www.native-instruments.com/fileadmin/ni_media/downloads/pdf/KeepTopology.pdf> | Keep filters in topology-preserving/trapezoidal forms when parameters move. | Maps directly to the existing SVF and improves cutoff/resonance modulation behavior. |
| High | Vadim Zavalishin, "The Art of VA Filter Design." <https://www.native-instruments.com/fileadmin/ni_media/downloads/pdf/VAFilterDesign_2.1.2.pdf> | Standardize one-pole and SVF forms around TPT integrators; add coefficient smoothing rules. | Practical music-DSP reference for stable virtual-analog filters. |
| Medium | Sophocles J. Orfanidis, "Digital Parametric Equalizer Design with Prescribed Nyquist-Frequency Gain," JAES, 1997. <https://secure.aes.org/forum/pubs/journal/?elib=7854> | Add decramped biquad coefficient options for high cutoff or EQ-like future filters. | Same runtime structure as a biquad; improves response near Nyquist. |
| Medium | Antti Huovilainen, "Non-Linear Digital Implementation of the Moog Ladder Filter," DAFx 2004. <https://www.dafx.de/paper-archive/details/UervgvkeeDC1a4sWDluM4Q> | Optional nonlinear 4-pole ladder with approximated `tanh` and voice-count budgeting. | Good musical filter, but make it opt-in because nonlinearities and oversampling can consume CPU. |

Recommended first change: do not replace the current biquad. Add coefficient smoothing and tests for modulated SVF behavior first. Treat the Moog ladder as a separate optional module after benchmarking.

### 3. Fractional Delay, Chorus, Flanger, and String Tuning

Current modules: `DelayLine::readLinear`, `DelayLine::readCubic`, `Delay`, `Chorus`, `KarplusStrongVoice`.

Cubic interpolation is now available through `DelayLine::readCubic()` and is used by `Delay`, `Chorus`, and `examples/modulated_space.cpp`. Linear interpolation remains the cheapest option, while future allpass or shared fractional-delay helpers could further improve string tuning, phasing, and low-memory flanging.

| Priority | Source | Implementation idea | Fit |
|---|---|---|---|
| High | T. I. Laakso, Vesa Valimaki, Matti Karjalainen, and U. K. Laine, "Splitting the Unit Delay: Tools for Fractional Delay Filter Design," IEEE Signal Processing Magazine, 1996. DOI: <https://doi.org/10.1109/79.482137> | Keep the existing linear and cubic Lagrange-style reads, then evaluate first-order allpass or a named fractional-delay wrapper. | Small static state; direct upgrade for chorus and string tuning. |
| High | Jon Dattorro, "Effect Design, Part 2: Delay-Line Modulation and Chorus," JAES, 1997. <https://secure.aes.org/forum/pubs/journal/?elib=10159> | Implement chorus/flanger as modulated delay taps with interpolation, feedback, and smoothing. | Directly relevant to the current `Chorus` and future flanger. |
| Medium | Julius O. Smith, "An Allpass Approach to Digital Phasing and Flanging," ICMC 1984. <https://quod.lib.umich.edu/i/icmc/bbp2372.1984.014> | Add allpass-based phaser/flanger alternative using cascaded allpass sections. | Tiny memory footprint and useful when delay RAM is constrained. |

Recommended next change: benchmark `readCubic()` against `readLinear()` in chorus, delay, and Karplus-Strong patches, then decide whether a named `FractionalDelay` helper or allpass option is worth the extra API surface.

### 4. Reverb: Damping, Diffusion, and Better Tails

Current modules: `CombFilter`, `AllpassFilter`, `SchroederReverb`.

The current reverb is a reasonable starter, but it has no per-comb damping and no early reflection structure. The cheapest audible improvement is Moorer/Dattorro-style damping and taps before attempting a full FDN.

| Priority | Source | Implementation idea | Fit |
|---|---|---|---|
| High | Manfred R. Schroeder, "Natural Sounding Artificial Reverberation," AES convention paper, 1961. <https://aes2.org/publications/elibrary-page/?id=343> | Use comb/allpass delay lengths and diffusion stages deliberately, not arbitrary values. | Foundational low-cost topology already close to the current implementation. |
| High | James A. Moorer, "About This Reverberation Business," Computer Music Journal, 1979. DOI: <https://doi.org/10.2307/3680280> | Add early reflection taps and one-pole lowpass damping in comb feedback paths. | High audible value for very little CPU. |
| High | Jon Dattorro, "Effect Design, Part 1: Reverberator and Other Filters," JAES, 1997. <https://secure.aes.org/forum/pubs/journal/?elib=10160> | Add a compact plate-like preset with input diffusion, tank delays, damping, and optional slow modulation. | Fixed-memory structures designed for realtime musical DSP. |
| Medium | Sebastian J. Schlecht and Emanuël A. P. Habets, "Accurate Reverberation Time Control in Feedback Delay Networks," DAFx 2017. <https://www.sebastianjiroschlecht.com/publication/schlecht-2017-va/> | If adding an FDN, design attenuation filters around target `T60`, not ad hoc feedback coefficients. | More advanced than current reverb; useful once the library grows beyond Schroeder/Moorer. |

Recommended first change: add damping inside each feedback comb and a small fixed early-reflection tap bank. Delay a full FDN until target benchmarks exist.

### 5. Waveshaper Aliasing and Nonlinear Cost

Current module: `Waveshaper` using `std::tanh`.

The present waveshaper is simple but aliases and pays a transcendental function cost. Both quality and efficiency can improve.

| Priority | Source | Implementation idea | Fit |
|---|---|---|---|
| High | Stefan Bilbao, Fabian Esqueda, Julian D. Parker, and Vesa Valimaki, "Antiderivative Antialiasing for Memoryless Nonlinearities," IEEE Signal Processing Letters, 2017. DOI: <https://doi.org/10.1109/LSP.2017.2675541> | Add optional antiderivative antialiasing for memoryless shaping functions. | Avoids jumping straight to high oversampling factors. |
| Medium | Julian D. Parker, Vadim Zavalishin, and Efflam Le Bivic, "Reducing the Aliasing of Nonlinear Waveshaping Using Continuous-Time Convolution," DAFx 2016. <https://www.dafx.de/paper-archive/details/vem_XXF5qBbfiWOH2RVVAA> | Use continuous-time convolution or low-order oversampling for stronger drive modes. | Good quality reference, but benchmark carefully on RP2350. |
| Medium | Marc Le Brun, "Digital Waveshaping Synthesis," JAES, 1979. <https://secure.aes.org/forum/pubs/journal/?elib=3212> | Use polynomial/Chebyshev shaping options where harmonic intent is explicit. | Can replace some `tanh` use with cheaper polynomial shapers. |

Recommended first change: benchmark `tanhf` against a rational or table approximation, then add an opt-in antialiased waveshaper mode.

### 6. Dynamics and Metering

Current modules: `EnvelopeFollower`, `CompressorStaticCurve`, `CompressorDetector`, `GainReductionSmoother`, `Compressor`, and `RmsPeakMeter`.

The compressor is now split into detector, static curve, gain-reduction smoothing, and gain application. It has soft-knee support through `CompressorStaticCurve`; remaining opportunities are stereo linking, sidechain input, and reducing expensive dB/gain conversion work when profiling shows it matters.

| Priority | Source | Implementation idea | Fit |
|---|---|---|---|
| High | Dimitrios Giannoulis, Michael Massberg, and Joshua D. Reiss, "Digital Dynamic Range Compressor Design: A Tutorial and Analysis," JAES, 2012. <https://aes2.org/publications/elibrary-page/?id=16354> | Refactor compressor into detector, static curve, gain smoothing, and gain application. Add soft knee and feed-forward mode. | Best single source for making the compressor predictable and testable. |
| Medium | Fred Floru, "Attack and Release Time Constants in RMS-Based Feedback Compressors," AES convention paper, 1998. <https://www.thatcorp.com/datashts/AES4703_Attack_and_Release_Time_Constants_I.pdf> | Calibrate attack/release coefficient meanings and avoid unnecessary `sqrtf`/`logf` per sample. | Helps both sound and CPU budget. |

Recommended next change: add stereo-linked and sidechain-friendly APIs after the mono behavior is stable, then profile whether log/gain conversion needs control-rate batching.

### 7. Pitch Detection

Current modules: `ZeroCrossingPitchDetector` and `YinPitchDetector<WindowSize, MaxTau>`.

Zero-crossing is cheap but weak for real musical signals. `YinPitchDetector` provides the static-buffer accurate path and runs at a configurable update interval so pitch tracking can coexist with realtime audio.

| Priority | Source | Implementation idea | Fit |
|---|---|---|---|
| High | Alain de Cheveigne and Hideki Kawahara, "YIN, a Fundamental Frequency Estimator for Speech and Music," JASA, 2002. DOI: <https://doi.org/10.1121/1.1458024> | Add YIN difference function, cumulative mean normalized difference, thresholding, and parabolic interpolation over a bounded lag range. | Much more robust than zero crossing; CPU can be bounded by update cadence and lag limits. |
| Medium | Philip McLeod and Geoff Wyvill, "A Smarter Way to Find Pitch," ICMC 2005. <https://quod.lib.umich.edu/i/icmc/bbp2372.2005.107> | Implement normalized square difference function with clarity/confidence output. | Good for musical pitch tracking and rejecting noisy inputs. |

Recommended next change: tune YIN window sizes and update cadence against representative instruments, then consider MPM/NSDF if musical confidence reporting needs to improve.

## Suggested Implementation Order

1. Compare the current B-spline oscillators with PolyBLEP saw/pulse and DPW saw candidates.
2. Benchmark cubic delay interpolation in chorus, delay, and Karplus-Strong patches, then decide whether to add a named fractional-delay helper or allpass option.
3. Add reverb damping inside comb feedback paths and a small early-reflection tap bank.
4. Add stereo-linked and sidechain-friendly compressor APIs after the mono staged compressor is fully characterized.
5. Tune YIN windows/update cadence against representative instruments.
6. Benchmark `tanh` and add an opt-in antialiased or approximated waveshaper.
7. Consider optional Moog ladder and FDN modules only after target-cycle benchmarks exist.

## API Search Notes

No-key API lookups were used as metadata checks:

- Crossref confirmed DOI metadata for "Antialiasing Oscillators in Subtractive Synthesis," "Fractional Delay Filters--Design and Applications," and "YIN, a Fundamental Frequency Estimator for Speech and Music."
- Semantic Scholar's no-key endpoint returned HTTP 429 during parallel probing, so it was not used as an authoritative source in this note.
- Source links above prefer DOI, publisher, author, university, AES, DAFx, Aalto, or ICMC archive pages over general web summaries.
