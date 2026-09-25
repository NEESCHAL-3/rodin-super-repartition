# rodin physical super repartition

Two OrangeFox/TWRP-style packages are provided:

- **RODIN-Super-Expand-20G** — changes physical `super` from 11 GiB to 20 GiB
- **RODIN-Super-Restore-11G** — returns the physical GPT to the factory 11 GiB layout

The 20 GiB layout takes exactly 9 GiB from the front of `userdata`.

## Safety design

The installer validates the exact rodin storage geometry and both GPT CRCs,
readback-verifies every moved partition, writes the backup GPT before the primary
GPT, and aborts on unknown layouts rather than attempting a best-effort write.

No personal GPT image is bundled in the release ZIPs. GPT identifiers and other
entry fields already present on the target device are preserved.

## Mandatory after flashing either ZIP

**Reboot directly back to recovery, then Format Data.**

Do not boot Android first. The start LBA of `userdata` changes in both directions.

When restoring to 11 GiB, the current 20 GiB `super` contents are destroyed by
design; flash an 11 GiB-compatible `super.img` after formatting data.

See `docs/RECOVERY-ZIPS.md` and `docs/RESTORE-STOCK.md` for details.
