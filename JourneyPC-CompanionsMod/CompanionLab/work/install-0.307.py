import datetime
import hashlib
import json
from pathlib import Path
import re
import subprocess
import zipfile
import sys

WORK = Path(__file__).resolve().parent
TARGETS = {'devishaz': ('7D2AA64D-E176-4C5B-86A5-1EB2EF88AAD4', '0.306'), 'rebishaz': ('9FAD618B-4D76-4C96-B0AA-4A3A85EC24A0', '0.306'), 'smooviepuffs': ('598C5EF7-1BBF-4C6F-815E-6068C5507D85', '0.189')}
REQUESTED = sys.argv[1] if len(sys.argv)==2 else "devishaz"
if REQUESTED not in (*TARGETS,"all","current"):
    raise RuntimeError("Unknown test bottle")
NAME = REQUESTED
UUID, PRIOR_VERSION = TARGETS["devishaz"]
ARCHIVE_HASH = 'ea3d255b33a4b91f6b8c96f38265cfd5d80cb47333d3687c9819b054ce34f04c'
FILES = {'JNETCharacter_P_MechBodyCombined.D3D11x64', 'JNETFlipL.D3D11x64', 'JNETCharacter_P_FlowCreature_Fins.D3D11x64', 'JNETCharacter_P_FlowHunter_Head.D3D11x64', 'JNETCharacter_P_MechHeadEye.D3D11x64', 'JNETCharacter_P_ClothGuardianWing.D3D11x64', 'JNETCharacter_P_MechHeadGlowMask.D3D11x64', 'JNETCharacter_P_MechTail.D3D11x64', 'JNETCharacter_P_FlowCreature_Head.D3D11x64', 'JNETMenu.xme', 'JNETCharacter_P_FishFlat_A_Test.D3D11x64', 'JNETCharacter_P_MechHeadGrate.D3D11x64', 'JNET_WingSpin.D3D11x64', 'JNET_TailSpin.D3D11x64', 'JNETCharacter_P_FlowHunter_Fins.D3D11x64', 'JNETSpin3.D3D11x64', 'JNET_HeadBankL.D3D11x64', 'JNETCharacter_P_FlowHunter_FinsLarge.D3D11x64', 'JNET_FlatFishFlip.D3D11x64', 'JNET_NeckBankL.D3D11x64', 'JNETCharacter_P_Guardian.D3D11x64', 'manifest.json', 'JNETCharacter_P_FlowCreature_Body.D3D11x64', 'JNET_WingBankL.D3D11x64', 'JNETCharacter_P_GuardianWingR.D3D11x64', 'JNETCharacter_P_MechWing.D3D11x64', 'JNETCharacter_P_MechHeadFlaps.D3D11x64', 'JNETFlipR.D3D11x64', 'JNETCharacter_P_FlowHunter_Body.D3D11x64', 'JNET_HeadSpin.D3D11x64', 'JNET_TailBankL.D3D11x64', 'JNETCharacter_P_ClothGuardianTail.D3D11x64', 'README.txt', 'JNETCharacter_P_FlowHunter_Tail.D3D11x64', 'JNET.dll', 'JNET.exe', 'JNETCharacter_P_ClothGuardianHead.D3D11x64', 'JNET_NeckSpin.D3D11x64', 'JNETCharacter_P_MechHead.D3D11x64', 'JNET_FlatFishSwim.D3D11x64', 'JNETCharacter_P_ClothGuardianNeck.D3D11x64', 'JNETCharacter_P_GuardianWingL.D3D11x64', 'JNETCharacter_P_FlowCreature_Tail.D3D11x64'}
PRIOR_INVENTORIES = {'devishaz': ['JNET.dll', 'JNET.exe', 'manifest.json', 'session-peers.bin', 'JNETCharacter_NeckSpin.Phyre.D3D11x64.gz', 'JNETCharacter_HeadBankL.Phyre.D3D11x64.gz', 'JNETCharacter_WingSpin.Phyre.D3D11x64.gz', 'JNETCharacter_WingBankL.Phyre.D3D11x64.gz', 'JNETCharacter_FlatFishSwim.Phyre.D3D11x64.gz', 'JNETCharacter_NeckBankL.Phyre.D3D11x64.gz', 'JNETCharacter_HeadSpin.Phyre.D3D11x64.gz', 'JNETCharacter_FlatFishFlip.Phyre.D3D11x64.gz', 'JNETCharacter_TailBankL.Phyre.D3D11x64.gz', 'JNETCharacter_TailSpin.Phyre.D3D11x64.gz', 'JNETCharacter_TailBankL.D3D11x64', 'JNETCharacter_FlatFishFlip.D3D11x64', 'JNETCharacter_HeadBankL.D3D11x64', 'JNETCharacter_WingBankL.D3D11x64', 'JNETCharacter_HeadSpin.D3D11x64', 'JNETCharacter_TailSpin.D3D11x64', 'JNETCharacter_NeckBankL.D3D11x64', 'JNETCharacter_FlatFishSwim.D3D11x64', 'JNETCharacter_NeckSpin.D3D11x64', 'JNETCharacter_WingSpin.D3D11x64', 'JNETCharacter_P_ClothGuardianNeck.D3D11x64', 'JNETCharacter_P_ClothGuardianTail.D3D11x64', 'JNETCharacter_P_MechWing.D3D11x64', 'JNETCharacter_P_MechHead.D3D11x64', 'JNETCharacter_P_ClothGuardianHead.D3D11x64', 'JNETCharacter_P_ClothGuardianWing.D3D11x64', 'JNETCharacter_P_MechBodyCombined.D3D11x64', 'JNETCharacter_P_MechHeadGlowMask.D3D11x64', 'JNETCharacter_P_MechHeadEye.D3D11x64', 'JNETCharacter_P_FishFlat_A_Test.D3D11x64', 'JNETCharacter_P_MechTail.D3D11x64', 'JNETCharacter_P_MechHeadGrate.D3D11x64', 'JNETCharacter_P_GuardianWingL.D3D11x64', 'JNETCharacter_P_GuardianWingR.D3D11x64', 'JNETCharacter_P_MechHeadFlaps.D3D11x64', 'JNETCharacter_P_Guardian.D3D11x64', 'sound-state.bin', 'tone-code.bin', 'JNET-audio-manifest.json', 'JNET-audio-README.md', 'sound-runtime.bin', 'JNET-audio-install.py', 'Journey.exe', 'JNETCharacter_P_Card.D3D11x64', 'JNET_WingBankL.D3D11x64', 'JNET_FlatFishSwim.D3D11x64', 'JNET_NeckSpin.D3D11x64', 'JNET_HeadSpin.D3D11x64', 'JNET_FlatFishFlip.D3D11x64', 'JNET_TailSpin.D3D11x64', 'JNET_WingSpin.D3D11x64', 'JNET_NeckBankL.D3D11x64', 'JNET_TailBankL.D3D11x64', 'JNET_HeadBankL.D3D11x64', 'README.txt', 'JNETMenu.xme', 'JNETSpin3.D3D11x64', 'JNETCharacter_P_FlowHunter_Fins.D3D11x64', 'JNETCharacter_P_FlowCreature_Tail.D3D11x64', 'JNETCharacter_P_FlowHunter_Body.D3D11x64', 'JNETCharacter_P_FlowCreature_Body.D3D11x64', 'JNETCharacter_P_FlowHunter_FinsLarge.D3D11x64', 'JNETCharacter_P_FlowCreature_Head.D3D11x64', 'JNETCharacter_P_FlowCreature_Fins.D3D11x64', 'JNETCharacter_P_FlowHunter_Head.D3D11x64', 'JNETCharacter_P_FlowHunter_Tail.D3D11x64', 'JNETFlipR.D3D11x64', 'JNETFlipL.D3D11x64'], 'rebishaz': ['JNET.dll', 'JNET.exe', 'README.txt', 'JNETMenu.xme', 'JNETSpin3.D3D11x64', 'JNET_FlatFishFlip.D3D11x64', 'JNET_FlatFishSwim.D3D11x64', 'JNET_HeadBankL.D3D11x64', 'JNET_HeadSpin.D3D11x64', 'JNET_NeckBankL.D3D11x64', 'JNET_NeckSpin.D3D11x64', 'JNET_TailBankL.D3D11x64', 'JNET_TailSpin.D3D11x64', 'JNET_WingBankL.D3D11x64', 'JNET_WingSpin.D3D11x64', 'JNETCharacter_P_ClothGuardianHead.D3D11x64', 'JNETCharacter_P_ClothGuardianNeck.D3D11x64', 'JNETCharacter_P_ClothGuardianTail.D3D11x64', 'JNETCharacter_P_ClothGuardianWing.D3D11x64', 'JNETCharacter_P_FishFlat_A_Test.D3D11x64', 'JNETCharacter_P_Guardian.D3D11x64', 'JNETCharacter_P_GuardianWingL.D3D11x64', 'JNETCharacter_P_GuardianWingR.D3D11x64', 'JNETCharacter_P_MechBodyCombined.D3D11x64', 'JNETCharacter_P_MechHead.D3D11x64', 'JNETCharacter_P_MechHeadEye.D3D11x64', 'JNETCharacter_P_MechHeadFlaps.D3D11x64', 'JNETCharacter_P_MechHeadGlowMask.D3D11x64', 'JNETCharacter_P_MechHeadGrate.D3D11x64', 'JNETCharacter_P_MechTail.D3D11x64', 'JNETCharacter_P_MechWing.D3D11x64', 'manifest.json', 'JNETCharacter_P_FlowHunter_Fins.D3D11x64', 'JNETCharacter_P_FlowCreature_Body.D3D11x64', 'JNETCharacter_P_FlowHunter_FinsLarge.D3D11x64', 'JNETCharacter_P_FlowHunter_Body.D3D11x64', 'JNETCharacter_P_FlowCreature_Fins.D3D11x64', 'JNETFlipR.D3D11x64', 'JNETFlipL.D3D11x64', 'JNETCharacter_P_FlowHunter_Tail.D3D11x64', 'JNETCharacter_P_FlowCreature_Tail.D3D11x64', 'JNETCharacter_P_FlowCreature_Head.D3D11x64', 'JNETCharacter_P_FlowHunter_Head.D3D11x64'], 'smooviepuffs': ['JNET.dll', 'QUICKSTART.txt', 'LICENSE-MINHOOK.txt', 'LICENSE-JOURNEY-DETOUR.txt', 'JNET.exe', 'LICENSE-ZIG.txt', 'manifest.json', 'LICENSE-MINGW.txt', 'THIRD-PARTY.txt', 'session-peers.bin']}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    if path.is_symlink() or path.resolve() != path or not path.is_file():
        raise RuntimeError("Missing or redirected file: " + str(path))
    return path.read_bytes()


