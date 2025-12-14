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
USEFIRMW=""
SEDFIRMW=""
if test -e "$2"/firmware/cmdline.txt; then
    USEFIRMW="/firmware"
    SEDFIRMW="\/firmware"
fi
cat "$2"/"${USEFIRMW}"/cmdline.txt | grep -e 'init=.*/init_resize.sh' -e 'init=.*/firstboot' -e "resize" || (echo "The image was booted already."; ls "/$2";cat "$2"/"${USEFIRMW}"/cmdline.txt;exit 1;)

# set password:
echo 'pi:$6$BfzNyd/.tlSynCad$HX/TpdQ35vqP2ahCHIrQbUXxzc8ld3CSW.Nb6pwIqP5/vSIxtO3IunfIiI/mmgzSulbbDwIO9jORU6n/wdbsB0' | sudo tee "$2"/userconf.txt
sudo umount "$2"

# Now mount rootfs partition:
sudo mount ${DEVNAME}p2 "$2"
SRCPATH=`dirname $0`
(
    cd ${SRCPATH}
    sudo cp install_pi5 "$2/home/pi/install"
    sudo chmod a+x "$2/home/pi/install"
    #if test -e "$2/usr/lib/raspi-config/init_resize.sh"; then
    #    # In /mnt/usr/lib/raspi-config/init_resize.sh line 187 modify line
    #    sudo sed -i -e "\+init=/usr/lib/raspi-config/init_resize+ s/.*/sed -i \'s| init=\/usr\/lib\/raspi-config\/init_resize\\\.sh| init=\/usr\/lib\/raspi-config\/install_ovclient\\\.sh|\' \/boot${SEDFIRMW}\/cmdline.txt/1" "$2/usr/lib/raspi-config/init_resize.sh"
    #fi
    #if test -e "$2/usr/lib/raspberrypi-sys-mods/firstboot"; then
    #    sudo sed -i -e "\+init=/usr/lib/raspberrypi-sys-mods/firstboot+ s/.*/sed -i \'s| init=\/usr\/lib\/raspberrypi-sys-mods\/firstboot| init=\/usr\/lib\/raspi-config\/install_ovclient\\\.sh|\' \/boot${SEDFIRMW}\/cmdline.txt/1" "$2/usr/lib/raspberrypi-sys-mods/firstboot"
    #fi

    echo "install autorun script:"
    sudo cp pi/autorun_pi5 "$2/home/pi/autorun"
    sudo chmod a+x  "$2/home/pi/autorun"
    # The next line does not work on host system:
    #sudo chown pi:pi "$2/home/pi/autorun"

    echo "register autorun script in /etc/rc.local:"
    sudo touch "$2/etc/rc.local"
    sudo sed -i -e '/.*bin.sh.*/ d' "$2/etc/rc.local"
    sudo echo "" | sudo tee -a "$2/etc/rc.local" 
    sudo sed -i '1s/^/#!\/bin\/sh\n/' "$2/etc/rc.local"
    sudo sed -i -e '/exit 0/ d' -e '/.*autorun.*autorun/ d' -e '/.*home.pi.install.*home.pi.install/ d' -i "$2/etc/rc.local"
    echo "test -x /home/pi/autorun && su -l pi /home/pi/autorun &"|sudo tee -a "$2/etc/rc.local"
    echo "exit 0"|sudo tee -a  "$2/etc/rc.local"
    sudo chmod a+x "$2/etc/rc.local"
    
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
