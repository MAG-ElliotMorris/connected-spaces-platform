# openusd Conan package

Wraps a **prebuilt** OpenUSD install tree as a Conan package, so that CSP and its
downstream consumers can `requires("openusd/26.11")` instead of hardcoding a
local path.

OpenUSD is not built by this recipe. It is built out-of-band by USD's own
orchestrator, `build_scripts/build_usd.py`, which also fetches and builds USD's
dependencies (oneTBB, the Emscripten zlib port). This recipe packages the result.

## 1. Patch build_usd.py before building USD

**This is required, not optional.** `build_usd.py` leaves the exception model
unset, and USD's own CMake then applies `-fexceptions` per target (692 call
sites). That selects Emscripten's *JS-based* exception handling — `invoke_*`
trampolines and `__resumeException` — whereas CSP and everything else in its
Conan graph use *native wasm* exceptions via `-fwasm-exceptions`.

The two ABIs cannot be linked together. Linking a stock USD into CSP fails with:

```
wasm-ld: error: libusd_usdGeom.a(boundable.cpp.o): undefined symbol: __resumeException
```

for every USD object carrying a cleanup or a catch.

In `OpenUSD/build_scripts/build_usd.py`, `GetWasmCompilerFlags()` (around line
83) is the single place all wasm dependencies get their flags from. Change it to:

```python
def GetWasmCompilerFlags(buildTarget):
    compileFlags = '-pthread --use-port=zlib -fwasm-exceptions'
    linkerFlags = '-pthread -fwasm-exceptions'
```

`-fwasm-exceptions` takes precedence over `-fexceptions` regardless of the order
the two appear in, so setting it here overrides USD's per-target flag without
having to patch those 692 call sites.

Editing this function is preferable to passing `--cmake-build-args`, because it
applies to oneTBB as well as USD, keeping the exception mode consistent across
every archive.

`-pthread` and wasm32 already match CSP's `profiles/host/emscripten`, so
`-fwasm-exceptions` is the only divergence.

## 2. Build USD

```powershell
python OpenUSD/build_scripts/build_usd.py --build-target wasm C:/USDBuild
```

Build from clean after changing the flags — a partial rebuild will leave
already-compiled archives in the old exception mode.

## 3. Create the Conan package

```powershell
conan create recipes/openusd `
  --profile:host=profiles/host/emscripten `
  --profile:build=profiles/build/windows `
  -c user.openusd:prebuilt_root=C:/USDBuild
```

The recipe verifies the tree was built with `-fwasm-exceptions` (by reading
`build/OpenUSD/CMakeCache.txt`) and refuses to package it otherwise.

Re-run this after every USD rebuild. `build_type` is erased from the package ID,
so one package serves Debug and Release CSP builds — Emscripten/libc++ has no
debug-vs-release runtime ABI split.

## How consumers link it

The recipe sets `cmake_find_mode = "none"` and consumers call
`find_package(pxr CONFIG)` against USD's own config, rather than Conan
describing USD's libraries itself.

This matters. USD's `pxrTargets.cmake` wraps every archive in
`-Wl,--whole-archive` because USD registers schema types and plugins through
static initialisers; without it the linker discards those objects and USD fails
at runtime with unresolvable `TfType`s rather than at link time. Restating that
link line (plus the exact library order and the TBB dependency) in `cpp_info`
would be a second source of truth that silently diverges on each USD upgrade.

`pxr_DIR` and `TBB_DIR` are pointed at the package from the root
`conanfile.py`'s `generate()`. Both of USD's configs are relocatable, so they
work unchanged from the Conan cache.

## Two things the root CMakeLists has to work around

Both were found by `Tools/UsdSmoke` and neither shows up as a build failure.

### Whole-archive is load-bearing, and CMake silently drops it

Each USD imported target wraps itself in
`-Wl,--whole-archive ... -Wl,--no-whole-archive`. CMake de-duplicates repeated
link flags, so all 33 pairs collapse into a **single empty pair** and every USD
archive gets linked normally. USD registers its schemas, file formats and asset
resolvers through static initialisers that nothing references directly, so the
linker discards them. The build succeeds and then fails at runtime:

```
Coding Error: Failed to manufacture asset resolver Sdf_UsdzResolver from plugin sdf
Runtime Error: in _DefinePrim -- Failed to create primSpec for </Root>
```

`plugInfo.json` still advertises those types; only the C++ registration is gone.
It also manifests further away as a TBB crash
(`null function or function signature mismatch` in `control_storage::active_value`)
during teardown.

CMake's `$<LINK_LIBRARY:WHOLE_ARCHIVE,...>` cannot express this, because USD's
targets also reference each other plainly and CMake refuses to link one item both
with and without a feature. The root `CMakeLists.txt` therefore builds a single
explicit whole-archive group over every USD archive and passes it as a raw link
option.

### USD's per-file --embed-file list overflows the command line

USD is not self-contained once linked: it discovers its schemas at runtime from
`lib/usd/plugInfo.json` and the per-library `lib/usd/*/resources/` directories.
Natively it derives that search path from its shared libraries' location; under
wasm there is none, so USD's imported targets carry `--embed-file` entries in
`INTERFACE_LINK_OPTIONS` that mount `lib/usd` at `/usd`.

Helpful, but there is one entry *per resource file* — roughly a hundred
arguments, each an absolute path into the Conan cache. Linking all of USD that
way fails on Windows with `The command line is too long`. The root
`CMakeLists.txt` clears those properties and substitutes one directory-level
embed producing the identical layout. `--embed-file` entries are the only thing
USD puts in that property, so clearing it loses nothing.

A browser-facing build may want `--preload-file` instead, which moves the
resources into a separately cacheable `.data` file rather than the `.wasm`.

### Discovering the plugins at runtime

`PXR_PLUGINPATH_NAME` is only consulted while `PlugRegistry` initialises, so
whether a `setenv()` in `main()` is early enough depends on whether static
initialisation touched the registry first. `Tools/UsdSmoke` calls
`PlugRegistry::RegisterPlugins("/usd")` explicitly instead, which has no ordering
requirement — worth copying in any consumer, since under wasm there is no
launcher script to set the variable.

## Version

The packaged tree reports `PXR_VERSION 2611` (26.11), a dev-branch snapshot
ahead of the latest tag (v26.08, July 2026) — not a released version. Bump
`version` in the recipe when you move to a different USD.
