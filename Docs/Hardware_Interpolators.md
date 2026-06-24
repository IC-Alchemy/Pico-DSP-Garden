https://people.ece.cornell.edu/land/courses/ece4760/RP2040/C_SDK_interpolator/index_interpolator.html


The rp2040 has two fairly specialized interpolator hardware modules (interp) for each M0 core. The advantage of using the interp is that each unit can perform an add, shift, bit-mask, and a second add on each cpu clock cycle. Data paths are set up by configuring registers. The setup requires bending your brain to figure out the intent of the interp design which seems to be motivated by repetitive table look-up and interpolations. There are two special configurations for the interpolators. Interp0 can be put in blend mode and interp1 can be put into clamp mode. The examples below use both modes, plus the basic table look-up capabilities of the interpolators.

Weighted average of two audio signals using Blend Mode.
This example cross-fades between two audio waveforms in realtime to show a very basic use of the interpolator blend mode. Interpolator0 is set up in default blend mode, except signed arithmetic is turned on. Two DDS units synthesize sine waves of settable frequency and phase. The interpolator adds the two sine waves, weighted by the value of α, which sets the blend according to
outn = sine_sample0n + α * (sine_sample1n - sine_sample0n)
at time n.
The image below shows sine0 set to 200 Hz and sine1 set to 600 Hz, phase zero, and α set to add 3/4 sine0 with 1/4 of sine1.
These parameters should produce a two Fourier component approximation of a square wave. The top trace is sine0.
The bottom trace is the blended waveform.

# Adding a simple amplitude envelope using an interpolator
Most sounds are recognized both by their spectral content and by their time course. The FM generator described above was modified to use another interp lane to produce a simple decaying amplitude envelope. The envelope is set to some maximum value by the C code by initializing an accumulaor. A constant in a base register is subtracted on each synthesis sample until the envelope amplitude is zero. The basic waveform equation for this process is:
output wave = (amp_envelope(t)) * sin(Fout*t + fm_depth*(sin(Fmod*t)))

The setup code configures three lanes.
Interpolator0-lane1 (FM modulation frequency) setup to:
-- add accum1 + base1 amd store in accum0 (result1 to accum1) (add raw -- no shift)
-- right-shift accum1 23 bits and mask to bits 8:1 (zero low bit for short pointer)
-- add shifted/masked accum0 to base2 and read result2 as table position output to interp1

Interpolator0-lane0 (AM modulation amplitude) setup to:
-- add accum0 + base0 amd store in accum0 (result0 to accum0) (add raw -- no shift)
but note that base0 contains a negative number.
-- the shift/mask options were set to always output zero, so that the lane1 calculations are not affected.

Interpolator1-lane1 (main oscillator) setup to:
-- add accum1 + base1 amd store in accum0 (result1 to accum1) (add raw -- no shift)
-- right-shift accum1 23 bits and mask to bits 8:1 (zero low bit for short pointer)
-- add shifted/masked accum0 to base2 and read result2 as table position output to PWM

For a main frequency of 200 Hz, modulation frequency of 330 Hz,
fm_depth of 16 and decay rate of 100 we get the following waveform.
You can see the linear decay and the odd distortion due to the FM modulation.
It would be possible to add one more amplitude modulation (perhaps rise time)
using the interpolators, but the C overhead makes it less desirable. A more general system
is described below with the DDS units in the interpolators and amplitude envelope in C.


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