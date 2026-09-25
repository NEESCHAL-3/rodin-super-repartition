#!/usr/bin/env bash
set -euo pipefail
BACKUP="${1:-$HOME/rodin-super-backup}"
CONFIRM="${2:-}"
[[ "$CONFIRM" == "I_UNDERSTAND_DATA_WILL_BE_LOST" ]] || {
  echo 'REFUSING: this step destroys the existing /data filesystem.'
  echo 'Unmount /data, then run with I_UNDERSTAND_DATA_WILL_BE_LOST.'
  exit 2
}
if adb shell 'mount | grep -q " /data "'; then echo 'REFUSING: /data is mounted.'; exit 1; fi

move_verify() {
  local name="$1" start="$2" count="$3" img="$BACKUP/$1.img" rb="/tmp/$1-stage-readback.img"
  echo "===== $name WRITE ====="
  adb exec-in "dd of=/dev/block/sdc bs=4096 seek=$start count=$count conv=fsync 2>/dev/null" < "$img"
  adb exec-out "dd if=/dev/block/sdc bs=4096 skip=$start count=$count 2>/dev/null" > "$rb"
  cmp -s "$img" "$rb" || { echo "VERIFY FAILED: $name"; exit 1; }
  echo "$name: VERIFIED"
}
move_verify ffu 5767168 2048
move_verify mem 5769216 1024
move_verify countrycode_a 5770240 256
move_verify countrycode_b 5770496 256
move_verify oops 5770752 4096
move_verify charger 5774848 256
move_verify rescue 5775104 32768
move_verify blackbox 5807872 41984
adb shell sync
echo 'All moved partitions staged and byte-for-byte verified.'
echo 'DO NOT REBOOT until the new GPT is written.'
