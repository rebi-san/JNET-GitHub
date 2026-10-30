import unittest
import json
import tempfile
from pathlib import Path

from analyze_connection_scan import clock_valid, summarize, LifecycleSummary, load
from inspect_build import EXPECTED_EXE


def sample(ms, connected=0, gate=0, clock=0):
    dude = dict(code=0, symbol_zero_based=2, dissolve=[0, 0], emitters=[0, 0], clips=[
        dict(name="Glide", frame=clock, clock=clock, duration=-1, range=[.04166667, 1], state_code=0)])
    return dict(code=0, ms=ms, frame=ms, frame_after=ms, read_ms=5, connected=connected,
                gate=gate, gates=[gate, 0, 0], dudes=[dict(code=1), dude],
                cloth=[2, 4, 0], shout=[0, 7, 1, 0, 1])


class ConnectionScanTests(unittest.TestCase):
    def header(self):
        return dict(format="jnet-native-connection-1", game_sha256=EXPECTED_EXE, version="0.73")

    def test_current_and_historical_capture_names(self):
        records = [self.header(), sample(0), sample(250, 1, 1), dict(end=True, error=0)]
        current = summarize(records)
        records[0]["format"] = "jc-native-connection-1"
        self.assertEqual(summarize(records), current)
        records[0]["format"] = "unsupported"
        with self.assertRaises(ValueError):
            summarize(records)

    def test_native_clock_below_asset_start(self):
        for clock in (0, .004938841):
            self.assertTrue(clock_valid(clock, -1, [.04166667, 1]))
        self.assertFalse(clock_valid(1, -1, [.04166667, 1]))
        self.assertFalse(clock_valid(0, -1, [1, 1]))
        self.assertFalse(clock_valid(float("nan"), -1, [0, 1]))
        self.assertTrue(clock_valid(4, 4, [0, 1]))

    def test_connection_then_visibility_then_departure(self):
        records = [self.header(), sample(0), sample(250, 1), sample(500, 1, 1), sample(750)]
        records[-1]["dudes"][1].update(dissolve=[.3, .2], emitters=[1, 1])
        records[-1]["dudes"][1]["clips"][0]["name"] = "Med"
        records.append(dict(end=True, error=0))
        result = summarize(records)
        self.assertEqual(len(result["transitions"]), 4)
        remote = result["travelers"][1]
        self.assertEqual(len(remote["old_range_rejections"]), 4)
        self.assertEqual(remote["corrected_clock_rejections"], [])
        self.assertEqual(remote["first_emitter_ms"], 750)
        self.assertEqual(remote["first_target_dissolve_ms"], 750)
        self.assertEqual(remote["sitting_samples"], [dict(ms=750, name="Med")])
        self.assertEqual(result["remote_shout"]["samples"], 4)
        self.assertEqual(result["local_shout"]["samples"], 0)

    def test_declined_and_incomplete_capture(self):
        result = summarize([self.header(), dict(code=9, ms=200, read_ms=75)])
        self.assertEqual(result["base_codes"], {9: 1})
        self.assertFalse(result["ended"])
        self.assertEqual(result["transitions"], [])
        self.assertEqual(result["travelers"][1]["old_range_rejections"], [])

    def test_wrong_fingerprint_and_order_rejected(self):
        with self.assertRaises(ValueError):
            summarize([dict(self.header(), game_sha256="wrong")])
        with self.assertRaises(ValueError):
            summarize([self.header(), sample(500), sample(250)])

    def test_lifecycle_native_frame_and_gap(self):
        trace = LifecycleSummary()
        trace.consume(dict(lifecycle=1))
        def entry(seq, event, role=0, frame=10):
            return dict(lc=[seq, frame, seq, 7, event, 0x118713, role, 0, "10", "20", "30", "40"])
        for row in (entry(1,21), entry(2,6,3), entry(3,8,1), entry(4,8,2), entry(5,22)):
            trace.consume(row)
        result=trace.result()
        self.assertEqual(result["complete_frame_spans"],1)
        self.assertEqual(result["common_frame_paths"][0]["entries"],
                         ["RemoteApply:player_barn", "DudeUpdate:local", "DudeUpdate:remote"])
        trace.consume(entry(6,21,frame=11))
        trace.consume(entry(8,22,frame=11))
        self.assertEqual(trace.result()["sequence_gaps"],1)
        self.assertEqual(trace.result()["incomplete_frame_spans"],1)
        self.assertEqual(trace.result()["complete_frame_spans"],1)
        with self.assertRaises(ValueError):
            trace.consume(entry(8,22))

    def test_lifecycle_roles_and_missing_header_rejected(self):
        trace=LifecycleSummary()
        row=dict(lc=[1,1,1,1,8,0,15,0,"0","0","0","0"])
        with self.assertRaises(ValueError):
            trace.consume(row)

        trace.consume(dict(lifecycle=1))
        with self.assertRaises(ValueError):
            trace.consume(row)
        row["lc"][6]=2
        row["lc"][8]="not a pointer"
        with self.assertRaises(ValueError):
            trace.consume(row)

    def test_animation_job_owner_comes_from_submit_mesh(self):
        trace=LifecycleSummary()
        trace.consume(dict(lifecycle=1))
        def event(seq,kind,thread=7,roles=0):
            return dict(lc=[seq,1,seq,thread,kind,0,roles,0,"1234","200210","0","0"])
        trace.consume(event(1,21))
        trace.consume(event(2,17,roles=6<<4))
        trace.consume(event(3,18,thread=8))
        trace.consume(event(4,18))
        trace.consume(event(5,22))
        result=trace.result()
        self.assertEqual(result["animation_jobs"]["matched_submit_finish_entries"],{"remote_mesh":1})
        self.assertEqual(result["animation_jobs"]["unmatched_finish_entries"],1)
        self.assertEqual(result["common_frame_paths"][0]["entries"],
                         ["AnimationSubmit:remote_mesh","AnimationFinish:remote_mesh"])
        trace.consume(event(6,17,roles=5<<4))
        trace.consume(event(8,18))
        self.assertEqual(trace.result()["animation_jobs"]["unfinished_or_abandoned"],1)

    def test_streamed_lifecycle_preserves_state_analysis(self):
        rows=[self.header(), dict(lifecycle=1), sample(0),
              dict(lc=[1,1,100,7,21,0,4,0,"10","0","0","0"]),
              dict(lc=[2,1,101,7,8,0x118713,2,0,"20","0","0","0"]),
              dict(lc=[3,1,102,7,22,0,4,0,"10","0","0","0"]),
              sample(250,1,1), dict(lifecycle_end=True, overwritten=0, contention_drops=0),
              dict(end=True,error=0)]
        with tempfile.TemporaryDirectory(dir=Path(__file__).parent / "work") as directory:
            path=Path(directory)/"capture.jsonl"
            path.write_text("".join(json.dumps(r)+"\n" for r in rows))
            result=load(path)
        self.assertEqual(result["samples"],2)
        self.assertEqual(result["lifecycle"]["events"],3)
        self.assertEqual(result["lifecycle"]["observed_duration_ms"],2)
        self.assertTrue(result["ended"])

    def test_native_path_coverage_and_known_092_blind_spot(self):
        trace=LifecycleSummary()
        trace.consume(dict(self.header(),version="0.92"))
        trace.consume(dict(lifecycle=1))
        # Attach mid-frame, one complete remote update, then a partial final frame.
        path=[(22,4),(21,4),(6,3),(8,2),(9,2),(19,6),(13,2),(12,2),
              (19,6),(11,2),(22,4),(21,4)]
        for seq,(event,role) in enumerate(path,1):
            trace.consume(dict(lc=[seq,10 if seq<12 else 11,seq,7,event,0,role,0,"0","0","0","0"]))
        r=trace.result()
        self.assertEqual(r["unmatched_frame_ends"],1)
        self.assertEqual(r["incomplete_frame_spans"],1)
        self.assertEqual(r["frame_coverage"]["remote_apply_matched_path_frames"],1)
        self.assertEqual(r["path_count_mismatches"],{})
        self.assertIn("RCX",r["capture_limitations"][0])


if __name__ == "__main__":
    unittest.main()
