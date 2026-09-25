# Restore the factory 11 GiB layout

This reverses the 20 GiB developer layout and returns the device to the original physical GPT geometry.

It is destructive: the current 20 GiB super contents and `/data` are considered disposable during rollback.

## Requirements

- the backup created before repartitioning
- `out/stock/` generated from that same backup
- custom recovery with root ADB
- no mounted `/data`, `system`, `system_ext`, `product`, `vendor`, or `odm`
- an 11-GiB-compatible `super.img`

Never use somebody else's stock GPT output.

## 1. Generate stock GPT files

```bash
scripts/02-generate.sh ~/rodin-super-backup out
```

`out/stock/` contains the original GPT structures extracted from your own backup.

## 2. Boot recovery and unmount everything relevant

```bash
adb reboot recovery
adb shell '
twrp unmount data 2>/dev/null || true
mount | grep -E " /data | /system | /system_ext | /product | /vendor | /odm " || echo ALL_CLEAR
'
```

## 3. Restore the small partitions to their stock LBAs

```bash
scripts/restore-01-stage-stock-partitions.sh \
  ~/rodin-super-backup \
  I_UNDERSTAND_SUPER_AND_DATA_WILL_BE_DESTROYED
```

This writes into the lower part of the current 20 GiB super by design. Do not reboot afterward.

## 4. Restore the stock GPT

```bash
scripts/restore-02-stock-gpt.sh out/stock RESTORE_STOCK_GPT
```

The backup GPT is written first, then the primary GPT.

## 5. Reboot directly back to recovery and verify

```bash
adb reboot recovery
adb shell '
echo -n "super: "; blockdev --getsize64 /dev/block/by-name/super
echo -n "userdata: "; blockdev --getsize64 /dev/block/by-name/userdata
'
```

Expected:

```text
super: 11811160064
userdata: 250170277888
```

## 6. Format Data

Use recovery's **Format Data** action. The userdata start LBA changed again, so this is mandatory.

## 7. Flash an 11-GiB-compatible super image

```bash
adb reboot bootloader
fastboot getvar partition-size:super
fastboot flash super super.img
```

The stock 11 GiB physical super size is `0x2C0000000`.

Do not flash a 20 GiB-layout super image after restoring the stock GPT.
