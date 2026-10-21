## 0.55: renderer resource indices and trial torso preparation

The user reported 0.54 at update 8325: appearance result 6, stage 0, zero
setters, check 5; envOccCube resource code 3; all 13 source/trial parameters
present; only texture bit 4096 differed from the definition. No draw occurred.
Both applications were then confirmed closed. This report does not identify
which subfield mismatched and does not prove the cause of earlier invisibility.

Pinned PC evidence in `work/native/text.asm` (all addresses below are RVAs):

- `a9680-a9695` selects a game-owned environment texture pair from
  Game+646d30/646d40 using the traveler flag. `a9825-a9836` passes it to
  the same texture setter used for ordinary appearance textures.
- `2736a8-2736b6` skips rewriting a binding when its handle is already equal.
  `2db09e-2db217` forms a 24-byte binding, including handle+c at binding+4.
- **`34c7d0-34c874` copies the material buffer and refreshes binding+4 from
  handle+c in the copy. `34edaf-34ee53` uses it as a resource-array index.**
  Kinds 1/3 use count/table at handle+30/+38 with 48-byte entries; kind 2
  uses +38/+40 with 40-byte entries. The previous generation interpretation
  was wrong. A stored index can lag without a changed texture identity.
- `263890` resolves `spineB` through model+a8's name tree (node name+20,
  target+40). `263990` and `a94dc-a9550` accumulate node+20 transforms along
  parent+8. `91600` composes parent times child in column-major affine layout.
  `51e0-51f8` initializes the center-joint string to the same `spineB` name.
- `a956f` inverts that world transform; `a97fa-a9808` writes `dudeInvTorso`.
  `a9840-a984e` writes the spine's world translation as `dudeCenter`.
  `273508-273525` copies the matrix without a transpose.
- `JourneyIOS/Cloth_vs.cg:170-186` uses inverse torso for body occlusion and
  lighting. `Cloth_fs.cg:734-737` uses the environment cube for lighting and
  occlusion; `:840-852` uses center/dissolve for the sand fade. The corresponding
  readable PC shader source was matched previously; no iOS addresses are used.

Implementation: `native_appearance.h` now separates texture identity from the
resource-slot index. Both cached and active indices must fit a readable bounded
array (diagnostic cap 16). Stable metadata is reread; replacement tables,
handles, types and aliases decline. The renderer still performs its own copy
and index refresh; original buffers are never fixed up. Borrow checks validate
both source and trial bindings before each additional consumer call.

The previously unintegrated `native_trial_shader.h` is now audited and wired
into the bounded preview. It reads only the trial spine hierarchy, rejects
cycles/missing roots/non-finite or singular transforms, and writes exactly the
16-byte center and 64-byte inverse torso fields in its separate material.
The shader reader rejects duplicate or overlapping named descriptors; all
active material instances are inventoried for cross-traveler overlap. Whole
selected buffers and ownership are rechecked before/after writing. Partial
writes or failed checks stop the preview and require restart, without unsafe
rollback. No full Dude update, cloth simulation, new hook or networking is added.

Validation: expanded ASan/UBSan fixtures and Windows x64 cross-builds passed.
The combined fixture checks 600 valid slot rotations with unchanged material
indices, invalid active/cached indices, missing/oversized/replaced slot tables,
owner/handle/type changes, parent-child rotation/translation and inverse math,
cyclic hierarchies, singular/non-finite transforms, duplicate/overlapping shader
fields, material aliasing, original-buffer corruption and partial writes.
All 14 Python/package checks passed after the helper/DLL build. Live/full status
format arity is 37/91 with conservative bounds 3187/6043 characters.

Installed in devishaz after the user confirmed both applications closed.
ZIP `work/JourneyCompanions-Diagnostics-0.55.zip`: 254717 bytes; SHA-256
`23eae4bfd9aeb26e45a5f178fd227aafba04e7e92d12a85797efcfce60487bce`.
All nine installed diagnostic hashes and eight original-file hashes verified;
rollback receipt `work/install-devishaz-0.54.json`. No game/helper was launched
and no other bottle accessed. Git inspection reported this is not a Git repository;
no repository-changing Git command was used.

Source and harness evidence do not establish live resource lifetime, GPU
correctness, visibility or three-client gameplay. Restart-only rendered-resource
ownership and the real additional-traveler/network adapter remain outstanding.

# Rendering and three-client architecture review

Updated 2026-10-08 through the user's Scarfhelper 0.53 result and the supplied community-mod audit. This review supersedes the proposed two-uniform fix and the older README's next-test instructions. **No new runtime test is requested yet.** The first product milestone remains three independently controlled clients, each seeing and interacting with two companions. A stationary mesh is only a rendering prerequisite.

## 0.54: validated current texture bindings

The user's 0.53 run at update 9378 accepted the material list as three entries / one unique instance, then stopped in Desert with appearance result 6, stage 0, zero setters, preview failure 20 and zero validated consumer calls. Both applications were subsequently confirmed closed. This confirms the repeated-instance correction for that run. It does **not** identify whether the next rejection was a missing source input, a missing trial input, or a source texture that differed from the definition: 0.53 returned before saving those masks.

The source review checked the complete binding path, not only one opacity value:

- `9d9a0` resolves definition row+48 through resource+10/payload+c8 and invokes texture setter `273640`; `a6b43` passes the local mesh at Dude+160. Our original local material address matches that mesh's +80 material.
- `217c30` changes atlas resource names through the current appearance definition for outfit/style selection. This supports treating displayed bindings and definition resolution as distinct observations, but does not prove a style change caused this particular rejection.
- `273640` looks up the parameter and compares the current binding's +10 handle before calling `2db010`. `2db010` constructs a 24-byte binding: type, generation from handle+c, owner=texture+8, handle=texture+70. Texture kind comes from texture+110. `364f80` reads handle+94 bit 31, matching the binding flag used for 2D textures. `270a80` copies the descriptor's exact width into the selected material buffer. The reader retains the exact 24-byte texture-width requirement.
- PC Cloth source and `JourneyIOS/Cloth_fs.cg` both use atlas A/B, mask, detail and environmental occlusion inputs. See iOS lines 408/419, 432/433, 459-462 and 734-737. Missing or malformed bindings must not be treated as an arbitrary zero/opaque fallback.

`jc_appearance_bound_texture` now resolves the actual original-traveler binding through its owner, validates the reconstructed texture's handle/generation/kind/flags, compares all four binding fields, and rereads the resource metadata. A different texture from the definition is permitted only if this independent validation passes. `jc_appearance_source_plan` builds the trial texture plan from those validated current bindings; original scalar/style capture and the selected native setters remain in use. No raw material object or whole pointer-filled traveler is copied.

Before setters, required source/trial descriptors, all traveler buffer separation, complete selected-buffer snapshots, definition resource stability, list ownership and live texture metadata are rechecked. Missing required parameters or a trial texture without a corresponding source input still decline. After setters, original buffers must be byte-identical, trial parameters must match the prepared plan, and unrelated trial bytes must remain unchanged. Borrowed-resource checks retain both the definition snapshot and the actual source texture plan; replacement of either, a changed generation/flag/handle, a changed source binding or a chapter transition stops further added rendering. These remain same-thread CPU checks with borrowed resources, not new reference-count ownership or GPU-completion proof.

Source failure details are retained before returning: reason, parameter name, resource code, found/matching masks, missing source/trial masks and difference from definition. Both the live panel and full Status show these fields. A terminal panel now says to copy/quit instead of instructing the user to return to gameplay. No additional game hooks, native entry points, connection changes or draw-wrapper experiments were introduced.

