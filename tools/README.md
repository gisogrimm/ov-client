# tool set for OVBOX on Raspberry Pi

This folder contains shell scripts to perform an installation on a Raspberry Pi. The folder `pi` contains autorun shell scripts, which will be installed on the Raspberry Pi.

## Preparation of installer image with apt repo, 32 Bit

Download Raspberry Pi OS lite image, 32 Bit, based on debian buster [here](https://downloads.raspberrypi.com/raspios_oldstable_lite_armhf/images/raspios_oldstable_lite_armhf-2023-05-03/), or run:

```
rm -f 2023-05-03-raspios-buster-armhf-lite.img*
wget https://downloads.raspberrypi.com/raspios_oldstable_lite_armhf/images/raspios_oldstable_lite_armhf-2023-05-03/2023-05-03-raspios-buster-armhf-lite.img.xz
unxz 2023-05-03-raspios-buster-armhf-lite.img.xz
./prepareimage.sh 2023-05-03-raspios-buster-armhf-lite.img /mnt
```

Tested on Ubuntu 20.04 host system, and Raspberry Pi 4B.

## Preparation of installer image with apt repo, 64 Bit

Download Raspberry Pi OS lite image, 64 Bit, based on debian bullseye [here](https://downloads.raspberrypi.com/raspios_oldstable_lite_arm64/images/raspios_oldstable_lite_arm64-2024-07-04/), or run:

```
rm -f 2024-07-04-raspios-bullseye-arm64-lite.img*
wget https://downloads.raspberrypi.com/raspios_oldstable_lite_arm64/images/raspios_oldstable_lite_arm64-2024-07-04/2024-07-04-raspios-bullseye-arm64-lite.img.xz
unxz 2024-07-04-raspios-bullseye-arm64-lite.img.xz
./prepareimage.sh 2024-07-04-raspios-bullseye-arm64-lite.img /mnt
```

Tested on Ubuntu 20.04 host system, and Raspberry Pi 4B.

## Preparation of installer image with installation from source code, development version

Download Raspberry Pi OS lite image, 64 Bit, based on debian bookworm [here](https://downloads.raspberrypi.com/raspios_lite_arm64/images/raspios_lite_arm64-2024-07-04/), or run:

```
rm -f 2024-07-04-raspios-bookworm-arm64-lite.img*
wget https://downloads.raspberrypi.com/raspios_lite_arm64/images/raspios_lite_arm64-2024-07-04/2024-07-04-raspios-bookworm-arm64-lite.img.xz
unxz 2024-07-04-raspios-bookworm-arm64-lite.img.xz
./prepareimage_pi5.sh 2024-07-04-raspios-bookworm-arm64-lite.img /mnt
```

Tested on Ubuntu 20.04 host system, and Raspberry Pi 4B. Installation takes at least 25 minutes due to compilation and installation of all packages required for compilation. Compilation may fail on hardware with low memory.

Currently not working on Raspberry Pi 5B (ov-client crashes with "Bus error").

## Preparation of ready-to-use image

First, completely install an ovbox system using a method from above. Then remove the SD card from the Raspberry Pi and copy the disk image (make sure to replace `/dev/disk` by the name of the SD card drive, e.g. `/dev/sdd`):

```
sudo dd if=/dev/disk of=ovbox_ready.img status=progress bs=128M oflag=sync
sudo chown $USER ovbox_ready.img
```

Then set up loopback device, in order to clear zeros and shrink it:

```
sudo losetup -f -P ovbox_ready.img
DEVNAME=$(losetup -l|grep -F -e "ovbox_ready"|tail -1|sed -e 's/[[:blank:]].*//1')
echo $DEVNAME
```

Now shrink filesystem with
```
sudo gparted $DEVNAME
```

Then clear zeros:

```
sudo zerofree ${DEVNAME}p2
```

Now list partitions:

```
sudo fdisk -l $DEVNAME
```
Identify the end sector, and follow [softwarebakery.com/shrinking-images-on-linux](https://softwarebakery.com/shrinking-images-on-linux) to truncate (replace numbers by end sector):

```
truncate --size=$[(9181183+1)*512] myimage.img
```



See also:
https://superuser.com/questions/1373289/how-do-i-shrink-the-partition-of-an-img-file-made-from-dd
https://softwarebakery.com/shrinking-images-on-linux
https://unix.stackexchange.com/questions/44234/clear-unused-space-with-zeros-ext3-ext4
