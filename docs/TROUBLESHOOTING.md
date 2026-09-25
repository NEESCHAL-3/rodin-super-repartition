# Troubleshooting

## `rescue` hash changes after reboot

During validation `rescue` matched exactly before the GPT switch. After reboot it changed because it is an ext4 filesystem that recovery can journal/update. The filesystem still had the same label, UUID and size.

## `blackbox` hash changes after reboot

During validation it matched byte-for-byte immediately after staging. It changed after recovery reboot, so treat it as runtime-writable diagnostic/log storage. The important relocation check is the pre-GPT readback verification.

## Fastboot prints `skip copying super image avb footer due to sparse image`

That message alone is not a failure. A sparse image may be sent in many chunks. The command still needs `OKAY` on sends/writes and must finish without `FAILED`.

## Fastboot still reports 11 GiB

Do not flash a >11 GiB logical image. Return to recovery and verify:

```bash
adb shell 'blockdev --getsize64 /dev/block/by-name/super'
```

20 GiB must report `21474836480`.

## `/data` will not mount after the GPT switch

Expected until Format Data is performed because userdata starts at a new LBA.

## Device stops enumerating

Do not improvise further raw writes. Use your original backups and your known low-level restore/unbrick path.
