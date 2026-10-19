"""Summarize a supplied native-connection capture without accessing a game process."""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path

from inspect_build import EXPECTED_EXE


LIFECYCLE_EVENTS = {
    1: ("DudeConstructor", 0xa1e80), 2: ("DudeResourceInit", 0xa4a60),
    3: ("DudeCleanup", 0xa3fe0), 4: ("DudeDestructor", 0xa24b0),
    5: ("RemoteReceive", 0x977a0), 6: ("RemoteApply", 0x118730),
    7: ("PlayerUpdate", 0x118690), 8: ("DudeUpdate", 0xa6350),
    9: ("DudeSimulation", 0x24ab80), 10: ("PlayerAppearance", 0x2177c0),
    11: ("ClothAppearance", 0x24c010), 12: ("ChestSymbol", 0xa8d70),
    13: ("ChirpSymbol", 0xa8930), 14: ("PlayerShout", 0x24e290),
    15: ("ShoutUpdate", 0x13e6b0), 16: ("RemoteDeparture", 0x24fa70),
    17: ("AnimationSubmit", 0x262680), 18: ("AnimationFinish", 0x2628d0),
    19: ("MeshUpdate", 0x263aa0), 20: ("DudeClothInit", 0x246230),
    21: ("FrameBegin", None), 22: ("FrameEnd", None),
}
LIFECYCLE_ROLES = ("other", "local", "remote", "player_barn", "game",
                   "local_mesh", "remote_mesh", "local_state", "remote_state")