def closed():
    output = subprocess.run(["ps", "-axo", "pid=,args="], check=True, capture_output=True, text=True).stdout
    for line in output.splitlines():
        if re.search(r"(?i)(?:^|[/\\\s])(?:Journey|JNET)\.exe(?:\s|$)", line):
            raise RuntimeError("Close Journey and JNET before installation; PID " + line.strip().split()[0])


def main():
    PRIOR_FILES = set(PRIOR_INVENTORIES[NAME]) - {"session-peers.bin"}
    archive = WORK / "JNET-0.307.zip"
    if sha(read(archive)) != ARCHIVE_HASH:
        raise RuntimeError("Package hash mismatch")
    with zipfile.ZipFile(archive) as package:
        if len(package.namelist()) != len(FILES) or set(package.namelist()) != {"JNET/" + n for n in FILES}:
            raise RuntimeError("Package allowlist mismatch")
        payload = {n: package.read("JNET/" + n) for n in FILES}
    manifest = json.loads(payload["manifest.json"])
    if manifest["version"] != "0.307" or set(manifest["artifacts"]) != FILES - {"manifest.json"}:
        raise RuntimeError("Manifest mismatch")
    for name, digest in manifest["artifacts"].items():
        if sha(payload[name]) != digest:
            raise RuntimeError("Package artifact mismatch: " + name)
    bottle = next(b for b in json.loads(read(WORK / "bottles.json"))["bottles"] if b["name"] == NAME)
    game = Path("/Users/rebisan/Library/Containers/com.isaacmarovitz.Whisky/Bottles") / UUID / "drive_c/Program Files (x86)/Steam/steamapps/common/Journey"
    if bottle["bottle_uuid"] != UUID or bottle["game_directory"] != str(game):
        raise RuntimeError("Bottle identity mismatch")
    folder = game / ("JNET" if NAME == "rebishaz" else "Journet")
    receipt_path = WORK / ("install-" + NAME + ".json")
    receipt_bytes = read(receipt_path)
    receipt = json.loads(receipt_bytes)
    if receipt["version"] != PRIOR_VERSION or receipt["directory"] != str(folder) or set(receipt["files"]) != set(PRIOR_INVENTORIES[NAME]):
        raise RuntimeError("Prior installation receipt mismatch")
    closed()
    for name in FILES - PRIOR_FILES:
        if (folder / name).exists() or (folder / name).is_symlink():
            raise RuntimeError("Unexpected new artifact already exists: " + name)
    saved = {name: read(folder / name) for name in receipt["files"]}
    if any(sha(value) != receipt["files"][name] for name, value in saved.items()):
        raise RuntimeError("Installed artifact changed")

    def originals():
        for name, info in bottle["files"].items():
            if Path(name).is_absolute() or ".." in Path(name).parts:
                raise RuntimeError("Invalid original inventory")
            if sha(read(game / name)) != info["sha256"]:
                raise RuntimeError("Original-file mismatch: " + name)

    originals()
    launch_path=folder / "JNET-launch.ini"
    launch_bytes=read(launch_path) if launch_path.exists() else None
    config_path=folder / "JNET-network.ini"
    config_bytes=None
    if config_path.exists() or config_path.is_symlink():
        import configparser
        config_bytes=read(config_path)
        config=configparser.ConfigParser();config.read_string(config_bytes.decode("utf-8-sig"))
        expected={("Diagnostics","Scans"):0,("Diagnostics","ObservationLogs"):0,("Transport","Worker"):0,("Motion","SmoothMs"):0}
        if any(config.getint(section,key,fallback=value)!=value for (section,key),value in expected.items()):
            raise RuntimeError("Existing network settings differ from baseline; preserved without installation")
    backup=WORK / ("rollback-0.307-"+NAME)
    backup.mkdir(exist_ok=False)
    for name,data in saved.items():
        (backup / name).write_bytes(data)
        if sha(read(backup / name))!=sha(data):raise RuntimeError("Rollback hash mismatch")
    (backup / "install-receipt.json").write_bytes(receipt_bytes)
    if config_bytes is not None:(backup / "JNET-network.ini").write_bytes(config_bytes)
    closed()
    retired = {n for n in saved if n.startswith("JC_") and n.endswith(".D3D11x64") and ("JNET_" + n[3:]) in payload}
    retired |= set(saved) & {"QUICKSTART.txt", "THIRD-PARTY.txt", "LICENSE-MINHOOK.txt", "LICENSE-JOURNEY-DETOUR.txt", "LICENSE-ZIG.txt", "LICENSE-MINGW.txt", "LICENSES.txt"}
    changed = []
    try:
        for name, data in payload.items():
            changed.append(name)
            (folder / name).write_bytes(data)
            if sha(read(folder / name)) != sha(data):
                raise RuntimeError("Copied hash mismatch: " + name)
        originals()
        if launch_bytes is not None and read(launch_path)!=launch_bytes:
            raise RuntimeError("Executable selection changed")
        if "session-peers.bin" in saved and read(folder / "session-peers.bin") != saved["session-peers.bin"]:
            raise RuntimeError("Session configuration changed")
        if config_bytes is not None and read(config_path)!=config_bytes:
            raise RuntimeError("Network configuration changed")
        for name in retired:
            (folder / name).unlink()
            if (folder / name).exists():raise RuntimeError("Retired file remains: " + name)
        receipt.update(version="0.307", installed_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                       package_sha256=ARCHIVE_HASH,
                       files={**{n: h for n,h in receipt["files"].items() if n not in retired}, **{n: sha(v) for n, v in payload.items()}},
                       original_checks_passed=len(bottle["files"]), game_launched=False, helper_launched=False,
                       runtime_verified=False, process_check="closed before replacement",
                       validation="0.307 initializes and caches each War Machine coating separately; no live shader rebinding. fl0w uses Rogue/Hunter skins with shared controls and both chirp families. Offline regression and package verified; gameplay unverified.", rollback_directory=str(backup), rollback=str(backup))
        receipt_path.write_text(json.dumps(receipt, indent=2) + "\n")
    except Exception:
        for name in reversed(changed):
            (folder / name).write_bytes(saved[name]) if name in saved else (folder / name).unlink()
        for name in retired:
            (folder / name).write_bytes(saved[name])
        receipt_path.write_bytes(receipt_bytes)
        raise
    print(json.dumps(dict(installed="0.307", bottle=NAME, artifacts_verified=len(payload), retired_files=len(retired),
                          original_files_verified=len(bottle["files"]), configuration_unchanged=True, baseline_settings_verified=True, rollback=str(backup),
                          game_launched=False, runtime_verified=False), indent=2))


if __name__ == "__main__":
    for NAME in (tuple(TARGETS) if REQUESTED == "all" else ("devishaz","rebishaz") if REQUESTED == "current" else (REQUESTED,)):
        UUID, PRIOR_VERSION = TARGETS[NAME]
        main()
