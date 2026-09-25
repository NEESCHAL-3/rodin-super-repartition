# Technical layout

All GPT LBAs in this repository use the device's **4096-byte logical block size**.

The Linux `/sys/class/block/sdc*/start` values observed on-device are reported in 512-byte units, so they appear 8x larger than the GPT LBAs used here.

## Device geometry

- Disk: `/dev/block/sdc`
- Disk size: `264,484,421,632` bytes
- Logical block size: `4096`
- GPT LBAs: `64,571,392`
- Primary GPT header: LBA `1`
- Primary GPT entries: LBA `2`, 3 sectors
- Backup GPT entries: LBA `64,571,384`, 3 sectors
- Backup GPT header: LBA `64,571,391`
- GPT entries: 93 entries × 128 bytes

## Stock layout

| # | Partition | Start | End | Size |
|---:|---|---:|---:|---:|
| 83 | super | 524288 | 3407871 | 11 GiB |
| 84 | ffu | 3407872 | 3409919 | 8 MiB |
| 85 | mem | 3409920 | 3410943 | 4 MiB |
| 86 | countrycode_a | 3410944 | 3411199 | 1 MiB |
| 87 | countrycode_b | 3411200 | 3411455 | 1 MiB |
| 88 | oops | 3411456 | 3415551 | 16 MiB |
| 89 | charger | 3415552 | 3415807 | 1 MiB |
| 90 | rescue | 3415808 | 3448575 | 128 MiB |
| 91 | blackbox | 3448576 | 3490559 | 164 MiB |
| 92 | userdata | 3490560 | 64567287 | 250,170,277,888 bytes |
| 93 | flashinfo | 64567288 | 64571383 | 16 MiB |

## 20 GiB development layout

The block `ffu` through `blackbox` moves forward exactly `2,359,296` 4K LBAs = `9,663,676,416` bytes = 9 GiB.

| # | Partition | Start | End | Size |
|---:|---|---:|---:|---:|
| 83 | super | 524288 | 5767167 | 20 GiB |
| 84 | ffu | 5767168 | 5769215 | 8 MiB |
| 85 | mem | 5769216 | 5770239 | 4 MiB |
| 86 | countrycode_a | 5770240 | 5770495 | 1 MiB |
| 87 | countrycode_b | 5770496 | 5770751 | 1 MiB |
| 88 | oops | 5770752 | 5774847 | 16 MiB |
| 89 | charger | 5774848 | 5775103 | 1 MiB |
| 90 | rescue | 5775104 | 5807871 | 128 MiB |
| 91 | blackbox | 5807872 | 5849855 | 164 MiB |
| 92 | userdata | 5849856 | 64567287 | 240,506,601,472 bytes |
| 93 | flashinfo | 64567288 | 64571383 | 16 MiB |

`super` keeps its original start. `flashinfo` does not move. Only the start of `userdata` changes, therefore Format Data is mandatory.

## Why partitions 84-91 move

They physically sit between stock `super` and `userdata`. Extending `super` in place would overlap them. The safe layout copies them forward by exactly 9 GiB, verifies the copies, and only then switches GPT.

## Android dynamic partitions

This project changes only the physical GPT partition named `super`.

`system`, `product`, `vendor`, `odm`, etc. are logical partitions described by Android LP metadata inside the super image. A 20 GiB physical container does not automatically resize those logical partitions; the `super.img` must already contain valid LP metadata that fits the new container.
