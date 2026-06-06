RP2350 Interpolators: A Deep Dive for Audio DSP

The RP2350 inherited and expanded one of the most unusual features from the RP2040: two hardware interpolators per core. With two cores, that's four interpolator blocks total, each capable of performing address generation, bit extraction, masking, shifting, accumulation, and blending operations in hardware. Raspberry Pi explicitly calls out audio acceleration as one of the intended uses. 

The interpolators are easy to underestimate. They are not floating-point units. They are not DSP MAC engines. They are more like tiny configurable arithmetic co-processors sitting beside the CPU.

Think of them as:

> "Hardware that continuously computes 'the next value' while your CPU does other work."



That turns out to be surprisingly useful for audio.


---

What an interpolator actually does

Each interpolator contains:

Accumulators

Shift units

Masks

Adders

Cross-input options

Special blend mode

Special clamp mode


Conceptually:

phase_accumulator
        |
     shift
        |
      mask
        |
      add
        |
     result

You preload configuration once.

Then each read can produce:

base + ((accumulator >> shift) & mask)

in essentially a single operation. 

This is why Raspberry Pi lists:

table lookup generation

quantization

dithering

decompression

affine mapping

audio operations


among the intended applications. 


---

IN-THE-BOX AUDIO USES

Let's start with obvious DSP applications.


---

1. Wavetable Oscillators

This is arguably the killer application.

A wavetable oscillator normally does:

phase += increment;

index = phase >> 20;
sample = wavetable[index];

Every sample.

The interpolator can generate the wavetable address automatically.

Accumulator = phase
Shift = fractional bits
Mask = table size
Base = wavetable start

Now each read produces:

wavetable + index

without needing separate shift/mask/add instructions.

For dozens of oscillators:

32 oscillators × 48kHz

the savings add up.


---

2. Linear Interpolation

Suppose:

sample0
sample1
fraction

Need:

sample0 + frac*(sample1-sample0)

The interpolator's blend mode was literally designed for this sort of thing.

You can use it to produce weighted blends between neighboring table entries. This is useful for:

wavetable oscillators

delay lines

sample playback

granular synthesis


Instead of:

a*(1-f)+b*f

being done entirely in software, part of the address and fraction handling moves into hardware.


---

3. Fractional Delay Lines

Many audio effects need non-integer delays:

chorus

flanger

vibrato

physical modeling


Example:

delay = 123.456 samples

You need:

buffer[123]
buffer[124]

plus interpolation.

The interpolator can continuously generate the fractional read pointer.

This reduces pointer arithmetic inside the audio loop.


---

4. Sample Rate Conversion

Suppose:

44.1kHz -> 48kHz

You maintain a fractional phase:

phase += 44100/48000

and interpolate between samples.

The interpolator naturally fits:

phase accumulator

plus

integer index extraction

plus

fraction extraction

in hardware.

This is one of the strongest practical uses.


---

5. Envelope Generators

ADSRs often look like:

value += increment

every sample.

The interpolator can serve as a hardware ramp generator.

One interpolator could continuously generate:

attack
decay
release

curves represented as fixed-point accumulators.

Not a huge win for one envelope.

Potentially a meaningful win for hundreds.


---

6. Sequencers

For a synth:

step_index = phase >> N

The interpolator can turn a phase accumulator directly into:

sequence position

or

clock division

with almost no software overhead.


---

FILTER APPLICATIONS

At first glance filters seem less suitable because filters need multiply-accumulates.

But there are still opportunities.


---

7. Coefficient Lookup

Suppose a state-variable filter has:

cutoff
resonance

mapped into a coefficient table.

The interpolator can generate:

coefficient addresses

for rapidly changing filter parameters.

This reduces parameter management overhead.


---

8. Morphing Filters

Imagine:

LP -> BP -> HP

continuous morphing.

Interpolator blend mode can help generate intermediate coefficients between filter states.

Not replacing DSP math.

Reducing interpolation cost.


---

9. Fast Nonlinear Filters

For analog-model filters:

tanh(x)

or

saturation(x)

often comes from lookup tables.

Interpolator hardware can:

generate lookup addresses

generate interpolation fractions


very efficiently.


---

SEMI-CREATIVE USES

Now we're leaving conventional territory.


---

10. Massive Polyphony Phase Engine

Imagine:

128 oscillators

Each oscillator:

phase += frequency

Every sample.

Instead of thinking of the interpolator as an address generator:

Think of it as a dedicated phase-generation unit.

One interpolator could manage:

voice phase

while the CPU only handles table fetches.

This can reduce register pressure significantly.


---

11. Hardware-Controlled Modulation Matrix

Modulation systems are mostly:

destination += amount * source

But many modulation sources are ramps.

LFOs. Envelopes. Sequencer clocks.

Interpolators can generate these evolving control values independently from the DSP core.

Almost like tiny modulation coprocessors.


---

12. Wavefolding Lookup Engine

Wavefolders often use:

piecewise nonlinear curves

stored in tables.

Interpolator hardware can:

extract regions

generate offsets

interpolate transitions


while CPU focuses on audio summation.


---

OUT-OF-THE-BOX IDEAS

Now for the weird stuff.


---

13. Audio Cellular Automata

Treat an interpolator accumulator as the state variable of a cellular automaton.

Each sample:

shift
mask
feedback

creates evolving bit patterns.

Feed those into oscillators.

You get:

glitch textures

pseudo-chaotic modulation

evolving noise fields


with almost no CPU.


---

14. Fractal Oscillators

The interpolator naturally performs:

shift
mask
add
feedback

operations.

Those are exactly the ingredients used in many fractal and recursive generators.

Imagine:

phase
→ bit extraction
→ recursive accumulation

producing self-similar waveforms.

Not conventional synthesis, but potentially very interesting.


---

15. Hardware Granular Scheduler

Granular synthesis spends surprising CPU time deciding:

which grain
where
when

An interpolator could continuously generate:

grain positions

grain indices

grain start offsets


freeing CPU for interpolation and mixing.


---

16. Probabilistic Music Generator

Use accumulators and masks as deterministic pseudo-random state machines.

Instead of generating audio directly:

Generate:

note events

modulation events

rhythmic structures


from hardware-generated evolving bitfields.

Think of the interpolator as a tiny compositional engine.


---

17. Audio Compression/Expansion Engine

The datasheet specifically mentions decompression. 

A synth storing:

wavetables

impulse responses

multisamples


could use interpolators to accelerate decoding and address reconstruction.

This is an underexplored area for RP2350 audio.


---

The Most Valuable Uses

If I were writing a serious synthesizer for RP2350, I'd prioritize:

Rank	Use

1	Wavetable oscillator addressing
2	Fractional delay interpolation
3	Sample-rate conversion
4	Nonlinear lookup tables (tanh/saturation)
5	Envelope and LFO generation
6	Filter coefficient interpolation
7	Granular synthesis scheduling


The biggest win is not raw arithmetic speed.

The biggest win is eliminating repetitive bookkeeping:

phase extraction

masking

shifting

address generation

fraction generation


Those operations occur millions of times per second in a synth. The interpolators turn many of them into hardware reads, leaving the Cortex-M33's DSP instructions and FPU free for the actual audio math.

For an RP2350 synthesizer with dozens of oscillators, wavetable morphing, delay effects, and modulation sources, the interpolators can become a surprisingly important part of the architecture rather than just a minor optimization.