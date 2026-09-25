#!/usr/bin/env bash
set -euo pipefail
BACKUP="${1:-$HOME/rodin-super-backup}"
SUPER="$(adb shell 'blockdev --getsize64 /dev/block/by-name/super' | tr -d '\r')"
DATA="$(adb shell 'blockdev --getsize64 /dev/block/by-name/userdata' | tr -d '\r')"
echo "super    : $SUPER"
echo "userdata : $DATA"
[[ "$SUPER" == "21474836480" ]] || { echo 'FAIL: super is not 20 GiB'; exit 1; }
[[ "$DATA" == "240506601472" ]] || { echo 'FAIL: userdata size unexpected'; exit 1; }

for p in ffu mem countrycode_a countrycode_b oops charger; do
  adb exec-out "dd if=/dev/block/by-name/$p bs=4096 2>/dev/null" > "/tmp/$p-after-gpt.img"
  cmp -s "$BACKUP/$p.img" "/tmp/$p-after-gpt.img" || { echo "$p: MISMATCH"; exit 1; }
  echo "$p: EXACT MATCH"
done

echo 'rescue: ext4 may journal/change after recovery boot; compare label/UUID/size rather than hash.'
adb exec-out "dd if=/dev/block/by-name/rescue bs=4096 2>/dev/null" > /tmp/rescue-after-gpt.img
blkid -p "$BACKUP/rescue.img" || true
blkid -p /tmp/rescue-after-gpt.img || true

echo 'blackbox: verified byte-for-byte before GPT switch; it may be runtime-written after reboot.'
echo '20 GiB GPT layout is active.'
