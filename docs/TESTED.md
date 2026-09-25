# Tested status

Validated on a POCO X7 Pro / Redmi Turbo 4 (`rodin`) on **2026-09-25**.

## What was actually validated

1. Original primary GPT header and entry-array CRCs were valid.
2. Both GPT ends were backed up.
3. `ffu`, `mem`, `countrycode_a`, `countrycode_b`, `oops`, `charger`, `rescue`, `blackbox`, and `flashinfo` were backed up.
4. The 20 GiB GPT was generated offline from the exact original GPT.
5. Every partition moved from `ffu` through `blackbox` was written to its future raw LBA and read back byte-for-byte before the GPT switch.
6. Backup GPT entries/header were written and read back successfully.
7. Primary GPT entries/header were written and read back successfully.
8. GPT header and partition-array CRC32 values were validated after writing.
9. Recovery rebooted successfully with the new GPT.
10. Kernel reported `super = 21,474,836,480` bytes and `userdata = 240,506,601,472` bytes.
11. After reboot, `ffu`, `mem`, `countrycode_a`, `countrycode_b`, `oops`, and `charger` remained exact hash matches.
12. `rescue` remained the same ext4 filesystem (same UUID, label, filesystem size) but its journal/metadata changed after recovery boot.
13. `blackbox` matched before GPT switching and changed after reboot, consistent with runtime-written diagnostic/log data.
14. Format Data succeeded and the resized userdata mounted as f2fs.
15. Fastboot accepted a sparse `super.img` for the new 20 GiB physical `super` partition.

## Boundary of this validation

The physical repartition is validated. A specific ROM boot is a separate matter: its LP metadata and any required boot/vendor_boot/init_boot/vbmeta images must also be correct.
