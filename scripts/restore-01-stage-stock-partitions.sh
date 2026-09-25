#!/usr/bin/env bash
set -euo pipefail
BACKUP="${1:-$HOME/rodin-super-backup}"
CONFIRM="${2:-}"
[[ "$CONFIRM" == "I_UNDERSTAND_SUPER_AND_DATA_WILL_BE_DESTROYED" ]] || {
  echo 'REFUSING: restoring stock locations overwrites part of the current 20 GiB super and requires a later data format.'
  echo 'Run again with I_UNDERSTAND_SUPER_AND_DATA_WILL_BE_DESTROYED.'
  exit 2
}
if adb shell 'mount | grep -Eq " /data | /system | /system_ext | /product | /vendor | /odm "'; then
  echo 'REFUSING: data or a dynamic partition is mounted.'; exit 1
fi

move_verify() {
  local name="$1" start="$2" count="$3" img="$BACKUP/$1.img" rb="/tmp/$1-stock-readback.img"
  echo "===== RESTORE $name ====="
  adb exec-in "dd of=/dev/block/sdc bs=4096 seek=$start count=$count conv=fsync 2>/dev/null" < "$img"
  adb exec-out "dd if=/dev/block/sdc bs=4096 skip=$start count=$count 2>/dev/null" > "$rb"
  cmp -s "$img" "$rb" || { echo "VERIFY FAILED: $name"; exit 1; }
  echo "$name: VERIFIED"
}
move_verify ffu 3407872 2048
move_verify mem 3409920 1024
move_verify countrycode_a 3410944 256
move_verify countrycode_b 3411200 256
move_verify oops 3411456 4096
move_verify charger 3415552 256
move_verify rescue 3415808 32768
move_verify blackbox 3448576 41984
adb shell sync
echo 'Stock small partitions staged. DO NOT REBOOT until the stock GPT is restored.'
