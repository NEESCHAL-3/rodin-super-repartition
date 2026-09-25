#!/usr/bin/env bash
set -euo pipefail

echo '===== DEVICE ====='
adb shell 'getprop ro.product.device; getprop ro.product.vendor.device' | tr -d '\r'

echo
echo '===== ROOT ====='
adb shell 'id'

echo
echo '===== STORAGE ====='
adb shell '
echo -n "sdc bytes: "
blockdev --getsize64 /dev/block/sdc
echo -n "super bytes: "
blockdev --getsize64 /dev/block/by-name/super
echo -n "userdata bytes: "
blockdev --getsize64 /dev/block/by-name/userdata
'

echo
echo '===== PHYSICAL GEOMETRY 83-93 ====='
adb shell '
for p in sdc83 sdc84 sdc85 sdc86 sdc87 sdc88 sdc89 sdc90 sdc91 sdc92 sdc93
do
  printf "%s start=" "$p"; cat /sys/class/block/$p/start
  printf "%s size=" "$p"; cat /sys/class/block/$p/size
done
'

cat <<'TXT'

Expected stock core sizes:
  /dev/block/sdc              264484421632
  super                        11811160064
  userdata                    250170277888

Do not continue if device/storage geometry differs from profiles/rodin.json.
TXT
