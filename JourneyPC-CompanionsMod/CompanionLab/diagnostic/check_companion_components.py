"""Build isolated component fixtures; never load a DLL, open Journey or package an update."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
WORK = HERE.parent / "work"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--skip-socket-tests", action="store_true",
                        help="Skip socket I/O only; still run both authenticated packet/policy fixtures and record a separate receipt")
    args = parser.parse_args()
    zig = WORK / "deps/zig-macos-aarch64-0.13.0/zig"
    if not zig.is_file():
        raise SystemExit("Approved workspace Zig toolchain missing; no downloads performed")
    output = WORK / "companion-components"
    output.mkdir(exist_ok=True)
    env = dict(os.environ)
    for key, directory in (("ZIG_GLOBAL_CACHE_DIR", "zig-cache"),
                           ("ZIG_LOCAL_CACHE_DIR", "zig-local-cache"), ("TMPDIR", "tmp")):
        path = WORK / directory
        path.mkdir(exist_ok=True)
        env[key] = str(path)
    env["UBSAN_OPTIONS"] = "halt_on_error=1"
    result = {"game_runtime_tested": False, "transport_implemented": "IPv4 localhost pose prototype; Wine runtime unverified",
              "installed": False, "checks": {}, "sources": {}, "skipped": []}
    for name in ("native_peer_lifecycle", "native_cloth_producer", "native_cloth_prelaunch", "native_chapter_gate", "native_render_gpu", "native_cloth_material_live", "native_render_retirement", "native_cloth_frame", "native_cloth_partition", "native_receiver", "traveler_trial_runtime", "native_traveler_trial", "native_lifecycle_trace", "companion_session", "native_appearance", "native_geometry_pool", "native_initialized_lifetime", "native_dual_body", "native_mesh_guard", "native_idle_pose", "native_animation_playback", "native_connection_scan", "native_skeleton_mirror", "native_evaluation_layout", "native_evaluation_run", "native_companion_chirp", "native_companion_cloth", "native_read_batch", "native_shader_inputs", "companion_pose", "companion_room", "native_render_queue", "diagnostic_checkpoint", "diagnostic_fault", "native_local_read"):
        if args.skip_socket_tests and name in ("companion_pose", "companion_room"):
            result["skipped"].append(name + ": socket I/O only")
        source = HERE / ("test_" + name + ".c")
        host = output / ("test_" + name)
        windows = output / ("test_" + name + ".exe")
        flags = ["-std=c11", "-fms-extensions", "-Wall", "-Wextra", "-Werror"]
        if args.skip_socket_tests and name in ("companion_pose", "companion_room"):
            flags.append("-DJC_NO_SOCKET_TESTS")
        subprocess.run(["clang", *flags, "-fsanitize=address,undefined", str(source),
                        "-o", str(host)], check=True, env=env)
        completed = subprocess.run([str(host)], capture_output=True, text=True, env=env)
        if completed.returncode:
            raise SystemExit(completed.stdout + completed.stderr)
        print(completed.stdout.strip(), flush=True)
        subprocess.run([str(zig), "cc", "-target", "x86_64-windows-gnu", *flags,
                        str(source), *(["-lbcrypt", "-lws2_32"] if name in ("companion_pose", "companion_room") else []), "-o", str(windows)], check=True, env=env)
        result["checks"][name] = {"host_asan_ubsan": "passed", "windows_x64_build": "passed",
                                  "windows_runtime": "not run", "output": completed.stdout.strip()}
    for name in ("native_peer_lifecycle.h", "test_native_peer_lifecycle.c", "native_actor_frame.h", "native_actor_material.h", "traveler_actor_runtime.h", "traveler_live_backend.h", "traveler_glyph_init.h", "native_cloth_producer.h", "test_native_cloth_producer.c", "native_actor_runtime.h", "native_cloth_prelaunch.h", "test_native_cloth_prelaunch.c", "native_actor_live.h", "native_cloth_publication.h", "native_cloth_render_runtime.h", "native_chapter_gate.h", "test_native_chapter_gate.c", "native_cloth_commands.h", "native_cloth_live.h", "test_native_render_gpu.c", "native_cloth_material_live.h", "test_native_cloth_material_live.c", "native_render_retirement.h", "test_native_render_retirement.c", "native_cloth_frame.h", "test_native_cloth_frame.c", "native_cloth_draw.h", "native_cloth_partition.h", "test_native_cloth_partition.c", "native_receiver.h", "test_native_receiver.c", "native_traveler_trial.h", "traveler_trial_runtime.h", "test_native_traveler_trial.c", "test_traveler_trial_runtime.c", "native_lifecycle_trace.h", "native_lifecycle_hooks.h", "test_native_lifecycle_trace.c", "native_companion_cloth.h", "test_native_companion_cloth.c", "native_companion_chirp.h", "test_native_companion_chirp.c", "native_evaluation_run.h", "test_native_evaluation_run.c", "native_evaluation_layout.h", "test_native_evaluation_layout.c", "native_skeleton_mirror.h", "test_native_skeleton_mirror.c", "native_connection_scan.h", "test_native_connection_scan.c", "connection_recorder.h", "companion_session.h", "test_companion_session.c", "native_appearance.h",
                 "test_native_appearance.c", "native_environment_probe.h", "native_material_ready.h", "native_layout.h", "native_geometry_pool.h", "test_native_geometry_pool.c",
                 "native_initialized_lifetime.h", "test_native_initialized_lifetime.c", "native_dual_body.h", "test_native_dual_body.c", "native_mesh_guard.h", "test_native_mesh_guard.c", "native_idle_pose.h", "test_native_idle_pose.c", "native_shader_inputs.h", "test_native_shader_inputs.c", "native_trial_shader.h", "native_shader_inputs.h",
                 "native_read_batch.h", "test_native_read_batch.c", "companion_animation.h", "native_animation_playback.h", "test_native_animation_playback.c", "companion_room.h", "test_companion_room.c", "companion_pose.h", "pose_link.h", "test_companion_pose.c", "native_render_queue.h", "test_native_render_queue.c", "diagnostic_checkpoint.h", "test_diagnostic_checkpoint.c", "diagnostic_fault.h", "test_diagnostic_fault.c", "native_local_read.h", "test_native_local_read.c", "check_companion_components.py"):
        result["sources"][name] = hashlib.sha256((HERE / name).read_bytes()).hexdigest()
    receipt = output / ("verification-no-sockets.json" if args.skip_socket_tests else "verification.json")
    receipt.write_text(json.dumps(result, indent=2) + "\n")
    print("Verification receipt: " + str(receipt))


if __name__ == "__main__":
    main()
