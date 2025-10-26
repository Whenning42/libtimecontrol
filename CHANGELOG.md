0.2.0 - 2025-10-25

Updated API so that TimeController's no longer take channels.
Channels are now selected automatically and are hidden from
callers.

0.1.5 - 2025-10-23

Add a pyproject description.

0.1.4 - 2025-10-23

FIXED:
- Seqlock and SockReadStruct helper class correctness fixes. Fixes should
  improve correctness and stability respectively.

0.1.3 - 2025-07-14

FIXED:
- Incorrect fake times when time controller and controllee have mismatched bitness
- Improved libtimecontrol stability in forked processes

0.1.1 - 2025-07-07

Initial Release
