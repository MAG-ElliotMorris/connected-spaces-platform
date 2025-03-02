/*
 * Copyright 2023 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

// Enable this define if you are using the CSP library in DLL form
// #define USING_CSP_SHARED

#if defined(_WIN32) && !defined(CSP_WINDOWS)
#define CSP_WINDOWS
#elif defined(__APPLE__) && !defined(CSP_MACOSX) && !defined(CSP_IOS)
#include <TargetConditionals.h>

#if defined(TARGET_OS_MAC)
#define CSP_MACOSX
#elif defined(TARGET_OS_IPHONE)
#define CSP_IOS
#endif
#elif defined(__EMSCRIPTEN__) && !defined(CSP_WASM)
#define CSP_WASM
#elif defined(__ANDROID__) && !defined(CSP_ANDROID)
#define CSP_ANDROID
#elif defined(__linux__) && !defined(CSP_LINUX)
#define CSP_LINUX
#endif

#if __has_include(<unistd.h>)
#define CSP_POSIX
#endif

#if defined CSP_WINDOWS
#define CSP_EXPORT __declspec(dllexport)
#define CSP_IMPORT __declspec(dllimport)

#elif defined CSP_MACOSX || defined CSP_IOS || defined CSP_ANDROID
#define CSP_EXPORT __attribute__((visibility("default")))
#define CSP_IMPORT __attribute__((visibility("default")))

#endif

#ifdef CSP_BUILD_SHARED
#define CSP_API CSP_EXPORT
#define CSP_C_API CSP_EXPORT
#elif defined USING_CSP_SHARED
#define CSP_API CSP_IMPORT
#define CSP_C_API
#elif defined CSP_WASM
#define CSP_API
// The EMSCRIPTEN_KEEPALIVE keyword is the way to export a function from WASM in order to call it from the JS side.
#define CSP_C_API EMSCRIPTEN_KEEPALIVE

#else
#define CSP_API
#define CSP_C_API
#endif

// Wrapper generator macros
#define CSP_NO_EXPORT
#define CSP_ASYNC_RESULT
#define CSP_ASYNC_RESULT_WITH_PROGRESS
#define CSP_EVENT
#define CSP_OUT
#define CSP_IN_OUT
#define CSP_START_IGNORE
#define CSP_END_IGNORE
#define CSP_INTERFACE
#define CSP_NO_DISPOSE
#define CSP_FLAGS

// TODO: Remove the following includes. I don't think we should be default including these everywhere
#include <stdint.h>
#include <stdlib.h>

#ifdef CSP_WASM
#include <emscripten/emscripten.h>
#endif
