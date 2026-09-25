# Recovery flashable packages

The release ZIPs are the easy frontend for the same physical repartition process
documented in this repository.

## Assets

- `RODIN-Super-Expand-20G-vX.Y.zip`
- `RODIN-Super-Restore-11G-vX.Y.zip`
- `SHA256SUMS.txt`

The ZIPs are intended for OrangeFox/TWRP-style recovery on `rodin`.

## Expand: 11 GiB -> 20 GiB

Flash `RODIN-Super-Expand-20G-vX.Y.zip`.

The installer:

1. validates that recovery reports device `rodin`;
2. validates the exact `/dev/block/sdc` size;
3. validates both GPT headers and both GPT partition-entry CRC32 values;
4. accepts only the documented stock, development, or recoverable mixed layout;
5. extracts its static AArch64 helper before unmounting `/data`;
6. refuses to continue while `/data` or a dynamic partition remains mounted;
7. copies `ffu` through `blackbox` to their +9 GiB locations;
8. readback-compares every byte of every moved partition;
9. writes the backup GPT first and fsyncs it;
10. writes the primary GPT and fsyncs it;
11. re-reads both GPT copies and verifies the final 20 GiB layout.

After the ZIP reports success:

1. reboot directly back to recovery;
2. **Format Data**;
3. reboot recovery once;
4. flash the 20 GiB-compatible `super.img`.

Expected physical `super` size after reboot:

```text
21474836480
```

## Restore: 20 GiB -> stock 11 GiB

Flash `RODIN-Super-Restore-11G-vX.Y.zip`.

This intentionally destroys the current 20 GiB `super` contents while the
small partitions are placed back at their factory LBAs. It also changes the
start of `userdata` back to stock.

After success:

1. reboot directly back to recovery;
2. **Format Data**;
3. reboot recovery once;
4. flash an 11 GiB-compatible `super.img`.

Expected physical `super` size after reboot:

```text
11811160064
```

## Why the ZIP does not Format Data itself

The recovery kernel continues using the partition map it booted with until a
reboot. Formatting before reboot would operate against the stale kernel view of
`userdata`.

Therefore the safe boundary is:

```text
flash repartition ZIP
-> reboot recovery
-> Format Data
```

## Native helper

`bin/rodin-gpt` is a tiny statically linked AArch64 ELF built from
`recovery-installer/helper/rodin_gpt.c` and `start.S`.

It does not depend on Python, `sgdisk`, `parted`, or recovery-specific GPT tools.
The helper performs exact GPT CRC validation, raw partition copying/readback,
and backup-GPT-first updates.

## Already on the requested layout

Both packages are idempotent at the layout level:

- flashing Expand while already on 20 GiB exits without writing;
- flashing Restore while already on stock 11 GiB exits without writing.

## Interrupted transitions

The helper accepts a recoverable mixed state where one valid GPT copy contains
the stock geometry and the other valid GPT copy contains the 20 GiB geometry.
Re-running the appropriate ZIP completes the requested target layout.

If GPT CRCs are invalid or the geometry is unknown, the installer aborts instead
of guessing.
