#!/usr/bin/env bash
set -euo pipefail
OUT="${1:-out/stock}"
CONFIRM="${2:-}"
[[ "$CONFIRM" == "RESTORE_STOCK_GPT" ]] || { echo 'REFUSING: pass RESTORE_STOCK_GPT after staging stock partition locations.'; exit 2; }
if adb shell 'mount | grep -Eq " /data | /system | /system_ext | /product | /vendor | /odm "'; then
  echo 'REFUSING: data or a dynamic partition is mounted.'; exit 1
fi
for f in primary-header.bin primary-entries.bin backup-header.bin backup-entries.bin; do
  adb push "$OUT/$f" "/tmp/stock-$f" >/dev/null
done
adb shell 'dd if=/tmp/stock-backup-entries.bin of=/dev/block/sdc bs=4096 seek=64571384 count=3 conv=fsync'
adb shell 'dd if=/tmp/stock-backup-header.bin of=/dev/block/sdc bs=4096 seek=64571391 count=1 conv=fsync'
adb shell 'dd if=/tmp/stock-primary-entries.bin of=/dev/block/sdc bs=4096 seek=2 count=3 conv=fsync'
adb shell 'dd if=/tmp/stock-primary-header.bin of=/dev/block/sdc bs=4096 seek=1 count=1 conv=fsync'
adb shell sync
echo 'Stock GPT written. Reboot DIRECTLY back to recovery.'
echo 'Expected after reboot: super=11811160064, userdata=250170277888.'
echo 'Then Format Data and flash an 11-GiB-compatible super image.'
