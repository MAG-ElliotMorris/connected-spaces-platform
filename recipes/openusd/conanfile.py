import os
import re

from conan import ConanFile
from conan.errors import ConanException
from conan.tools.files import copy


class OpenUSDConan(ConanFile):
    """Packages a *prebuilt* OpenUSD install tree.

    OpenUSD is built out-of-band by its own orchestrator,
    OpenUSD/build_scripts/build_usd.py, which fetches and builds USD's
    dependencies (oneTBB, the Emscripten zlib port) itself. This recipe does not
    try to replace that; it takes the resulting install tree and turns it into a
    Conan package so that both this repository and downstream consumers
    (connected-spaces-platform-web) can depend on it through the normal
    requires() mechanism instead of hardcoding a machine-specific path.

    Deliberately, we do not describe USD's libraries via cpp_info.libs.
    USD's installed pxrTargets.cmake wraps every archive in
    -Wl,--whole-archive, and that is load-bearing rather than incidental: USD
    registers its schema types and plugins through static initialisers, so
    without whole-archive the linker discards those objects and USD fails at
    *runtime* with unresolvable TfTypes rather than at link time. Restating
    USD's link line (whole-archive, the exact library order, and the TBB
    dependency) inside cpp_info would create a second source of truth that
    silently diverges on every USD upgrade. Consumers therefore call
    find_package(pxr CONFIG) and use USD's own config, which we have verified is
    fully relocatable -- both pxrTargets.cmake and TBBTargets.cmake derive their
    prefix from CMAKE_CURRENT_LIST_FILE and contain no absolute paths.

    Consumers are expected to point pxr_DIR (and TBB_DIR) at this package from
    their own generate(); see the requirements()/generate() pair in the
    repository root conanfile.py for the pattern.
    """

    name = "openusd"
    version = "26.11"
    package_type = "static-library"
    settings = "os", "compiler", "build_type", "arch"

    # Subdirectories of the install tree that make up the consumable package.
    # build/ and src/ are build_usd.py's working area (the USD checkout and the
    # CMake build trees) and run to gigabytes, so they are excluded.
    _tree_dirs = ("include", "lib", "plugin", "share", "cmake")

    @property
    def _prebuilt_root(self):
        root = self.conf.get("user.openusd:prebuilt_root", check_type=str)
        if not root:
            raise ConanException(
                "openusd packages a prebuilt USD tree, so it needs to be told "
                "where that tree is. Pass it on the command line, e.g.\n"
                "  conan create recipes/openusd "
                "-c user.openusd:prebuilt_root=C:/USDBuild ...\n"
                "See recipes/openusd/README.md."
            )
        return root.replace("\\", "/")

    def build(self):
        # These checks live in build() rather than validate() on purpose:
        # validate() also runs for every consumer of this package, and consumers
        # have no reason to know where the prebuilt tree lives. Nothing is
        # compiled here - this only inspects the tree we are about to package.
        root = self._prebuilt_root

        if not os.path.isfile(os.path.join(root, "pxrConfig.cmake")):
            raise ConanException(
                f"'{root}' does not look like an OpenUSD install tree "
                "(no pxrConfig.cmake at its root)."
            )

        self._check_exception_flags(root)

    def _check_exception_flags(self, root):
        """Reject a USD tree built without -fwasm-exceptions.

        Upstream build_usd.py's GetWasmCompilerFlags() leaves the exception
        model unset, and USD's own CMake then applies -fexceptions per target.
        That selects Emscripten's JS-based exception handling (invoke_*
        trampolines and __resumeException), whereas CSP and the rest of its
        Conan graph use native wasm exceptions via -fwasm-exceptions. The two
        ABIs cannot be linked together -- wasm-ld fails with
        "undefined symbol: __resumeException" for every USD object carrying a
        cleanup or a catch.

        The failure is loud rather than silent, but it appears at the final link
        of whatever consumes CSP, which is a long way from the USD build that
        caused it. Checking here reports it at the point it can be fixed.
        """
        if self.settings.os != "Emscripten":
            return

        if self.conf.get("user.openusd:allow_unchecked_flags", check_type=bool, default=False):
            self.output.warning(
                "user.openusd:allow_unchecked_flags is set, so the "
                "-fwasm-exceptions check was bypassed. Only do this to prove "
                "build wiring; a USD packaged this way may abort at runtime "
                "inside path parsing."
            )
            return

        cache = os.path.join(root, "build", "OpenUSD", "CMakeCache.txt")
        if not os.path.isfile(cache):
            self.output.warning(
                "Could not find build/OpenUSD/CMakeCache.txt in the prebuilt "
                "tree, so the -fwasm-exceptions check was skipped. If this USD "
                "was built without -fwasm-exceptions, exception handling inside "
                "USD is silently broken. See recipes/openusd/README.md."
            )
            return

        with open(cache, "r", encoding="utf-8", errors="replace") as handle:
            match = re.search(r"^CMAKE_CXX_FLAGS:STRING=(.*)$", handle.read(), re.M)

        flags = match.group(1) if match else ""
        if "-fwasm-exceptions" not in flags:
            raise ConanException(
                "This USD tree was built without -fwasm-exceptions "
                f"(CMAKE_CXX_FLAGS was '{flags.strip()}').\n"
                "Emscripten's default is DISABLE_EXCEPTION_CATCHING=1, so USD's "
                "own try/catch blocks were compiled out and destructors on "
                "unwind paths were omitted. It will link and then abort at "
                "runtime inside path/.usda parsing.\n"
                "Fix GetWasmCompilerFlags() in "
                "OpenUSD/build_scripts/build_usd.py so it reads:\n"
                "    compileFlags = '-pthread --use-port=zlib -fwasm-exceptions'\n"
                "    linkerFlags  = '-pthread -fwasm-exceptions'\n"
                "then rebuild USD from clean. See recipes/openusd/README.md."
            )

    def package_id(self):
        # The prebuilt tree is used exactly as built (Release), for every CSP
        # configuration. Unlike MSVC, Emscripten/libc++ has no debug-vs-release
        # runtime ABI split, so a Debug CSP linking a Release USD is fine and we
        # do not want to force a second USD build just to satisfy package_id.
        self.info.settings.rm_safe("build_type")

    def package(self):
        root = self._prebuilt_root

        for directory in self._tree_dirs:
            source = os.path.join(root, directory)
            if os.path.isdir(source):
                copy(
                    self,
                    "*",
                    src=source,
                    dst=os.path.join(self.package_folder, directory),
                    keep_path=True,
                )

        # pxrConfig.cmake lives at the root of a USD install prefix, not under
        # lib/cmake, which is why consumers need pxr_DIR set to the package
        # folder itself.
        copy(self, "pxrConfig.cmake", src=root, dst=self.package_folder)

    def package_info(self):
        # Consumers use USD's own pxrConfig.cmake (see the class docstring), so
        # suppress Conan's generated config rather than have two competing
        # descriptions of how to link USD.
        self.cpp_info.set_property("cmake_find_mode", "none")

        # Nothing here is consumed through CMakeDeps, but keeping the layout
        # accurate means `conan cache path`/graph output and any non-CMake
        # consumer still describe the package correctly.
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.resdirs = ["lib/usd"]
        self.cpp_info.defines = ["PXR_STATIC"]