class LifecycleSummary:
    """Bounded aggregation; never retain the full per-frame trace in memory."""
    def __init__(self):
        self.total = self.last = self.gaps = self.incomplete = self.complete = 0
        self.counts, self.callers, self.patterns = Counter(), Counter(), Counter()
        self.pending = {}
        self.enabled = False
        self.loss = None
        self.first_tick = self.last_tick = None
        self.version = None
        self.frame_coverage = Counter()
        self.missing_steps = Counter()
        self.boundaries = []
        self.orphan_frame_ends = 0
        self.animation_jobs = {}
        self.animation_pairs = Counter()
        self.animation_unmatched = 0
        self.animation_abandoned = 0

    def consume(self, record):
        if "format" in record:
            self.version = record.get("version")
        if "lifecycle" in record:
            if record["lifecycle"] != 1 or self.enabled:
                raise ValueError("Invalid lifecycle header")
            self.enabled = True
        if "lifecycle_end" in record:
            self.loss = record
        if "lc" not in record:
            return
        e = record["lc"]
        if (not self.enabled or not isinstance(e, list) or len(e) != 12
                or any(type(v) is not int or not 0 <= v <= 0xffffffff for v in e[:8])
                or e[4] not in LIFECYCLE_EVENTS or e[0] <= self.last):
            raise ValueError("Invalid lifecycle entry or sequence")
        for value in e[8:]:
            if not isinstance(value, str) or not 1 <= len(value) <= 16 or any(c not in "0123456789abcdef" for c in value):
                raise ValueError("Invalid lifecycle register value")
        roles = tuple((e[6] >> (4*i)) & 15 for i in range(4))
        if e[6] > 0xffff or any(v >= len(LIFECYCLE_ROLES) for v in roles):
            raise ValueError("Invalid lifecycle role")
        if self.last and e[0] != self.last+1:
            self.gaps += e[0]-self.last-1
            self.incomplete += len(self.pending)
            self.pending.clear()
            self.animation_abandoned += len(self.animation_jobs)
            self.animation_jobs.clear()
        self.last = e[0]
        if self.first_tick is None:
            self.first_tick = e[2]
        self.last_tick = e[2]
        self.total += 1
        event, thread, frame = e[4], e[3], e[1]
        path_role = roles[0]
        job_key = (thread, int(e[8], 16))
        if event == 17 and roles[1] in (5, 6):
            path_role = roles[1]
            if job_key in self.animation_jobs:
                self.animation_abandoned += 1
            elif len(self.animation_jobs) >= 128:
                raise ValueError("Excessive unfinished animation jobs")
            self.animation_jobs[job_key] = roles[1]
        elif event == 18:
            owner = self.animation_jobs.pop(job_key, None)
            if owner is None:
                self.animation_unmatched += 1
            else:
                path_role = owner
                self.animation_pairs[LIFECYCLE_ROLES[owner]] += 1
        self.counts[event, roles[0], roles[1]] += 1
        if event in (1, 2, 3, 4, 20) and len(self.boundaries) < 256:
            self.boundaries.append(dict(event=LIFECYCLE_EVENTS[event][0], frame=frame,
                trace_ms=(e[2]-self.first_tick) & 0xffffffff, role=LIFECYCLE_ROLES[roles[0]],
                caller_rva=hex(e[5]), object_token=e[8]))
        key = (event, e[5])
        if key not in self.callers and len(self.callers) >= 4096:
            raise ValueError("Excessive lifecycle call sites")
        self.callers[key] += 1
        if event == 21:
            self.incomplete += thread in self.pending
            if thread not in self.pending and len(self.pending) >= 128:
                raise ValueError("Excessive lifecycle threads")
            self.pending[thread] = (frame, [])
        elif thread in self.pending:
            expected, path = self.pending[thread]
            if expected != frame or len(path) >= 256:
                self.incomplete += 1
                del self.pending[thread]
            elif event == 22:
                pattern = tuple(path)
                if pattern in self.patterns or len(self.patterns) < 256:
                    self.patterns[pattern] += 1
                self.complete += 1
                counts = Counter(path)
                self.frame_coverage["remote_apply_frames"] += counts[6, 3] > 0
                for role, name, mesh in ((1, "local", 5), (2, "remote", 6)):
                    if counts[8, role] != 1:
                        if counts[8, role] > 1:
                            self.frame_coverage[name+"_repeated_update_frames"] += 1
                        continue
                    self.frame_coverage[name+"_update_frames"] += 1
                    missing = [label for event, owner, expected, label in (
                        (9, role, 1, "simulation"), (11, role, 1, "cloth_appearance"),
                        (12, role, 1, "chest_symbol"), (13, role, 1, "chirp_symbol"),
                        (19, mesh, 2, "two_mesh_updates")) if counts[event, owner] != expected]
                    self.frame_coverage[name+"_matched_path_frames"] += not missing
                    if role == 2 and counts[6, 3]:
                        self.frame_coverage["remote_apply_matched_path_frames"] += not missing
                    for label in missing:
                        self.missing_steps[name+":"+label] += 1
                del self.pending[thread]
            else:
                path.append((event, path_role))
        elif event == 22:
            self.orphan_frame_ends += 1

    def result(self):
        if not self.enabled:
            return None
        return dict(events=self.total, sequence_gaps=self.gaps, recorder_loss=self.loss,
            animation_jobs=dict(matched_submit_finish_entries=dict(self.animation_pairs),
                unmatched_finish_entries=self.animation_unmatched,
                unfinished_or_abandoned=self.animation_abandoned+len(self.animation_jobs),
                meaning="Job/thread entry pairing only; finish entry is not completed evaluation."),
            observed_duration_ms=(self.last_tick-self.first_tick) & 0xffffffff if self.total else 0,
            complete_frame_spans=self.complete, incomplete_frame_spans=self.incomplete+len(self.pending),
            unmatched_frame_ends=self.orphan_frame_ends,
            frame_coverage=dict(self.frame_coverage), path_count_mismatches=dict(self.missing_steps),
            resource_boundaries_first_256=self.boundaries,
            capture_limitations=(["0.92 filters AnimationSubmit/Finish by RCX mesh identity, but RCX is the evaluation job. "
                                  "Those calls are not observable in this capture; zero is not evidence of absence."]
                                 if self.version == "0.92" else []),
            calls=[dict(name=LIFECYCLE_EVENTS[e][0], rva=hex(LIFECYCLE_EVENTS[e][1]) if LIFECYCLE_EVENTS[e][1] else None,
                        rcx_role=LIFECYCLE_ROLES[a], rdx_role=LIFECYCLE_ROLES[b], count=n)
                   for (e,a,b),n in sorted(self.counts.items())],
            callers=[dict(name=LIFECYCLE_EVENTS[e][0], caller_rva=hex(c), count=n)
                     for (e,c),n in sorted(self.callers.items())],
            common_frame_paths=[dict(count=n, entries=[LIFECYCLE_EVENTS[e][0]+":"+LIFECYCLE_ROLES[r] for e,r in path])
                                for path,n in self.patterns.most_common(8)],
            interpretation="Entry observations only, not completion order or native function duration. "
                           "Complete spans cover one thread between frame markers; contention drops can still omit calls. "
                           "Role identity is from the latest frame boundary; constructors before attachment can be absent. "
                           "Register words are process-local observations, not a callable ABI or room payload.")


def clock_valid(frame, duration, bounds):
    if not all(math.isfinite(v) for v in (frame, duration, *bounds)):
        return False
    length = bounds[1] - bounds[0]
    return (abs(frame) <= 1000000 and duration <= 1000000
            and (duration >= 0 or duration == -1) and 0 < length <= 1000000
            and -.01 <= frame <= max(length, duration) + .01)