Local sanitizer fixtures cover valid alternate A/B atlases (including the 2D high flag), all five alternate textures, unchanged original buffers, exactly thirteen setters, rejected missing descriptors, invalid owner ranges, mismatched handles/generations/kinds, unreadable/changing resources, changes immediately before setters and borrowed-resource replacement. The existing three-simulated-client routing harness and its malformed/reconnect/chapter tests still pass. These are synthetic-memory/setter tests, not actual client or GPU tests.

0.54 is installed in devishaz for the same combined appearance/render test; nine diagnostic hashes and eight original-file hashes passed verification, with 0.53 retained for rollback. A source-validation success is necessary but is not proof of visible pixels or three-player gameplay. If preparation completes but no mesh appears, the remaining torso/skinning/pass work is still open. If it declines, the same run now identifies the parameter and condition without another generic “code 6” round. Installation and package verification are recorded in README.

## Community references: scrub and connection findings

`ObjectOutfitsMod` is now a **sanitized, reference-only copy**, not a runnable release. At the user's explicit request, removed the two excluded outfit definitions, their joint memberships, eight decoration records in each of twelve chapter files (96 records total), the menu entries and their special animation logic in the executable's compressed Lua. No excluded backup was retained in that folder. The README retains Alazar88's attribution but replaces installation instructions with a reference-only notice. No reference mod was installed or executed.

The executable's Tick and DebugKeys gzip frames were rewritten within their existing bounded extents, with no change to image length or native code. These are edits to the community reference copy only; `JourneyPC`, `JourneyIOS`, and the working game files remain unchanged. The cleanup checked all 202 successfully decoded embedded gzip streams, JSON, chapter record names, and both decompressed texture archives. Remaining `Ghost` substrings are unchanged stock Bullet physics RTTI (`btGhostObject`, `btPairCachingGhostObject`, `btGhostPairCallback`), unrelated to the excluded outfit. Shared vanilla assets needed by permitted outfits remain. Local audit: `Lab:work/object-outfits-reference/scrub-verification.json`; only sanitized Lua is extracted beside it.

Decoration records are 2112 bytes with the object name at +112, after a 16-byte header. Credits and Matt already contained 24 trailing records outside their declared counts. The scrub removed eight excluded trailing records there and preserved the declared counts and other trailing records, rather than silently repairing unrelated data. These chapters and the edited executable are not runtime-validated.

Useful retained outfit evidence (sanitized `Tick.lua:150` onward and `DebugKeys.lua:Mod.DoHideBody`):

- Each joint group creates a marker and decoration clump. `AttachObjectToJoint` explicitly sets `useLocalDudeMesh=true`; `AttachObjectToMarker` supplies position/orientation attachment. This demonstrates a data-driven decoration attachment route to the existing local traveler, not another independently simulated player.
- AncestorSpirit, LongtailCarpet, SmallJellyfish, BigJellyfish and WarMachine remain as references. The README explicitly says companions see the normal appearance. This mod supplies no demonstrated synchronized outfit protocol.
- The body-hiding path changes a shader and calls `InitLocalDude` with both local and remote definitions. Replaying it against our retained trial would risk reinitializing original travelers. We have not integrated this path.
- Future synchronized appearance should use a bounded, versioned outfit identifier from an installed allowlist, attached to the participant identity/lease. Peers must not send scripts, paths, mesh pointers or arbitrary resource names. The reference's author-reported quit crashes and robe-sharing effects are reasons to audit ownership, not accepted behavior for JourneyCompanions.

`MeetEverywhereandPlayAll LevelsMod` contains a readable Cheat Engine table, a CETRAINER, a standalone trainer EXE and a README. The README names the 2020-07-15 Steam release and credits Alazar88, Rebi, Paleologos and Yupa_Miralda; it says Cheat Engine 7.2 was used. No separate source project or license file was supplied. We inspected the CT as the mechanism's source; bundled trainer equivalence and runtime safety have not been established. No trainer or dependency was launched or installed. Hashes and decoded entry summaries: `Lab:work/meeting-reference/audit.json`.

| Mechanism | Supplied CT and pinned PC evidence | Consequence |
| --- | --- | --- |
| Allow meeting | CT entry 425448, RVA `187f67`: original `66 C7 41 08 01 01`, changed immediate `00 00` | Changes two event-state bytes at object+8/+9. The nearby original update `187f80` clears +421 in the object reached through its second argument's +180 pointer and handles duration. This is a lobby-suppression event change, not a peer-count allocation. |
| Prevent scripted remote takeover | CT entry 425449, RVA `170e22`: original `66 C7 42 09 00 01`, changed immediate `00 00` | Changes event defaults at +9/+10. Both “connect everywhere” and “meet everywhere” change suppression; only the latter also disables this scripted takeover. The README reports that connecting alone can leave the companion invisible/inaudible in the hub. This is an author-reported behavior, not a test we ran. |
| Credits movement | CT entry 425450, RVA `17daba`: replaces timer-derived `mov byte [rbx+9],al` and adjacent bytes | Broad event behavior with documented movement side effects; it is not required to add another peer. |
| Level swaps | CT changes embedded resource-script text starting at `376169e`, `375cc90`, `3761e87`, `3760dda` | Redirects chapter resource paths/banks. No extra peer state or traveler collection is introduced. Keep as a later optional feature, not the joining architecture. |

All three CT CodeEntry original instruction sequences match the supplied PC executable byte-for-byte. That validates these locations for this exact hash, not other storefronts/builds, and does not prove that the trainer itself is safe. The README's size/date alone is not a compatibility check.

PC/iOS `TriggerInstances.lua` texts match for Desert, Credits and Summit. Chapter Select differs in only two unrelated fade/delay values (iOS 1.0/2, PC 5.3/6); all four suppression/takeover records match including IDs and variables. PC Chapter Select defines indefinite suppression at `TriggerInstances.lua:618` (`duration=-1`) and long-lived remote takeover at `:3519` (`duration=1e+008`, `multiplayerOnly=false`). This is direct cross-platform support for the chapter-rule interpretation. It is not permission to transfer iOS native offsets or networking APIs.

**Implication for the first three-client test:** keep it in ordinary Desert initially. Neither reference resolves the single remote staging area, acknowledgment history or entity-update pair documented below. The hub's scripted traveler is a real ownership concern for future hub support, but the reference does not establish that it caused our independent trial's invisibility in Desert. Do not globally disable cutscene/takeover events or transplant the trainer patches now; that would change normal presentation without solving the third peer.

The longer-term roadmap retains larger lobbies (4, then evidence-supported 8/16), joined/left/chapter notifications, chat, private hosted rooms with counts/passwords, and synchronized permitted outfit selection. These remain deferred features. Native player identity must remain independent of both costume and UI display name; notification/chat data must be bounded and non-executable. The supported gameplay limit is still vanilla two, with three the first experimental target.

## 0.53 preparation and UI correction

User 0.52 at update 8187 declined before any appearance setter or additional draw: result 5, stage 0, check 4, layout field storage overlap, list count 3, variant 1, buffer 1296. It does not by itself distinguish repeated instance identity from unsafe aliasing.

PC material construction provides the missing evidence: `271e80` sets list count 3; `271ebd` loads the newly selected instance; `271ec2`, `271f16` and `271f4d` store that same pointer in entries 0/1/2. `native_appearance.h` now permits only an exact repeated instance/shader/buffer/size tuple **within one material**. Different-instance full/partial overlap and every cross-traveler buffer overlap still fail. Every list member is reread for ownership, and setters still execute once per parameter on the selected member. Status reports both list count and unique instance count. This is a correction grounded in native source inspection, not a claim that appearance or rendering now works.

