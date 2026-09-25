#!/usr/bin/env bash
set -euo pipefail
OUT="${1:-out/dev20g}"
CONFIRM="${2:-}"
[[ "$CONFIRM" == "WRITE_20G_GPT" ]] || { echo 'REFUSING: pass WRITE_20G_GPT after staging moved partitions.'; exit 2; }
if adb shell 'mount | grep -q " /data "'; then echo 'REFUSING: /data is mounted.'; exit 1; fi

for f in primary-header.bin primary-entries.bin backup-header.bin backup-entries.bin; do
  adb push "$OUT/$f" "/tmp/$f" >/dev/null
  pc="$(sha256sum "$OUT/$f" | awk '{print $1}')"
  dev="$(adb shell "sha256sum /tmp/$f" | tr -d '\r' | awk '{print $1}')"
  [[ "$pc" == "$dev" ]] || { echo "Push hash mismatch: $f"; exit 1; }
done

# Backup GPT first, then primary GPT.
adb shell 'dd if=/tmp/backup-entries.bin of=/dev/block/sdc bs=4096 seek=64571384 count=3 conv=fsync'
adb shell 'dd if=/tmp/backup-header.bin of=/dev/block/sdc bs=4096 seek=64571391 count=1 conv=fsync'
adb shell 'dd if=/tmp/primary-entries.bin of=/dev/block/sdc bs=4096 seek=2 count=3 conv=fsync'
adb shell 'dd if=/tmp/primary-header.bin of=/dev/block/sdc bs=4096 seek=1 count=1 conv=fsync'
adb shell sync

check_raw() {
  local name="$1" skip="$2" count="$3" rb="/tmp/${name}-readback.bin"
  adb exec-out "dd if=/dev/block/sdc bs=4096 skip=$skip count=$count 2>/dev/null" > "$rb"
  [[ "$(sha256sum "$OUT/$name.bin" | awk '{print $1}')" == "$(sha256sum "$rb" | awk '{print $1}')" ]] || { echo "Readback failed: $name"; exit 1; }
  echo "$name: VERIFIED"
}
check_raw backup-entries 64571384 3
check_raw backup-header 64571391 1
check_raw primary-entries 2 3
check_raw primary-header 1 1

echo 'GPT write/readback complete. Reboot DIRECTLY back to recovery.'
