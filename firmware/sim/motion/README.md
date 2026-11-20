# Motion recordings

Real accelerometer logs from the assembled cube, captured with
`POST /api/motion-log {"seconds": 30}` and `GET /api/motion-log`
(columns: ms, x, y, z, the tap detector's delta, |reading|, MotionFlag bits;
values in centi-m/s^2). `replay_motion.ps1` feeds them through the real
`SensorManager` and prints what it reports.

| File | What was done | Expected |
| --- | --- | --- |
| `2026-11-17_taps_soft.csv` | 13 soft taps, cube upright on Face 1 | 12 taps (one peaked at 1.7, barely a touch) |
| `2026-11-17_taps_firm.csv` | 18 taps as people tap today, some hard | 18 taps, no slam |
| `2026-11-17_handling.csv` | pick up, put down, roll to another face and back, one shake | 0 taps, shake(s), no slam |

Before the 2026-11-17 tuning the same files gave 0, 4 and 2 (false) taps.
Record new ones on the real cube when the enclosure or the sensor mounting
changes, and add them here with what was done.