Added ASan/UBSan cases accept a triplicated instance and mixed duplicate/disjoint members, reject partial overlap, detect replaced repeated members and borrowed-resource changes, reject cross-traveler overlap before setters, and verify the triplicated layout uses 13 setter calls with original buffers unchanged. The setter callbacks are simulated; no game functions are exercised in these fixtures.

Show Trial now keeps a bounded read-only progress panel: one exported-state snapshot per second, at most 180 timer ticks, stopping on terminal/error state. It shows chapter, game counter, pending action, delay, appearance result/stage, setter count, list/unique counts, validated consumer calls, failure code and relative bounds. A text selection pauses refresh so copying remains practical; Copy all copies the displayed text, and Status remains the full report. It reuses the validated process handle and never reattaches or queues another command. Non-atomic reads and background pausing are stated in the panel. Journey exit/read failure reports unavailability instead of presenting zero counters as success.

The retained chapter confirmation has a green banner, while Armed remains red. Keyboard-switch/mouse-jump instructions have been removed. These interface changes are compiled, but Whisky presentation is not runtime-verified. The user subsequently authorized installation: 0.53 is now installed in devishaz, with nine diagnostic hashes and 8 original-file hashes verified. Previous receipt: `Lab:work/install-devishaz-0.52.json`. No game/helper was launched or other bottle accessed. Runtime visibility and live-panel behavior remain unverified.

## Evidence boundaries

- **Runtime, user reported:** ordinary two-client movement, arrival/departure and one continuous chapter transition; temporary third geometry allocation, distinct sampled CPU storage and undrawn cleanup; 0.50 completed 600 validated consumer iterations but displayed no ghost. No three-client gameplay has been demonstrated.
- **Static, inspected here:** supplied PC executable/disassembly, compiled asset containers with readable shader source, original PC/iOS chapter records, iOS Lua/shaders and surviving native symbols, and community connection-tool source. These establish code/data structure, not successful execution of a proposed mod.
- **Proposed:** the entity/session architecture and combined experiment below. No new transport, full third Dude, appearance initialization or privacy protection is implemented by this review.

