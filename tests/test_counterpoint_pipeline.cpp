#include "../Examples/Shared/HarmonyExampleUtils.h"

#include "doctest.h"

TEST_CASE("Cantus generator starts and ends on tonic with one climax") {
    HarmonyEngine::Key key(0, HarmonyEngine::ScaleType::Major);

    for (std::uint32_t seed = 1; seed <= 100; ++seed) {
        HarmonyEngine::Voice cantus = harmony_examples::generateCantusFirmus(key, seed);

        REQUIRE(cantus.notes.size() >= 8);
        CHECK(cantus.notes.front().midiNumber % 12 == key.tonicNote);
        CHECK(cantus.notes.back().midiNumber % 12 == key.tonicNote);
        CHECK(harmony_examples::hasSingleClimax(cantus));
        CHECK(cantus.isValid());
    }
}

TEST_CASE("Counterpoint pipeline builds playable fixed-buffer plans") {
    const HarmonyEngine::ScaleType modes[] = {
        HarmonyEngine::ScaleType::Major,
        HarmonyEngine::ScaleType::Dorian,
        HarmonyEngine::ScaleType::Phrygian,
        HarmonyEngine::ScaleType::Lydian,
        HarmonyEngine::ScaleType::Mixolydian,
        HarmonyEngine::ScaleType::Aeolian,
    };

    for (int species = 0; species < 4; ++species) {
        harmony_examples::CounterpointSettings settings;
        settings.speciesZone = species;
        settings.strict = species == 0;
        settings.key = HarmonyEngine::Key(species % 12, modes[species % 6]);
        settings.seed = static_cast<std::uint32_t>(10 + species);

        harmony_examples::PlanBuffer plan;
        harmony_examples::CounterpointReport report;
        REQUIRE(harmony_examples::buildCounterpointPlan(settings, plan, &report));

        CHECK(plan.voiceCount >= 2);
        CHECK(plan.voiceCount <= 3);
        CHECK(plan.totalTicks > 0);
        CHECK(plan.voices[0].eventCount >= 8);
        CHECK(report.voices.size() == plan.voiceCount);

        for (std::size_t voice = 0; voice < plan.voiceCount; ++voice) {
            for (std::size_t i = 0; i < plan.voices[voice].eventCount; ++i) {
                const auto& event = plan.voices[voice].events[i];
                CHECK(event.midi >= 36);
                CHECK(event.midi <= 96);
                CHECK(event.lenTicks > 0);
                CHECK(event.startTick < plan.totalTicks);
            }
        }
    }
}