def summarize(records):
    if not records or records[0].get("format") != "jc-native-connection-1":
        raise ValueError("Unsupported connection capture")
    if records[0].get("game_sha256") != EXPECTED_EXE:
        raise ValueError("Unrecognized executable fingerprint")
    samples = [r for r in records if "code" in r]
    if len(samples) > 4800 or any(b["ms"] < a["ms"] for a, b in zip(samples, samples[1:])):
        raise ValueError("Invalid sample count or ordering")
    good = [r for r in samples if r["code"] == 0]
    transitions, last = [], None
    for r in good:
        state = (r["connected"], r["gate"], tuple(r["gates"]))
        if state != last:
            transitions.append(dict(ms=r["ms"], connected=state[0], gate=state[1], gates=list(state[2])))
            last = state
    travelers = []
    for index in range(2):
        clips, bad_old, bad_new, sit, target, rendered, emitters = [], [], [], [], [], [], []
        symbols = Counter()
        for r in good:
            d = r["dudes"][index]
            if d["code"]:
                continue
            symbols[d["symbol_zero_based"]] += 1
            if d["dissolve"][0] > 0:
                target.append(r["ms"])
            if d["dissolve"][1] > 0:
                rendered.append(r["ms"])
            if any(d["emitters"]):
                emitters.append(r["ms"])
            for c in d.get("clips", []):
                if c["state_code"]:
                    continue
                item = dict(ms=r["ms"], name=c["name"], frame=c["frame"], duration=c["duration"],
                            clock=c["clock"], range=c["range"], same_update=r["frame"] == r["frame_after"])
                clips.append(item)
                if not c["range"][0] - .01 <= c["frame"] <= c["range"][1] + .01:
                    bad_old.append(item)
                if not clock_valid(c["frame"], c["duration"], c["range"]):
                    bad_new.append(item)
                if c["name"] in ("ReadyToMed", "Med"):
                    sit.append(dict(ms=r["ms"], name=c["name"]))
        travelers.append(dict(role="local" if index == 0 else "native_remote", symbols_zero_based=dict(symbols),
                               clips_checked=len(clips), old_range_rejections=bad_old, corrected_clock_rejections=bad_new,
                               sitting_samples=sit, first_target_dissolve_ms=target[0] if target else None,
                               first_render_dissolve_ms=rendered[0] if rendered else None,
                               emitter_samples=len(emitters), first_emitter_ms=emitters[0] if emitters else None))
    def shout_window(column):
        times = [r["ms"] for r in good if r["shout"][0] == 0 and r["shout"][column] > 0]
        return dict(samples=len(times), first_ms=times[0] if times else None, last_ms=times[-1] if times else None)
    return dict(version=records[0].get("version"), samples=len(samples),
                base_codes=dict(Counter(r["code"] for r in samples)), transitions=transitions, travelers=travelers,
                read_ms_mean=sum(r["read_ms"] for r in samples) / len(samples) if samples else None,
                read_ms_peak=max((r["read_ms"] for r in samples), default=None),
                cloth_codes=dict(Counter(r["cloth"][0] for r in good if r["cloth"][0] != -1)),
                local_shout=shout_window(3), remote_shout=shout_window(4),
                ended=bool(records[-1].get("end")), recorder_error=records[-1].get("error"),
                interpretation="Sampled observations; transition times bound first observation, not exact event time. "
                               "A disconnected embedded traveler may still animate. No account identity or visual validation inferred.")


def load(path):
    records, lifecycle, digest, size = [], LifecycleSummary(), hashlib.sha256(), 0
    def invalid_constant(value):
        raise ValueError("Nonfinite JSON constant: " + value)
    with path.open("rb") as source:
        while True:
            line = source.readline(32001)
            if not line:
                break
            size += len(line)
            if len(line) > 32000 or size > 256 * 1024 * 1024:
                raise ValueError("Capture exceeds bounded record/file size")
            digest.update(line)
            record = json.loads(line, parse_constant=invalid_constant)
            if not isinstance(record, dict):
                raise ValueError("Invalid capture record")
            lifecycle.consume(record)
            if "lc" not in record and "lifecycle" not in record and "lifecycle_end" not in record:
                records.append(record)
                if len(records) > 4802:
                    raise ValueError("Excessive state snapshots")
    result = summarize(records)
    if lifecycle.enabled:
        result["lifecycle"] = lifecycle.result()
    result.update(file=path.name, sha256=digest.hexdigest())
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path)
    args = parser.parse_args()
    print(json.dumps(load(args.capture), indent=2, allow_nan=False))