Paths below are relative to the workspace unless prefixed `Lab:` (this document's directory). PC native addresses are hexadecimal RVAs in the pinned executable, not portable addresses. `Lab:work/native/text.asm` is the private disassembly; its printed addresses add image base `0x140000000`. Do not distribute original assets or extracted source.

Rechecked executable SHA-256: `16a7c176b9cce893e0ca72c5d0e4b2e7377c4fabec179f95a5e1331b6c2a788f`. Shader `JourneyPC/Data/Shaders/S_Cloth_vs__Cloth_fs__52674.fx` SHA-256: `44d8ac22e5aceddfd4623e69336bfca325d256990793f36dfd8794c9db4c2309`. Shader line references below count lines in that file's first ASCII span of at least 500 bytes, including its initial empty line. This is source stored in an RYHP container, not proof of which GPU program executed in a particular frame.

## What 0.50 did and did not establish

At update 9477, the user reported no ghost, 600/600 validated iterations, failure 0, 539 accepted bounds/dispatch/builder calls and a matched executor return. Relative CPU bounds were approximately `(2.967, 0.500, 0.014)`, so the earlier distant CPU-placement error was absent in this run. Actual GPU vertex positions, texture bindings, draw output and final compositing remain unverified.

Local/trial named-parameter snapshots were valid, each 1296 bytes, with disjoint CPU buffer ranges. Found/finite masks were both 7. All-zero masks were local 1 and trial 7: `dudeDissolve` was zero in both; `dudeCenter` and `dudeInvTorso` were populated only in the local sample. This identifies incomplete trial initialization, **not the cause of invisibility**. These were CPU material buffers sampled after a consumer return, not GPU constants captured at drawing time.

`model`, `modelViewProj` and `viewprojection` being absent from this named-parameter lookup is not evidence that transforms are zero. The PC source declares renderer semantic parameters separately (lines 564–577). A full transform audit must follow that binding path and the active skinning variant.

## Opacity and visibility audit

The embedded PC `Persistent.lua`, extracted and hash-checked by `Lab:startup_resources.py::extract_persistent`, declares `MeshDudeRemote` with `REMOTE DECORATION DUDE AO SAND ALPHATESTENABLE`. The inspected PC shader source has those same defines at lines 3–9. iOS `Data/Resources/Persistent.lua:37–40` has corresponding local/remote/frozen declarations. This correspondence justifies comparing the relevant shader branches, not transferring iOS graphics APIs or native layouts.

| Input/path | PC evidence; corresponding iOS evidence | Consequence for the trial |
| --- | --- | --- |
| Vertex alpha / cloth LOD dither | PC 752–809, 949–960, 2028–2034; `JourneyIOS/Cloth_fs.cg:438–444` | Mesh color alpha survives to the fragment stage and is thresholded against a noise texture. Geometry counts and finite pose matrices do not check this input. |
| Atlas alpha and UVs | PC 2049–2057, 2175–2179; iOS fragment 459–467, 587–589 | Texture alpha still contributes to final clipping. Must establish actual atlas bindings and UV inputs; a material allocation alone does not do so. |
| `invisFrom`, `invisTo` | PC 1922–1954; iOS fragment 333–364 | Both are forced to 1 in the selected DECORATION branch. Searching for an unset value of these two variables is not the useful next test. |
| `dudeDecoGlow` | PC 1933–1936, 2179; iOS fragment 344, 589 | Selects/blends atlas layers. Zero is a normal possible value, not automatically invisibility. Read the executed expression rather than relying on the nearby comment about alpha. |
| `dudeDissolve`, center Y | PC 2429–2442; iOS fragment 840–853 | With dissolve zero and ordinary finite nonnegative mask samples, the dissolve alpha multiplier stays visible even if center Y is zero. Center correction alone is therefore not a supported explanation for this run. Invalid samples/NaNs remain a separate issue. |
| `dudeInvTorso` | PC vertex 862–883; `JourneyIOS/Cloth_vs.cg:170` onward | Used for torso-relative lighting/occlusion; the DECORATION branch also overrides an occlusion term. A zero matrix is incomplete setup and may make intermediate calculations invalid, but it is not a direct world-position or alpha setter. |
| HSV, texture/detail/mask inputs | PC 1956–1958 and later color calculation; iOS fragment 366–368 and chapter records below | Missing values can affect appearance. Need the normal setup sequence, not arbitrary bright/opaque replacements. `alphaHardness` is declared/stored in the inspected material data but no additional use was found in this PC shader source. |
| Final clip | PC 2449–2475; iOS fragment 860–889 | Final alpha includes vertex alpha and transformed texture-derived alpha, then ALPHATEST discards below its threshold. Do not disable clipping to conceal missing bindings. |
| Prepass and later render state | PC 2457–2468; Persistent.lua prepass definitions and stencil values | PREPASS is a separate conditional path; its presence in source does not mean it ran in the private trial. Matching one consumer does not establish correct depth, stencil, render target, palette upload or final composition. |

The local and trial sampled draw records both have 5940 indices and enabled gates. That rules out an empty count at those sampled points; it does not establish a complete traveler. Robe/scarf cloth, effects, active animation and sound have separate setup/update paths.

The skinned vertex path explicitly uses `GenerateBoneMatrix` and indexed weights (PC 615–645, 794–809). Distinct, finite CPU animation matrices do not establish that the trial's palette reaches that shader. Another two-player rendering assumption is `117790`, which writes two matrix blocks at renderer-related state +`180`/+`1c0` and looks up `depthExtra`. Its effect on an added traveler is unresolved. There is no chapter-select/matchmaking predicate in the inspected Cloth shader source; this does not rule out native scheduling or pass-state differences by chapter.

## Missing normal appearance setup: stronger PC/iOS correspondence

Desert's `InitLocalDude` trigger references `runMesh` and `runMeshRemote` definitions, not just a bare shader name. The PC extracted `Lab:work/pc_scripts/Level_Desert/TriggerInstances.lua:5–12` matches `JourneyIOS/Data/Scripts/Level_Desert/TriggerInstances.lua` after CRLF normalization. The normalized SHA-256 is `fd404d5d64e318e132282fa19d340d3df0eeaf9b2c90450d6e9b480019172705`.

The referenced records are readable in `JourneyIOS/Data/Scripts/Level_Desert/DecorationMeshInstances.lua:533–668`. They identify `MeshDudeRemote`/`MeshDudeLocal` and include these bindings:

| Parameter | Resource/value in the records |
| --- | --- |
| `textureAtlasA` / `textureAtlasB` | `Ninja` / `NinjaGlow` |
| `textureDetail` / `textureMask` | `ClothDetail` / `NinjaMask` |
| `envOccCube` | `WhiteCube` |
| `specular` / `alphaHardness` | `(0, 1)` / `5` |
| `localDudeGlow`, `remoteDudeGlow`, `dudeDecoGlow` | Initially zero |
| `hsvHue`, `hsvSaturation`, `hsvValue` | Initially zero in these records; later native style update must be considered |

This is not based only on iOS names. Both platforms supply `Data/Scripts/Level_Desert/DecorationMeshInstances.lua.bin`, each 1,212,304 bytes. Header count 574 and file length fit a 16-byte header plus 574 records of 2112 bytes. The referenced remote/local record boundaries are file offsets `0x5ad0`/`0x6310`, with identifiers at record+`0x70`. Their parameter names, resource strings and remaining bytes match across platforms; **only record-relative bytes 2 and 3 differ**, inside the unresolved first eight-byte field. Do not interpret or transfer that field as a runtime pointer. This is strong record-level correspondence, not a complete binary-format decoder.

PC binary SHA-256: `cd8c6cfff84e582578831bf3f4087cef5d1a08b775efdb1dc201a81424c87abd`; iOS binary SHA-256: `a3e3d0f8c69fee5d975be3c20109aff6995f1b289522698d11689f2564cab592`.

Relevant PC native sequence:

1. Chapter initialization calls the full Dude initializer `a32c0` for the two original travelers. Its resource helper `a4a60` initializes the primary mesh and animation controller but clears the appearance-definition pointer at Dude+`2238`.
2. The trigger-shaped function `173590` assigns definition pointers to local Dude+`2238` and remote barn+`16928`, resolves their shader resources and initializes the respective primary meshes (`173671`, `1736aa`, `1736e9`, `173719`). Its field use and local/remote sequence agree with the supplied `InitLocalDude` record; the function's complete type attribution remains an inference.
3. The normal Dude update calls `9d9a0` at `a6b37` when the definition pointer exists. That helper walks bounded-by-data parameter records: type 2 resolves texture resources and calls material texture setter `273640`; type 1 uses vector setter `273200`; type 0 uses scalar setter `273a00`. Mesh CPU update follows at `a6bcb`.
4. `a6350` calls `a9470` at `a6cf4`. The latter sets dissolve, center, inverse torso and glow inputs. Center/bone lookup uses `spineB`, also present in PC `Vars.lua:227`.
5. The style path `2177c0` selects the two embedded travelers and calls `217a70`, which writes HSV and also updates cloth appearance. PC `Vars.lua:310–317` supplies local/remote style defaults 0/1/1. The related path `217c30` changes atlas resource names for outfit selection, including white-robed textures. This cannot be replaced by copying another player's whole mutable material.

The trial's current mesh/material/idle-CPU preparation omits this per-traveler definition/style sequence. That supplies a concrete broader hypothesis: **the mesh lacks part of its normal appearance bindings**, including textures that directly affect alpha. Whether its live texture bindings are absent, incorrect or already inherited is still unmeasured. Do not claim a texture fix until that distinction is established.

iOS `JourneyLaunch` retains useful getter/reflection symbols, including local/remote getter wrappers, `Dude::GetPos`, `Dude::SetPos`, `Dude::SetOutfit` and InitLocalDude variable registrations. It does not expose complete named implementations of these subsystems through the inspected symbol subset. Its GameKit import also warns against transferring the iOS connection backend. Native PC object offsets remain grounded in the PC image.

## Connection and synchronization audit

The existing Lua-facing lifecycle and community browser select a single companion. `journey_detour-main/journey_detour_v4/igig/lobbybrowser.cpp:118` calls `ConnectionBarn__DialOut` on Matchmaker+1536 (`0x600`). It neither allocates another traveler nor adds per-peer state. PC Vars `Matchmaker.kMaxRoomSize = 16` is not evidence for sixteen rendered/connected gameplay travelers; the native structures below still contain one peer path.

| Lifecycle boundary | PC evidence | Required change for three real clients |
| --- | --- | --- |
| Connect | `94cb0` outgoing and `937c0` incoming paths guard an existing connection, reset `96600`, use embedded client +`10` | Do not merely bypass the guard: it would reuse/reset state belonging to the first peer. |
| Transport send | `976b0` obtains interface through `96ca0`, sends to client+`11788`, channel argument 0, increments client sequence and advances its event queue | One sender/destination context is currently selected. Multiple peers require separate bookkeeping or a mod-owned envelope/dispatcher. |
| Transport receive | `977a0` polls the interface's +8 method, reads through +`10`, channel 0, caps the read buffer at 512 bytes, obtains a sender identifier | These signatures agree with the supplied legacy `ISteamNetworking` header. The executable also requests `SteamNetworking005` at `29b832` and stores it at service+`40`; the complete assignment into the getter's service object is not yet traced. No claim of relay-only access follows from these facts. |
| Sender demultiplexing | In the inspected receive routine, the returned sender storage is subsequently reused for acknowledgment data; no comparison to the selected peer was found there | Other session-acceptance code may constrain senders. Nevertheless, duplicating this receive routine would consume a shared channel queue without routing each packet to a distinct entity. Intercept at a boundary that still has sender identity. |
| Reliability | Receive decodes 16-bit sequence, 16-bit acknowledgment and 32-bit acknowledgment mask; `121c90`, `121ec0`, `10c770` update the single client's histories/event acknowledgments | Keep acknowledgment and duplicate/order state separate per peer/connection epoch. This is not a complete verified wire decoder. |
| Traveler state | Game pump `de050` selects local barn+`c0` for writing and the sole remote staging area barn+`28d30` for reading, via `117600` and `a1660` | Use distinct incoming state for each participant. Serializing or swapping a whole Dude object would include pointers and unrelated ownership. |
| Remote application | `118730` applies the sole staging state to the original remote. UpdatePos `118030` and UpdateData `118690` explicitly process two Dude objects | Additional entities need their own application and update scheduling; connection establishment alone cannot display them. |
| Shared variables | `14b3b0` scans 32-byte records, queues dirty variables, clears their dirty flags at `14b461`, serializes IDs/payloads, updates shared pending/send records | Calling the same serializer once per peer is not independent fan-out. Clearing dirty flags and sharing acknowledgments can lose updates for another peer. |
| Chapter gate | Game+`260` holds SynchVarBarn; `de3cf`/`de3d6` compare local/remote chapter bytes +`2010`/+`2011` before reading traveler/event state. `LevelUid.lua:2–9` describes this filter | Track each participant's chapter and transition generation separately. Do not disable the gate to make another peer's data apply. |
| Events | `de050` processes event records after decoding, including indexed game-object dispatch through Game+`258` and virtual +`78` | These are shared gameplay operations, not just movement. Determine per-event ownership, deduplication and chapter validity before forwarding between three clients. |
| Role | `97bb0` orders two 64-bit values and sets a HOST/CLIENT flag | A two-party role label does not prove one peer authoritatively simulates every traveler, trigger, cloth and progression subsystem. Global authority remains unresolved. |
| Chapter lifetime | Setup `d7e20` initializes both original travelers; teardown `d9fad`/`d9fbc` cleans both. GetRemoteDude is gated by state/chapter flags | A mod participant's stable identity must survive recreation of native entities. Reconnect must invalidate old packets and references. |

Initial state serialization details are now clearer: `a1660` begins with two three-float vectors, several raw scalar floats and narrower quantized fields; it is not a serialized render object. `117600` handles separate metadata. Full field meanings, exact framing, malformed-input behavior and event ownership are still incomplete. Do not expose the native deserializer to new untrusted packets based on this partial map.

The underlying Steam-style API accepts a destination per send and returns a sender per read. That makes multi-peer transport plausible at the API layer, **not established in this game's wrapper**. The existing legacy header's relay option permits fallback after direct connection failure; it is not a privacy guarantee. No service deployment or external API access was performed during this review.

## Interaction and progression constraints

PlayerBarn allocates two embedded Dude objects at +`b0` and +`146f0`; the separate Dude factory at `abcd0` allocates `0x14640` bytes. The retained trial tests have not established safe initialization and execution of an additional full Dude. Existing pair updates pass the other Dude as an argument, so proximity/robe recharge cannot be assumed to generalize by adding a render call.

PC `SoundEffectData.lua:24,28` has distinct local and second-player chirp entries; that alone does not prove a sound-instance limit. `EnvFX.lua:37–39` targets one remote player. Desert trigger data contains `HijackRemotePlayer`, `IsMultiplayer` and explicit `NetworkDisconnect` operations. Those are concrete audit points for cutscenes and transitions, not permission to rewrite all level scripts.

Chapter-select transitions disconnect ordinary companions by design. Existing chapter-select trial preparation is a diagnostic allocation boundary, not a proposed multiplayer joining flow. The later three-client test must include ordinary forward chapter progression and must distinguish scripted travelers from connected participants.

## Revised architecture and next substantial experiment

Use a native entity adapter plus a bounded participant registry. A participant maps to its own remote state, animation/appearance state, native entity handles and connection/chapter generation. Do not make the third traveler a succession of replacements for the original remote slot. Keep gameplay identity independent of mesh pointers, pool slots, Steam display names and session ordering. Registry code can support variable counts while the initial experimental limit remains three total clients; four/eight/sixteen remain untested targets.

The renderer should reuse normal resource/appearance/animation paths where ownership is established. Immutable textures can be referenced through the game's resource system; mutable material buffers, animation state and cleanup references must remain per traveler. Do not replay InitLocalDude wholesale, because it explicitly writes the two original slots. Do not invoke the full Dude update with guessed subsystems or copy its pointer-filled object memory.

For the first three bottles on this Mac, a private loopback transport is a viable development fallback if adapting the game's peer wrapper is more expensive. Keep its message format transport-independent, membership authenticated and buffers bounded. It would carry validated traveler data into the same entity adapter, not Lua or native addresses. No broker or socket is implemented yet, and this is not a public-IP-hiding mode. A transport coordinator does not decide shared gameplay authority automatically. Existing pair event traffic must not be blindly broadcast to everyone.

The next implementation work is one **complete appearance and state-routing slice**, with offline work preceding another user run:

1. Finish the native named-texture binding/ownership map and identify the normal primary render pass and skinning upload inputs. Compare local, original remote and trial together: shader variant, resource identity/presence, HSV/glow/dissolve, vertex alpha, palette binding, draw pass/depth/stencil. Use one bounded diagnostic collection, not a separate release for each field. Do not reintroduce the four crash-associated 0.43 draw-wrapper hooks merely to collect this data.
2. Prepare the trial's complete required appearance through validated bindings, with its own mutable storage and the game's existing resource lifetime rules. Apply the normal torso/center calculation as part of that setup, not as an asserted invisibility fix. Preserve original travelers and report setup failures before drawing. If bindings are already correct, investigate pass/palette submission rather than cycling shader guesses.
3. Alongside that work, implement the bounded participant/state/epoch layer and a synthetic routing harness for two remote identities, reordered/duplicate messages, stale chapters, reconnects and resource caps. It can be developed without waiting for visible rendering. Its tests prove routing only, not gameplay.
4. Connect independently captured third-client state to the native adapter when the additional entity is viable. First demonstrate independently moving remote entities, then complete chirp/proximity/interaction and forward-transition behavior. A three-machine/bottle packet exchange without both companions rendered is connectivity progress, not Milestone B.

The next user experiment should have a single clear pass condition: a nearby additional traveler with the expected appearance, while the original traveler remains correct, plus the combined setup report if it fails. Avoid another request merely to reconfirm 600 consumer returns. End the experiment after recording until cleanup after rendering is verified. Then promote to the real three-client scenario; no launcher polish, chat, alternate characters or larger advertised limits first.

## Implemented components after the review

The next development pass added working C components, not a new game package:

- `Lab:diagnostic/companion_session.h`: fixed-capacity participant registry and explicitly encoded movement/chapter messages. Each remote has its own identity, receiver-issued admission lease, sequence history, chapter generation, movement snapshot and timeout. The initial fixture uses three total participants. Sixteen is only the compile-time storage ceiling, not a supported lobby size.
- `Lab:diagnostic/native_appearance.h`: checked capture of the original thirteen-parameter appearance definition, five resolved texture references and named material destinations. `jc_appearance_prepare` constructs a complete appearance plan, takes current scalar/vector values from the source material (including style values that supersede chapter defaults), invokes supplied setter callbacks for the separate trial material, and verifies original buffers plus the destination afterward. It does not blindly copy an entire material or a pointer-filled Dude.
- `test_companion_session.c`, `test_native_appearance.c` and `check_companion_components.py`: reproducible native sanitizer fixtures and Windows x64 cross-builds using the already approved toolchain. They do not start Journey, install a DLL, contact a server or update the installed package.

### Session interface and security boundary

`jc_session_join` is a trusted membership operation, never an incoming packet type. `jc_session_receive` requires the participant ID and lease already associated with the transport connection; it checks that the encoded ID/lease agree. **Passing a packet's own ID as the trusted argument would defeat that check.** There is no authentication handshake, encryption, socket, invite system or transport adapter implemented here. This component must not be presented as a secure network listener or directly exposed to untrusted datagrams.

The caller supplies a fresh nonzero 128-bit room value, nonzero local participant ID, total capacity and monotonic clock. Initialization and all mutation occur on one owning thread. Participant IDs are stable across reconnect; receiver-local leases are new on every admission and never reused within that session. Packets for another room, an old lease or an unauthorized identity fail before changing movement/chapter state. Reinitialization must use a fresh room value. The native entity map must key its lifetime by both participant ID and lease, not a reused slot index.

Version 1 is an experimental mod-owned format, **not Journey's recovered native packet format**. All integers are little-endian, floats are IEEE binary32, and no C struct is copied onto the wire:

| Byte offset | Field |
| --- | --- |
| 0 / 4 / 6 | Four-byte magic `JCS1` / uint16 version / uint16 type (1 transition, 2 motion) |
| 8 / 10 | uint16 exact length / uint16 reserved flags, required zero |
| 12 | 16-byte room value |
| 28 / 36 / 44 / 52 | uint64 participant ID / receiver-issued lease / sequence / chapter generation |
| 60 | uint32 chapter ID, currently restricted to shipped IDs 0–8 |
| 64, motion only | Seven float32 values: position XYZ, velocity XYZ, heading in radians |

Transition packets are exactly 64 bytes and motion packets 92. Sequence numbers strictly increase without wrap; reconnect before exhaustion. A transition must increase the chapter generation and clears the old movement snapshot. Motion cannot implicitly change chapters. Re-entering the same chapter still needs a new generation, so old packets cannot reappear as current state. The caller must provide reliable/ordered transition delivery or its own acknowledgment/retry behavior; this module does not implement reliability. `jc_session_motion` also filters by the observer's current chapter, so it does not authorize cross-chapter entity updates.

Limits are finite and explicit: no heap allocation, 120 attempts per peer per one-second fixed window, exact sizes/types/version, finite numeric values, conservative diagnostic position/velocity/heading bounds, and no code, paths or arbitrary command strings. These numerical bounds are prototype policy, not measured Journey limits or an anti-cheat system. Invalid messages consume that peer's rate budget but do not update its gameplay state or keepalive timestamp. `jc_session_expire_one` returns an expired ID/lease for entity cleanup; the adapter must consume cleanup events before reusing native resources. Other participants' state remains independent.

### Appearance preparation and remaining integration requirement

This component now handles the complete known Desert appearance schema rather than only center/inverse-torso uniforms. It accepts exactly thirteen known rows, checks native parameter kinds, finite interpolation values, bounded tables/buffers, resolved resource links, texture handle/generation/type fields, rereads changing metadata and rejects overlapping parameter destinations. It reports missing parameters separately from zero values. Multiple shader instances/passes and an unexpected chapter schema currently decline; they are not silently treated as the known layout.

Before setters can run, local/original-remote/trial parameter buffers must be disjoint. Required atlas/detail/mask bindings must exist, and source texture bindings must agree with the captured definition. Only static allowlisted parameter names reach callbacks. Afterward it compares both original buffers byte-for-byte, requires the trial's named values to match the plan and rejects writes outside those destinations. A failed setter or postcheck records the stage; it does not attempt unsafe rollback using stale pointers. These checks detect several classes of unwanted writes; they are not protection against a native crash or an arbitrary faulty callback.

Additional pinned PC findings from this implementation:

- Material texture setter `273640` resolves a named parameter through `2daee0`, reads it through `2dae60`, and delegates to `2db010`. The texture binding occupies 24 bytes: type/flags, generation stamp, owner pointer and handle pointer. Type-1 bindings carry a high flag derived from handle+`94` via `364f80`; the reader includes that flag in equality checks.
- The helper called at `3630f0` is **just return-zero in this binary**, not an ownership acquisition or effective lifetime check. The setter alone must not be assumed to keep texture resources alive. This corrects the preliminary interpretation of the call as reference management.
- The hash-checked PC Persistent.lua does declare the needed resources: WhiteCube at line 275, ClothDetail at 514, and Ninja/NinjaGlow/NinjaMask plus the white variants at 732–737. This is a useful lifetime lead: these are startup resources, not names inferred from iOS alone. Their declarations do not by themselves pin the resolved runtime texture/handle objects against replacement.
- Vector setter `273200` writes a full 16 bytes at `273306`/`27330a`. The preparation component consequently requires a 16-byte destination for `specular`, even though the chapter source calls it float2. If the live descriptor advertises only eight bytes, it declines pending a verified layout/padding adaptation; it must not write through the boundary merely because the original function would.

At that component-only checkpoint, the real setter callbacks were not connected to `probe_dll.c`. They require hash-pinned entry points, the owning game thread, a stable chapter/resource lifetime, and validated live destination widths. Texture lifetime is not provided by this portable component. The existing untested torso draft remains deferred; this component does not claim to supply bone transforms, skinning uploads, cloth, animation, render-pass/depth/stencil setup or a full third Dude.

The integration task identified at that checkpoint was: bind the prepared appearance operations to the pinned native setters under the trial's lifetime guard, establish the texture-resource lifetime and the live vector destination width, and combine that with the pending pass/palette comparison. In parallel, connect the completed session core to independently captured client state through an authenticated transport adapter. Neither task needs another repetition of the existing 0.50 ghost test.

### Verification performed

Run `python3 JourneyPC-CompanionsMod/CompanionLab/diagnostic/check_companion_components.py` from the workspace. It only creates outputs under `Lab:work/companion-components` and existing workspace tool caches; no dependency downloads occur.

- Host AddressSanitizer/UndefinedBehaviorSanitizer passed: three simulated client registries, 1,800 independently routed motion deliveries; identity isolation, capacity, replay/reorder, chapter/revisit, reconnect, timeout, clock/rate/sequence exhaustion, malformed lengths/types/version/non-finite inputs, and 10,000 deterministic packet mutations.
- Host sanitizer appearance fixtures passed: reader rejection cases and seven complete preparation scenarios, including thirteen parameter writes/five texture references, source style preservation, aliased buffers rejected before writes, invalid source bindings, non-finite values, partial setter failure, original-buffer corruption and out-of-range trial writes. These setters modify synthetic memory only.
- Both fixture executables cross-compiled with warnings as errors for Windows x64; PE machine fields checked as `0x8664`. Windows executables were not run.
- `Lab:work/companion-components/verification.json` records source hashes and those outcomes. No generated executable or test result establishes rendered pixels, real-client traffic, authentication or three-player gameplay.

## Native integration: candidate 0.51

`diagnostic/probe_dll.c` now invokes `jc_appearance_prepare` before the first trial CPU update/draw. It runs under the existing initialized-mesh lock, owning-thread, chapter, resource and private-queue checks. The new callable entry points are hash-pinned and prologue-checked: scalar lookup `273a00`, vector setter `273200`, texture setter `273640`. No new detours or draw-wrapper hooks were added.

The supplied PC Cloth shader source declares `float4 specular` (ASCII source at file offset 72229 in `S_Cloth_vs__Cloth_fs__52674.fx`), consistent with the 16-byte native store despite the chapter definition using a two-component value. The live descriptor guard remains required.

The callback accepts only a static allowlisted name and the trial material. Scalar writes require the native lookup to return the exact previously checked destination; vector writes also check that destination and require a full 16-byte declared field. The initial plan accepts exactly the known thirteen-row schema. Current local color/glow values supersede chapter defaults; texture references must agree with the normal source material. Both original material buffers must remain byte-for-byte unchanged, and trial writes must stay inside the named destinations. Failure after writes starts requires restart; there is no blind rollback or destructor on uncertain resources.

Texture lifetime is deliberately limited: this prototype **borrows**, rather than retains, the original source resources. `jc_appearance_borrow_begin/check` records the definition, texture identities/handles/generation/type and source/trial buffer ownership. It rechecks these before each added consumer call, including source-definition identity. Normal scalar/color changes are allowed; changed texture bindings, handles, buffers, descriptors or chapter stop further added draws. These are bounded same-thread CPU checks, not an atomic GPU lifetime guarantee. The existing chapter-cleanup hook quarantines the trial once Show Trial begins; resources remain allocated until process exit. A future persistent companion needs real ownership and render-completion cleanup.

Preparation has its own 500 ms maximum read budget; normal per-draw checks share the existing 100 ms budget (first pose preflight 250 ms). Budget expiry declines rather than bypassing checks. These limits bound diagnostic work but do not establish production performance or protect against a fault/block inside native calls.

The helper exports and reads `JCProbeAppearance` and shows one combined, shorter Status report. Results distinguish pending, no writes, setters started/returned, postchecks, source mismatches, narrow destinations and changed resources. Preview failure 20 means appearance preparation, 21 means the borrowed-resource check. Existing pose/placement, shader and renderer summaries remain; Status does not label consumer returns as GPU draws. All actions remain available in the dialog, with the existing red Armed state and Copy all.

Verification in this workspace:

- Appearance fixtures passed ASan/UBSan, including preparation failures and lifecycle checks for generation changes, replaced trial texture bindings, replaced buffers, aliasing and normal scalar changes. Setter calls remain simulated in these fixtures.
- The existing three-simulated-client routing suite still passes, including 1,800 deliveries and 10,000 malformed-message mutations. No sockets or real game clients were involved.
- Both component executables and the actual helper/DLL cross-compiled for Windows x64. The native appearance entry points and new status export are checked in package verification. All 14 Python/package checks passed.
- Status has 61 matched format arguments; conservative output bounds are 4,469 characters / 4,499 after CRLF conversion, within 7,500/8,192 buffers.
- The 0.50 archive remains unchanged. The new allowlisted package contains nine diagnostic/license files and no proprietary game assets. No original PC/iOS files were edited.

The next runtime pass uses Capacity Test before startup, Continue into Desert, Prepare Trial, red Armed, chapter select then the Desert portal, Trial retained, open ground and Show Trial. The goal is a visible additional mesh with the original traveler still correct, plus the combined preparation report. Quit after copying; no chapter changes after Show Trial. If appearance passes but visibility fails, the remaining pass/palette/torso path still needs investigation. No third-player networking is added by 0.51.

## Material-list correction: candidate 0.52

User 0.51 report: update 10201, selected/current Level_Desert, preparation/allocation 5/4, appearance result 4, stage 0, check 4, zero setters in 4 ms, preview failure 20 and zero consumer calls. This confirms the trial binding reader declined after the thirteen-row source plan was read. It does not identify the particular range check. Zero masks are unavailable outputs after a failed read, not evidence that all parameters are missing. Both applications were then confirmed closed.

Pinned native code provides a concrete reader correction:

- Scalar lookup `273a00` (`273a69`–`273aa1`), vector setter `273200` (`273289`–`2732c6`), named texture lookup `2daee0` (`2daf50`–`2daf93`), texture binding read `2dae60` and texture write `2db010` all distinguish count one from a larger collection. For count one the instance is inline at collection+8; otherwise collection+8 points to an array and they select its first instance. They do not reject every count above one. The previous read-only `native_shader_inputs.h` already understood that selection, while `native_appearance.h` unnecessarily required count exactly one.
- `2db010` builds a 24-byte texture binding on its stack and passes it to `270a80`. At `270a9c`–`270acc`, the copy length is the descriptor's low thirteen size bits. Merely requiring size at least 24 is insufficient: a larger descriptor could copy beyond that binding. The new guard requires exactly 24 for textures; scalar/vector destination checks remain enforced.

`jc_appearance_bindings_read_detail` now supports one inline instance or an indirect list capped at sixteen members. Every member's instance, shader, parameter-buffer identity and byte range are captured and reread. Buffers must fit the existing 4096-byte limit and must not overlap within or across the three traveler materials. Native setters still operate only on member zero; this is not a newly implemented all-pass material updater. All selected original buffer bytes are compared after preparation; secondary metadata is compared rather than claiming a byte-for-byte check of every secondary buffer.

The complete inventory participates in pre-setter, post-setter and borrowed-texture lifetime comparisons. A replaced secondary buffer, changed count/list or overlap with another traveler's storage declines before further drawing. Neither the sixteen-member cap nor additional instance storage implies support for sixteen players.

`JCProbeAppearance` and the helper now include bounded, non-address diagnostics: layout field, allowlisted parameter name, variant, instance count, parameter-buffer byte count, descriptor table/row, offset and declared/required width. Width details are meaningful for a width rejection; other fields describe the last inspected metadata. Diagnostics never override a guard. These details cover the ambiguous 0.51 paths in one package, rather than requesting a separate run for each possible field.

Verification: ASan/UBSan fixtures cover both storage forms, two/sixteen instances, changed first and secondary member pointers, changed secondary buffers, same-material overlap, cross-traveler overlap through a secondary member, untouched secondary fixture buffers after thirteen simulated setters, oversized texture copies rejected, and exact parameter-width/count diagnostics. The existing preparation and session tests pass. Both component Windows builds and the actual helper/DLL build pass; all 14 Python/package checks pass. Status has 71 matched arguments and a conservative 5366-character / 5399-CRLF bound, below its 7500/8192 limits. 0.51 remains archived. No new game run or true third client is claimed.

The corrected native selection is established by static code and fixtures. **It remains a hypothesis that the single-instance restriction caused this particular 0.51 rejection.** The next runtime report can confirm the actual layout or name another rejected field. The goal remains completing appearance preparation and seeing a nearby mesh, then mapping independent third-client state to a real additional companion; no transport or extra rendering hooks were added here.

## Workspace state

Candidate 0.52 was built, locally tested and installed in devishaz after the scoped outside-workspace approval. All nine diagnostic hashes and eight original-file checks passed. Both applications were confirmed closed; none were launched. This update did not access rebishaz or smooviepuffs. The installed version has not yet been exercised in Journey.

`work/install-devishaz.json` records 0.52; `work/install-devishaz-0.51.json` and the unchanged 0.51 ZIP preserve rollback. The 0.52 ZIP is 244346 bytes, SHA-256 `0eb92cde7f62d4ecded7f7984b4be58d7aee8a178c09813f34de0159db69c2d4`. Restarting Journey without attaching returns to vanilla; removing the diagnostic folder removes this installation. No original executable or asset patch was applied.

`diagnostic/native_trial_shader.h` remains an unintegrated, untested torso draft. It is not included by the probe and its proposed writes are not in the package. The added snapshot fields in `native_shader_inputs.h` do not activate those writes. Do not treat this as full appearance/pose/cloth or full Dude initialization: the integrated thirteen-parameter setup and the unresolved torso/skinning/pass work are distinct.


## 0.55 visible body and 0.56 local pose adapter

**User runtime observation:** a floating extra body appeared in Desert in 0.55, without cape or scarf. The supplied active snapshot had 211/600 validated consumer calls, failure 0, all thirteen appearance parameters matching (8191), unchanged original material buffers, accepted torso writes and source/trial shader masks 7/7/1. The cube binding's cached/current slots were 0/1 of 3, validating that the previous immutable-index assumption was too strict for this case. The body later disappeared while the user typed; the active snapshot does not establish the terminal cause. The bounded static window is a plausible explanation. Both applications were closed afterward.

**Static native evidence for repeated movement:** `work/native/text.asm`, pinned PC RVAs `263f00..263f42`, copies mesh+530 into model root+20; `263f58..26407a` updates CPU matrix destinations. The same MeshUpdate routine's private queue assignment at `264099..2640d5` increments active count and checks the stored model; `2640bb..2640d5` skips another acquisition for the same stored model. This supports reusing the retained pair after the consumer resets count to zero. The adapter still checks both model/control, reference count, count=1 after update, root-transform agreement and the ledger, then relies on existing post-consumer checks. It does not infer GPU completion or allow destruction after rendering. Root writes are limited to the separately allocated trial mesh, with original local/remote transforms compared before and after each subsequent CPU update.

`diagnostic/native_trial_shader.h` now has a repeated-update entry point. The first write still requires zero initial trial torso inputs. Subsequent writes require exact agreement with the previously prepared center/inverse, independently inspect current material storage, and compare whole original buffers and allowed trial changes. Invalid prior values decline before writes. `probe_dll.c` obtains the new spine transform after each accepted mesh update. It retains full input scans and time budgets; no animation activity or cloth update is enabled.

**Implemented transport, not yet game-verified:** `companion_pose.h` defines one 152-byte packet: magic/version/length (8), pinned-build prefix (16), random process identity (16), sequence (8), chapter/reserved (8), 16 binary32 transform fields (64), HMAC-SHA256 (32). `pose_link.h` supplies Winsock IPv4 localhost and BCrypt authentication. The first authenticated source identity is pinned to the trial for this process. There are no peer-controlled pointers, script strings, object paths, commands or asset downloads. Replay, foreign identity, incompatible header/build and invalid transforms fail before the native adapter sees the sample. This prototype is not the larger session protocol; it is a bounded live source-to-entity adapter experiment.

Publisher: after the original GameUpdate returns, validate PlayerBarn at game+1e0, local Dude type at barn+b0 and local mesh type at barn+210, double-read the affine transform at barn+740, recheck barn and chapter, then publish at most twice per second. Receiver: at most four nonblocking reads/update, 32 parser attempts/second, fixed storage, same chapter required, 5-second receive freshness, ten-minute socket session. Rendering uses the normal owning update/render thread and existing lifetime lock. Wrong chapter and stale input hide the body; chapter cleanup after rendering remains restart-only. Source restart receives a new identity and is rejected rather than silently stealing the retained entity.

The receiver applies the source's exact world-space pose. It does not add the previous +3 offset, copy the vanilla remote's state or call the native single-peer deserializer. A local publisher send count is not an acknowledgement; recipient acceptance and applied-pose counts must be checked separately. Paused bottles and queued datagrams can delay observation. Existing native matchmaking remains active; an incidental vanilla connection can show the same source twice and is not a third independent participant.

Verification: real CommonCrypto HMAC known-answer test on macOS, authenticated IPv4 localhost UDP round-trip with synthetic endpoints, ASan/UBSan packet/identity/replay/chapter/staleness/rate checks, every truncated packet length, 10,000 tampered packets and authenticated non-finite matrices. The Windows fixture uses BCrypt and cross-compiles but has not run in Wine. Existing simulated three-member session tests remain separate; neither fixture constitutes real-client gameplay. Appearance fixtures additionally exercise 120 consecutive checked torso updates and rejection of stale expected inputs without another write. The actual native MeshUpdate call sequence still needs the two-bottle runtime test.

0.56 adds no new external dependencies or detours. Original PC/iOS sources and community reference mods are unchanged. Full traveler initializer/cloth, reciprocal views, interaction synchronization, safe rendered cleanup, transitions/rejoin, four-player and larger-lobby support remain pending.


## 0.56 runtime transport and 0.57 checkpoint correction

User report: smooviepuffs source state 1/error 0, 76 sends, capture code 0; devishaz receiver state 1/error 0, 137 accepts, no rejects, Desert, freshness 1/age 96 ms. The snapshots are asynchronous. Receiver applied poses 0, consumer returns 0, render failure 15/stage 0, appearance 0/stage 3, torso pending. No extra body was seen. This establishes real cross-bottle authenticated pose delivery while both vanilla companions were disconnected; it does not establish application to a rendered entity or three-client co-op.

Read-only inspection of devishaz log `jc-probe-20261008-212835-2320.log` found 66 total lines. Its last successful preparation record is appearance result 0/stage 3, thirteen setters, originals unchanged, masks 8191/3720/8191. The next record reports preview failure 15. The intervening production source-detail format is 160 bytes for match=891 and 161 for match=8191 with otherwise accepted values; `diagnostic_checkpoint.h` rejects length>160 without attempting file I/O. This reproduces a concrete format-limit defect. The original panel omitted exports 115..117, so its precise live checkpoint subcode was not supplied and cannot be inferred conclusively from the log. The record cap is 128, well above the total log length in this run.

0.57 splits the result/source/texture/layout/descriptor metadata into compact shared format strings. Regression fixtures execute those actual production formats at 891/8191 and INT_MIN/UINT_MAX extremes through the checkpoint writer, alongside cap, oversize, partial write and flush tests. No cap is raised and no failed checkpoint is ignored. Receiver Status now includes checkpoint attempts, first failure code, Windows error and pose-preflight step. `preview_fail` preserves the first render failure rather than allowing later chapter cleanup to replace it. Source Status now reports successful published chapter and does not imply it should allocate/render a body. No native call, transport packet, source identity or ownership guard is changed.

0.57 verification completed: five host sanitizer fixtures and Windows x64 fixture builds, actual helper/DLL build, and 14 Python/package checks passed. Both installed nine-file folders match the same 0.57 ZIP; all 16 recorded original-file hashes match. All four applications were confirmed closed before installation. No game runtime test was performed by the agent; the next observation is whether a received pose reaches the already demonstrated visible-body path.


## 0.57 visible linked body; 0.58 read-path optimization

User observation: body appeared in devishaz for approximately ten seconds with poor performance, then disappeared. Receiver snapshot: 55 accepted packets, no rejects, fresh Desert input age 205 ms, 46 applied poses, distance 0.361, 278 consumer returns, failure 21/stage 4, appearance and torso 0/3, checkpoints 72 with no failure. Source's separate snapshot: 111 sends, no error. The user later clarified they did not try moving smooviepuffs. This confirms visible rendering while independent-process state reached the native adapter, not observed controlled motion or three-client gameplay.

Read-only log `jc-probe-20261008-213827-2732.log` confirms first private draw/postchecks returned and the later failure 21, then a chapter-cleanup stop 13 while preserving first=21. It does not contain the borrowed-resource subcode. `appearance_check` can return a definition/texture/storage failure or code 9 for the shared read budget. No specific texture change or timeout is claimed as the established cause.

`callback_read` previously called self-process `ReadProcessMemory` for every field, including repeated descriptor strings and full animation/material audits. The 0.58 callback instead uses `native_local_read.h` after a successful private-page runtime self-test. It queries committed/readable/non-guard regions into a fixed 16-entry cache scoped to one inspection budget, then copies fresh bytes under Windows SEH. Cache failure clears entries; renewed inspection epochs discard prior permission information. The cache is not data caching: repeated reads still observe changes, and every higher-level validation remains. Fault filtering is limited to read access, in-page or guard faults whose address lies in the requested source range; unrelated faults propagate. A concurrent transition to guarded memory after a positive permission query can consume the guard on failure; this does not establish atomic protection or ownership. Normal budgeted checks continue under the existing owner-thread/lifetime discipline.

The private-page test checks live rereads, a cross-page valid read, stale permission metadata after no-access protection, rejected boundary overlap, and rejecting an initially guarded page without consuming its guard. It runs in Scarfhelper before injection and again before the DLL's hooks. A returned failure falls back to ReadProcessMemory. Host fixtures exercise the same cache/range logic with synthetic mappings (1,000 changed-value reads require one permission query); Windows protected-copy code cross-compiles with Clang's SEH extension. Wine execution/performance remain unverified in the workspace. No new native addresses, hook sites, networking formats, software dependencies or game-file changes were introduced.

Duplicate transforms now skip the expensive idle-pose/torso rebuild while preserving receiver identity, sequence and freshness handling. All per-draw ownership and resource checks still execute. `JCProbePerformance` separates CPU elapsed time from GPU claims and records borrowed-resource code, stage, parameter and material role; failures are also logged without addresses or invite material. Source publication remains capped at two Hz. The next meaningful runtime observation is movement between deliberately distinct source locations and reduced stalls; if the resource guard still rejects, its exact failing condition will be available.

0.58 verification/install: six host sanitizer and Windows x64 fixture builds, helper/DLL build and 14 Python/package checks passed. The protected reader performs volatile source-byte loads to prevent compiler reuse of inspected values. The actual Windows self-test and performance improvement are runtime-pending. Both installed folders match the 0.58 artifact; 18 diagnostic/16 original-file hashes passed, and no applications were launched. Both 0.57 installation receipts and artifact preserve rollback.
