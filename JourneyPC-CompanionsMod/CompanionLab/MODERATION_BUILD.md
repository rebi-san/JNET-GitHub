# JNET 0.357 moderation build

This repository is the source-review snapshot for the JNET **0.357** Windows x64 DLL and launcher EXE. It contains only their transitive source/header inputs, generated payload headers, MinHook sources and license notices, this guide, the rebuild script, and Git metadata files. [MODERATION_BUILD_INPUTS.txt](MODERATION_BUILD_INPUTS.txt) is the complete repository-root-relative allowlist of 157 files, including itself.

## Submitted release

| Artifact | SHA-256 |
| --- | --- |
| JNET.zip | `992d7beacbe686b22cd9c38b1f63390a3eed3509a6eb9a983913be089e07567d` |
| JNET.dll | `f233ffb8490233c035a044ba27bbc7376663652368d16a428db4361c97496595` |
| JNET.exe | `a81aea154ba9ea172424c7578a5a07dda6ad737a2fb622624c4c36ede408c5d8` |

Embedded build identifier: `7a13a4336b796a123790f058c11386139debe62c9beb869cbbcf4434827159c5`.

The submitted ZIP is provided separately; it is not tracked in this source-only repository. Its `manifest.json` hashes every packaged artifact. The local release output is `JourneyPC-CompanionsMod/CompanionLab/work/JNET.zip`. No game executable, compiler distribution, environment, cache, installation receipt, runtime log, test binary, or unrelated research is required in this Git tree. Removing those files from Git tracking preserves the developer's local copies. Older commits still contain the previous tree.

## Creature movement change

The shared movement path retains the correction that shifts native head history by the externally applied target displacement, avoiding counting that displacement twice. Controlled Longtail simulates before its private cloth preparation and skips its later duplicate simulation in the same frame.

This revision stabilizes vertical orientation for every playable creature. Straight ascent/descent preserves heading, while the orientation stays slightly short of the native world-up singularity; actual movement remains fully vertical. During native segment simulation, a guarded hook projects the previous right axis onto the new forward plane near vertical and reconstructs an orthonormal basis before the native marker/mesh publication. This prevents a collapsed or flipped sideways axis from feeding the visible pose and cloth. Level-flight bases, native NPCs, remote creatures, and attack-controlled simulation retain their native segment behavior.

Review `diagnostic/jnet_character.h`, `jnet_character_native.h`, `jnet_character_live.h`, and the hook registration in `probe_dll.c`. The new segment hook is pinned to RVA `0x2380d0` in `target_build.h`; startup verifies native entry signatures. It does not patch original game files.

Focused local checks passed at **30 and 55 FPS** for ascent/descent, Dash displacement, pole noise and orientation continuity, head-history rollback, and once-per-frame Longtail scheduling. Native scheduling/arithmetic fixtures are not rendered-game tests. The supplied pre-revision reports show zero sampled head holds and negligible target/head error; they do not prove the final rendered result. This release has not been installed or gameplay-validated. There is no claim of guaranteed elimination of every stutter.

## Rebuild both binaries

Prerequisites: Python **3.9+**, Zig **0.13.0** for the reviewer's host, and Git LFS payloads. MinHook **1.3.4** source and its bundled HDE license are included. Zig supplies the Windows cross-toolchain; no separate Windows SDK or Visual Studio project is required. No dependencies are downloaded or installed by the build script.

After cloning the authorized repository, run from its root:

```sh
git lfs pull
python3 JourneyPC-CompanionsMod/CompanionLab/build_moderation.py --zig /path/to/zig
```

On Windows PowerShell:

```powershell
git lfs pull
py -3 JourneyPC-CompanionsMod/CompanionLab/build_moderation.py --zig C:/tools/zig/zig.exe
```

If Zig 0.13.0 is on PATH, omit `--zig`. Compiler downloads: <https://ziglang.org/download/0.13.0/>. MinHook upstream: <https://github.com/TsudaKageyu/minhook/tree/v1.3.4>.

Outputs are `JourneyPC-CompanionsMod/CompanionLab/work/moderation-build/JNET.dll` and `JNET.exe`. The command compiles only; it never installs or launches either binary. It uses the existing JNET release compiler flags and translation units: C11, `x86_64-windows-gnu`, `-O2`, Unicode, Microsoft extensions, warnings as errors, the native DLL source, and MinHook's buffer/hook/trampoline/HDE64 sources. The EXE is built as a Windows GUI executable.

The script first builds the DLL, reads its PE exports, hashes that exact DLL, then generates an output-local `built_dll.h` and compiles the EXE against it. This preserves the launcher's DLL hash and RVA checks. Do not replace only one member of the pair or disable validation. The checked-in `work/diagnostic-build/built_dll.h` identifies the submitted DLL for review; the rebuild's output-local copy takes precedence during launcher compilation.

Generated inputs in `work/diagnostic-build/` and `work/menu-images/jnet_menu_assets.h` are part of the reviewed source. They contain executable-address signatures, embedded Lua, creature resources, collision/environment payloads, fonts, and UI data. The source-only rebuild consumes these supplied headers and does not regenerate original game-derived assets or the distribution ZIP. The full asset-generation workspace is deliberately excluded. Neither a Journey installation nor private bottle paths are needed to compile these binaries.

The complete release build passed, including artifact hashes, PE exports, embedded build identifier, GUI subsystem and system-library import checks. The source-only recipe also compiled both binaries successfully from an isolated copy of exactly the 157 allowlisted files. Its regenerated launcher header was checked against the rebuilt DLL hash and every required export RVA. Cross-host byte-for-byte reproducibility and Windows execution are not assumed. The identifier names the submitted generated-input snapshot, not a Git commit or a fresh hash of reviewer edits.

## Review scope

The DLL entry point is `diagnostic/probe_dll.c`; the launcher entry point is `diagnostic/probe_helper.c`. Review their included headers and generated payloads alongside them, especially launcher process access/DLL injection, native hooks, Steam networking, and save/configuration/report export writes. Notices are in `diagnostic/THIRD-PARTY.txt`, MinHook's `LICENSE.txt`/`AUTHORS.txt`, and `journey_detour-main/LICENSE.txt`.

Provide this matching source commit and the separately submitted release ZIP to Nexus Mods support. Repository visibility is unchanged by this release.
