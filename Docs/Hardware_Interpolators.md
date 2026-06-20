
## Use Case Ideas
###West Coast Wavefolding and Nonlinear Distortion

In analog synthesizer architectures, wavefolding is used to add rich harmonic complexity to simple waveforms (such as sine or triangle waves) by folding the peaks of the signal back on themselves when they exceed a specific voltage threshold. Implementing a multi-stage wavefolder in digital software typically requires conditional branching, absolute value calculations, and mathematical reflections, which can be computationally expensive.

The SIO interpolator can implement a high-speed digital wavefolder in hardware using its shift, mask, and signed-extension logic4:

Signal Ingestion: The input audio signal is represented as a signed 32-bit integer and loaded into the interpolator's accumulator10.
Peak Isolation: Lane 0 is configured with a custom bitmask that isolates the upper bits of the signal (representing the threshold boundaries)10.
Folding Execution: When the input signal exceeds the mask boundary, the isolated upper bits trigger an automatic inversion of the lower bits, folding the waveform back toward zero.
Offset Addition: The raw folded value is added to BASE0 to shift the folded peaks, creating complex, multi-stage fold patterns10.
Because the masking, shifting, and addition occur entirely within the single-cycle SIO block, this hardware-accelerated wavefolder can process high-rate audio buffers with no conditional branching penalties, enabling rich, West Coast-style synthesis with minimal CPU load4.

### Granular Synthesis and Multi-Voice Grain Windowing
Granular synthesis generates complex soundscapes by splitting an audio sample into hundreds of tiny acoustic "grains" (typically 10 to 100 ms in duration), each with its own playback speed, volume envelope, and spatial position. Generating and mixing dozens of overlapping grains in real time requires substantial computational power.

The SIO interpolators can accelerate granular synthesis by automating grain envelope generation and sample playback:

Playback Pointer Tracking: INTERP0 tracks the fractional playback head of an individual grain, using Lane 0 to determine the memory address index and Lane 1 to interpolate between adjacent sample points10.
Envelope Generation: INTERP1 is configured to generate the grain's volume envelope (such as a Hann or triangular window). Lane 0 tracks the envelope phase, while Lane 1 interpolates between points in a window lookup table10.
Single-Cycle Gain Application: The interpolated sample value from INTERP0 and the envelope window value from INTERP1 are loaded into BASE0 and BASE1 of INTERP010. The interpolator performs a single-cycle multiplication, outputting a fully windowed, high-fidelity audio grain4.
Using this multi-lane coordination, a single RP2350 core can manage dozens of concurrent audio grains, enabling rich, real-time granular textures4.