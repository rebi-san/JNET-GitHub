# JNET 0.357 moderation build

The release ZIP is `JourneyPC-CompanionsMod/CompanionLab/work/JNET.zip`. The DLL entry point is `diagnostic/probe_dll.c`; the launcher source is `diagnostic/probe_helper.c`. Source headers under `diagnostic/` implement the mod. The release was cross-compiled on Apple Silicon macOS for Windows x64 using Zig 0.13.0 and MinHook 1.3.4.

## Identify the submitted release

SHA-256 values for the submitted 0.357 package:

| File | SHA-256 |
| --- | --- |
| JNET.zip | `33d318fbe8526d730a5b82633f4e91290b394336cc72d798e4ae45bc2d10a3ef` |
| JNET.dll | `f6a7f7753e7e25d4b1d51ab0084522f0a88da288b3607d0ce99b00114ca14a34` |
| JNET.exe | `35fb8cf17422851bdc333f29c107e23ae5b149b021541b5f204264047b0cc99b` |

Embedded build identifier: `3bbe9c8e1c1ef1861487824e4dcdf295019a3b6fd6aa89d1f85609b44c4466e3`.

The ZIP's `manifest.json` records hashes for every packaged artifact. These values identify the submitted binaries; byte-for-byte reproducibility across different host environments has not been established. The embedded build identifier is generated from build inputs, not a Git commit ID.

## Compile the DLL on Windows for source review

Obtain Zig **0.13.0** from <https://ziglang.org/download/0.13.0/> and put its directory on PATH. Use the MinHook **1.3.4** source already included under `work/deps/minhook-1.3.4`; its upstream is <https://github.com/TsudaKageyu/minhook/tree/v1.3.4>. No Visual Studio project or separate Windows SDK is needed for this Zig command.

Clone the private repository with authorized access and fetch its Git LFS objects. A GitHub source ZIP does not necessarily contain the LFS payloads. Run the following in PowerShell from the repository root:

```powershell
git lfs pull
zig version  # Must print 0.13.0
$lab = (Resolve-Path 'JourneyPC-CompanionsMod/CompanionLab').Path
$generated = Join-Path $lab 'work/diagnostic-build'
$minhook = Join-Path $lab 'work/deps/minhook-1.3.4'
$output = Join-Path $lab 'work/moderation-build'
New-Item -ItemType Directory -Force $output | Out-Null
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $output 'zig-cache'
$env:ZIG_LOCAL_CACHE_DIR = Join-Path $output 'zig-local-cache'

$compileArgs = @(
    'cc', '-target', 'x86_64-windows-gnu', '-O2', '-std=c11',
    '-fms-extensions', '-DUNICODE', '-D_UNICODE',
    '-Wall', '-Wextra', '-Werror', '-Wno-unused-const-variable',
    "-I$generated", "-I$minhook/include", '-shared',
    "$lab/diagnostic/probe_dll.c",
    "$minhook/src/buffer.c", "$minhook/src/hook.c",
    "$minhook/src/trampoline.c", "$minhook/src/hde/hde64.c",
    '-lwinmm', '-lbcrypt', '-lws2_32', '-lgdi32',
    '-o', "$output/JNET.dll"
)
& zig @compileArgs
if ($LASTEXITCODE -ne 0) { throw 'DLL compilation failed' }
Get-FileHash "$output/JNET.dll" -Algorithm SHA256
```

This uses the same compiler flags and C translation units as the release builder. It compiles only the DLL; it does not launch Journey, install the mod, run tests, or redirect compiler output to a log. The PowerShell recipe has been checked against the release builder but has not been executed on Windows here.

The generated headers from the submitted build are included in `work/diagnostic-build/`, along with `work/menu-images/jnet_menu_assets.h`. They contain native address signatures, Lua payloads, creature data, fonts, and menu assets and must be included in review. This DLL-only path does not regenerate those inputs or independently establish their provenance.

The released launcher verifies the DLL hash. A separately compiled DLL whose hash differs will not work with that launcher unchanged. Use a complete build to generate a matching launcher; do not disable its hash validation.

## Compile the EXE on Windows for source review

In the same PowerShell session, compile the launcher with the generated headers supplied for the submitted release:

```powershell
$launcherArgs = @(
    'cc', '-target', 'x86_64-windows-gnu', '-O2', '-std=c11',
    '-fms-extensions', '-DUNICODE', '-D_UNICODE',
    '-Wall', '-Wextra', '-Werror', '-Wno-unused-const-variable',
    "-I$generated", "-I$minhook/include",
    '-municode', '-Wl,--subsystem,windows',
    "$lab/diagnostic/probe_helper.c",
    '-lbcrypt', '-luser32', '-lgdi32', '-lcomdlg32',
    '-o', "$output/JNET.exe"
)
& zig @launcherArgs
if ($LASTEXITCODE -ne 0) { throw 'EXE compilation failed' }
Get-FileHash "$output/JNET.exe" -Algorithm SHA256
```

This compiles the EXE without executing it. Review `work/diagnostic-build/built_dll.h` and `scarf_banners.h` as well as `probe_helper.c` and its source includes. The supplied `built_dll.h` pins the submitted DLL's SHA-256 and export addresses. Therefore this standalone EXE build is paired with the submitted DLL, not automatically with the separately rebuilt DLL above. The full builder regenerates `built_dll.h` from the newly compiled DLL before compiling the EXE. Both PowerShell recipes reproduce the release compilation commands but have not been executed on Windows here.

## Regenerate the complete package

The existing release builder is `diagnostic/build_diagnostic.py`. On the recorded Apple Silicon macOS setup, from the repository root:

```sh
python3 -B JourneyPC-CompanionsMod/CompanionLab/diagnostic/build_diagnostic.py --package-only
```

It requires Python 3.9 or newer, Zig at `work/deps/zig-macos-aarch64-0.13.0/zig`, MinHook at `work/deps/minhook-1.3.4`, and a Python environment with Pillow at `work/editor-assets-env/bin/python`. Paths here are relative to `JourneyPC-CompanionsMod/CompanionLab`. The builder does not install dependencies.

Full regeneration also requires the repository's original Journey assets and reference executables, extracted scripts under `work/pc_scripts`, collision inputs, day/night inputs, `WorldLevelPictures`, and the existing menu atlas and manifest. Keep the workspace layout intact. `JourneyPC/Journey.exe` must have SHA-256 `16a7c176b9cce893e0ca72c5d0e4b2e7377c4fabec179f95a5e1331b6c2a788f`; the optional sound-fix reference `Journey-sound-fix-0.3.1.exe` must have SHA-256 `0a73c2fb3a2e5b33f6c38082b966160cd54882fac9efbe09c3ea8dde6fc037ca`. The builder checks both before proceeding.

The builder regenerates payload headers, compiles the DLL and matching launcher, validates imports, exports and hashes, and writes `work/JNET.zip` and `work/package-0.357/JNET/`. `--package-only` skips the ABI test fixture and collision audit reports. It does not install or launch the mod. The complete build succeeded for the hashes above; no gameplay validation is claimed.

For moderation, provide the release ZIP, this guide, and access to the matching source commit including generated inputs and MinHook. Review the launcher's process access and DLL injection, the DLL's native hooks, Steam networking, and file-writing paths in the source. Repository access remains private unless the owner explicitly changes it.
