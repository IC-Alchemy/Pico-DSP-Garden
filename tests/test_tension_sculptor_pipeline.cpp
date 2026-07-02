#include "../Examples/Shared/HarmonyExampleUtils.h"

#include "doctest.h"

TEST_CASE("Tension Sculptor pipeline produces bounded fixed-buffer events") {
    harmony_examples::PlanBuffer plan;
    harmony_examples::TensionSculptorReport report;
    harmony_examples::TensionSculptorSettings settings;
    settings.tensionTarget = 0.62f;
    settings.complexity = 0.7f;
    settings.harmonicRhythm = 0.5f;
    settings.registerSpread = 0.6f;
    settings.presetIndex = 0;

    REQUIRE(harmony_examples::buildTensionSculptorPlan(settings, plan, &report));
    CHECK(plan.voiceCount >= 2);
    CHECK(plan.voiceCount <= harmony_examples::kMaxPlanVoices);
    CHECK(plan.totalTicks > 0);
    CHECK(plan.tensionCount > 0);

    for (std::size_t i = 0; i < plan.tensionCount; ++i) {
        CHECK(plan.tension[i] >= 0.0f);
        CHECK(plan.tension[i] <= 1.0f);
    }

    for (std::size_t voice = 0; voice < plan.voiceCount; ++voice) {
        CHECK(plan.voices[voice].eventCount > 0);
        CHECK(plan.voices[voice].eventCount <= harmony_examples::kMaxPlanEvents);
        for (std::size_t i = 0; i < plan.voices[voice].eventCount; ++i) {
            const auto& event = plan.voices[voice].events[i];
            CHECK(event.midi >= 24);
            CHECK(event.midi <= 108);
            CHECK(event.velocity >= 0.0f);
            CHECK(event.velocity <= 1.0f);
            CHECK(event.startTick < plan.totalTicks);
            CHECK(event.lenTicks > 0);
            CHECK(event.tension >= 0.0f);
            CHECK(event.tension <= 1.0f);
        }
    }

    CHECK(report.curve.isValid());
    CHECK_FALSE(report.voicing.hasVoiceCrossing);
}
