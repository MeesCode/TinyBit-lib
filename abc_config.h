#ifndef TINYBIT_ABC_CONFIG_H
#define TINYBIT_ABC_CONFIG_H

// TinyBit-local configuration for the vendored ABC-parser.
//
// This file is auto-included by abc_parser.h (via __has_include) whenever it is
// on the compiler's include path, which it is for both builds of the engine:
//   * the wasm build (build.rs adds .include("src/tinybit"))
//   * the CMake build (CMakeLists.txt adds this dir to the include path)
// The standalone ABC-parser repo does not see this file, so its own build keeps
// the ABC-standard defaults and its test suite is unaffected.
//
// TinyBit keeps its historical behavior for headerless music: tempo 100 and a
// fixed L:1/4 unit note length, rather than the parser's ABC-standard defaults
// (tempo 120 / meter-derived 1/8). This only affects tunes that omit Q:/L:/M:.

#define ABC_DEFAULT_TEMPO_BPM 100
#define ABC_DERIVE_LENGTH_FROM_METER 0

#endif // TINYBIT_ABC_CONFIG_H
