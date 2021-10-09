#!/bin/bash
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 imagefile mount-directory" >&2
    exit 1
fi
if ! [ -e "$1" ]; then
    echo "image file $1 not found." >&2
    exit 1
fi
if ! [ -e "$2" ]; then
    echo "mount directory $2 not found." >&2
    exit 1
fi
if ! [ -d "$2" ]; then
    echo "$2 is not a directory" >&2
    exit 1
fi
# minimal error handling:
trap "echo an error occured.;exit 1" ERR
#
SUFFIX=ovinstaller
IMGBASE=$(basename "$1")
sudo losetup -f -P "$1"
DEVNAME=$(losetup -l|grep -F -e "${IMGBASE}"|tail -1|sed -e 's/[[:blank:]].*//1')
echo "----------"
echo "mounting $1 ($DEVNAME) on $2"
# Mount boot partition:
sudo mount ${DEVNAME}p1 "$2"
cat "$2"/cmdline.txt | grep -F -e 'init=/usr/lib/raspi-config/init_resize.sh' || (echo "The image was booted already."; exit 1;)
sudo umount "$2"
# Now mount rootfs partition:
sudo mount ${DEVNAME}p2 "$2"
SRCPATH=`dirname $0`
(
    cd ${SRCPATH}
    sudo cp ../installovclient.sh "$2/home/pi/install"
    sudo cp install_ovclient.sh "$2/usr/lib/raspi-config/install_ovclient.sh"
    sudo chmod a+x "$2/usr/lib/raspi-config/install_ovclient.sh"
    # In /mnt/usr/lib/raspi-config/init_resize.sh line 187 modify line
    sudo sed -i -e "\+init=/usr/lib/raspi-config/init_resize+ s/.*/sed -i \'s| init=\/usr\/lib\/raspi-config\/init_resize\\\.sh| init=\/usr\/lib\/raspi-config\/install_ovclient\\\.sh|\' \/boot\/cmdline.txt/1" "$2/usr/lib/raspi-config/init_resize.sh"
)
sudo umount "$2"
if [ "${DIGITALSTAGE}" = "yes" ]; then
    # add config file to /boot partition
    SUFFIX=ds-ovinstaller
    sudo mount ${DEVNAME}p1 "$2"
    echo '{"protocol":"ov","ui":"https://digital-stage.ovbox.de/","url":"http://digital-stage-device.ovbox.de/"}' | sudo tee "$2/ov-client.cfg"
    sudo umount "$2"
fi
sync
sudo losetup -d ${DEVNAME}
SRC="${1}"
echo "renaming to ${SRC/.img}-${SUFFIX}.img"
mv "${SRC}" "${SRC/.img}-${SUFFIX}.img"
echo "compressing to ${IMGBASE/.img}-${SUFFIX}.zip"
zip "${IMGBASE/.img}-${SUFFIX}.zip" "${SRC/.img}-${SUFFIX}.img"
