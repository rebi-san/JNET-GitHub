# JNET

Private source repository for the Journey companion mod and its native-code research tools. This snapshot contains the 0.283 source. Package installation and offline tests do not establish live multiplayer correctness.

## Current version: 0.283

This version addresses receiver admission getting stuck after JNET has already owned a native companion slot. It can resume that retained slot after a receiver reset when storage, ownership, chapter and gate checks pass. Vanilla connection guards remain enabled. A stale result 124 is cleared only after successful state publication without an ownership guard failure. This does not establish that every reported visibility issue is fixed in live play.

## Contents

- `JourneyPC-CompanionsMod/CompanionLab/diagnostic/`: Windows helper, injected DLL, native adapters, Lua payloads, build scripts, checks, and test fixtures.
- `JourneyPC-CompanionsMod/CompanionLab/`: resource-generation and executable-analysis scripts, Python/Lua tests, and historical research notes.
- `JourneyPC-CompanionsMod/CompanionLab/research/`: selected standalone experiments and the orchestration-fixture generator previously kept in the local work directory. These are historical research, not an assertion that every experiment passes against current source.
- `source-inventory.json`: original workspace paths and exported-file hashes. Entries edited for this export also retain their original source hash and an edit note.

Only CompanionLab project files are included; the enclosing JourneyPC-CompanionsMod directory preserves paths used by the existing scripts. All other project folders are excluded. ObjectOutfits, extracted outfit references, game installations, game assets, downloaded toolchains, build products, release ZIPs, personal settings, installation receipts, saved games, and session logs are excluded. The optional prebuilt sound-fix payload and unrelated projects are also excluded.

## Local build inputs

The existing build system is retained. A clone alone is not sufficient to build: provide your own local inputs in these ignored paths under the repository root:

1. `JourneyPC/`: the supported Steam Journey installation, including its executable, data, and font files. The scripts check executable fingerprints.
2. `Journey-sound-fix-0.3.1.exe`: the supported sound-fix reference executable used by the build checks; it is not distributed here.
3. `JourneyPC-CompanionsMod/CompanionLab/work/deps/zig-macos-aarch64-0.13.0/`: Zig 0.13.0 for the existing Apple Silicon build workflow.
4. `JourneyPC-CompanionsMod/CompanionLab/work/deps/minhook-1.3.4/`: MinHook 1.3.4 source, including its original license.
5. `Journey-Save-Editor-Windows-main/Journey Save Editor_Data/sharedassets0.assets`: local input for the menu-image extraction script. That script also requires UnityPy and Pillow. Generated menu images remain ignored.

The signature audit also requires a separately supplied local `journey_detour-main/` reference checkout, which is excluded from Git.

The build script does not download dependencies. Some research tools additionally require the local iOS game assets or other original reference inputs described in their source. Do not add those inputs to Git.

From the repository root, after supplying the inputs:

```sh
mkdir -p JourneyPC-CompanionsMod/CompanionLab/work
python3 JourneyPC-CompanionsMod/CompanionLab/diagnostic/extract_menu_images.py
python3 JourneyPC-CompanionsMod/CompanionLab/diagnostic/build_diagnostic.py
```

Relevant checks include `diagnostic/check_menu.py`, `diagnostic/check_native_actor.py`, and the Python tests in CompanionLab. Some fixtures execute extracted x86-64 instructions and require an appropriate local environment. At the 0.283 snapshot, the focused local receiver regression and menu checks passed; the broader actor runner stopped at an unused-function warning in the chirp fixture.

The research folder preserves the relative depth of the original experiments so diagnostic includes still resolve. Scripts that previously consumed files beside themselves in `work/` may require generated inputs to be supplied explicitly. Run research generators only after reviewing their prerequisites and outputs.

See `JourneyPC-CompanionsMod/CompanionLab/diagnostic/THIRD-PARTY.txt` for provenance and third-party notices. No new license is assigned to the project's own source by this export.
