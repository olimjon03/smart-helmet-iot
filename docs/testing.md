# Bench test plan and validation status

## Recorded checks

| Check | Result |
| --- | --- |
| Compile for generic ESP32 | Passed; see [build record](build.md) |
| Source review and phone-number removal | Completed for the publication files |
| Hardware demonstration media | Physical prototype visible in the supplied video and photos |
| Controlled impact detection | Not recorded |
| GPS freshness and failure handling on hardware | Not recorded |
| Recipient SMS delivery | Not recorded |

## Tests to run

Use a bench setup and an intended test recipient. Do not stage a crash or test impacts on a person. Record the board, firmware revision, dependency versions and sanitized serial logs for each run.

| Scenario | Observation to record |
| --- | --- |
| Default empty recipient | SMS is refused with a configuration message |
| Sensor disconnected at boot | Actual errors and ready LED behavior |
| Stationary helmet | Measured acceleration/rotation and false triggers |
| Controlled motion crossing either threshold | Trigger timing and countdown start |
| Button press during countdown | Countdown stops and outputs return low |
| Manual SOS press and release | Reproduce the known immediate-cancellation interaction before changing it |
| No GPS fix | SMS content contains `GPS: NO FIX` |
| Valid then stale GPS fix | Determine location age at send time; current code has no freshness guard |
| Modem disconnected or unregistered | Prompt/result timeout and failure output |
| Intended recipient configured | Modem response and independently observed receipt; record them separately |
| Continued movement after alert | Repeated-trigger behavior without a cooldown |

Do not label detection accuracy, heart-rate/SpO₂ readings, network reliability or battery runtime as tested until measurements are supplied.
