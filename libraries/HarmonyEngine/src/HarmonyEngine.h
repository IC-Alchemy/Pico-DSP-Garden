// HarmonyEngine.h - Arduino library discovery shim
//
// This file exists solely so arduino-cli's library resolver can detect the
// HarmonyEngine library. User sketches may include it as the Arduino library
// entry point, or include the namespaced headers directly, e.g.:
//   #include <HarmonyEngine/MusicTheory.h>
//
// Pico-DSP-Garden vendors these headers from the upstream musicTheoryCode
// repository. Keep the upstream copy authoritative and sync this folder when
// HarmonyEngine changes.
//
// The resolver only scans the src/ root for headers; placing a file here causes
// the library to be detected and src/ to be added to the include path, which
// then makes the nested <HarmonyEngine/...> headers resolvable.

#pragma once
#include "HarmonyEngine/MusicTheory.h"
#include "HarmonyEngine/ChordAnalyzer.h"
#include "HarmonyEngine/MusicTheoryEngine.h"
#include "HarmonyEngine/VoiceLeadingEngine.h"
#include "HarmonyEngine/CounterpointGenerator.h"
#include "HarmonyEngine/AdvancedHarmony.h"
#include "HarmonyEngine/ReharmonizationEngine.h"
#include "HarmonyEngine/TensionAnalysisEngine.h"
#include "HarmonyEngine/OrchestrationEngine.h"
