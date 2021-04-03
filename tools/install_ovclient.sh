#!/bin/sh

reboot_pi () {
    umount /boot
    mount / -o remount,ro
    sync
    echo b > /proc/sysrq-trigger
    sleep 5
    exit 0
}

mount -t proc proc /proc
mount -t sysfs sys /sys
mount -t tmpfs tmp /run
mkdir -p /run/systemd

mount /boot

sed -i 's| init=/usr/lib/raspi-config/install_ovclient\.sh||' /boot/cmdline.txt

mount /boot -o remount,ro
mount / -o remount,rw

sync

# get autorun file:
rm -f autorun
if ! test -e /home/pi/install; then
    echo "wget https://github.com/gisogrimm/ov-client/raw/master/installovclient.sh" > /home/pi/install
    echo ". installovclient.sh" >> /home/pi/install
fi
chmod a+x /home/pi/install

# register autorun script in /etc/rc.local:
sed -i -e '/exit 0/ d' -e '/.*home.pi.install.*home.pi.install/ d' -i /etc/rc.local
echo "test -x /home/pi/install && su -l pi /home/pi/install &"|tee -a /etc/rc.local
echo "exit 0"|tee -a /etc/rc.local

reboot_pi
