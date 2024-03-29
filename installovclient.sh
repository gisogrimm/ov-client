#!/bin/bash
(
    # minimal error handling:
    trap "echo an error occured.;exit 1" ERR

    export DEBIAN_FRONTEND=noninteractive

    # identify OS release code name:
    OSREL=`lsb_release -a|grep Codename|sed 's/Codename:[[:blank:]]*//1'`
    OSRELHTCH="$OSREL"
    if test "$OSREL" = "buster"; then
        OSRELHTCH=bionic
    fi
    # identify OS architecture:
    OSARCH=`dpkg-architecture | grep -e 'DEB_BUILD_ARCH='|sed 's/[^=]*=//1'`

    # update main system
    # in case someone powered off during installation we need to fix unconfigured packages:
    sudo dpkg --configure -a
    sync
    # install dependencies:
    # retry 10 times because the raspios servers are not very stable:
    cnt=10
    while test $cnt -gt 0; do
	sudo -E apt update && cnt=-1 || ( sleep 20; let cnt=$cnt-1 )
	sync
    done
    if test $cnt -eq 0; then
        echo "failed 10 times, switching to GWDG mirror"
        sudo sed -i -e '/raspbian/ d' -e '/^[[:blank:]]*$/ d' /etc/apt/sources.list|| echo "unable to remove previous apt entries"
        (echo "";echo "deb http://ftp.gwdg.de/pub/linux/debian/raspbian/raspbian/ ${OSREL} main contrib non-free rpi")|sudo tee -a /etc/apt/sources.list
        sync
        cnt=10
        while test $cnt -gt 0; do
	    sudo -E apt update && cnt=0 || ( sleep 20; let cnt=$cnt-1 )
	    sync
        done
    fi
    cnt=10
    while test $cnt -gt 0; do
	sudo -E apt upgrade --assume-yes && cnt=0 || ( sleep 20; let cnt=$cnt-1 )
	sync
    done


    # add/update HoerTech apt repository (containing ov-client):
    sudo sed -i -e '/apt.hoertech.de/ d' -e '/^[[:blank:]]*$/ d' /etc/apt/sources.list|| echo "unable to remove previous apt entries"
    wget -qO- http://apt.hoertech.de/openmha-packaging.pub | sudo apt-key add -
    (echo "";echo "deb [arch=${OSARCH}] http://apt.hoertech.de ${OSRELHTCH} universe")|sudo tee -a /etc/apt/sources.list
    sync

    # in case someone powered off during installation we need to fix unconfigured packages:
    sudo dpkg --configure -a
    sync
    # install dependencies:
    # retry 10 times because the raspios servers are not very stable:
    cnt=10
    while test $cnt -gt 0; do
	sudo -E apt update && cnt=0 || ( sleep 20; let cnt=$cnt-1 )
	sync
    done
    cnt=10
    while test $cnt -gt 0; do
	sudo -E apt upgrade --assume-yes && cnt=0 || ( sleep 20; let cnt=$cnt-1 )
	sync
    done
    cnt=10
    while test $cnt -gt 0; do
	sudo -E apt install --no-install-recommends --assume-yes ov-client libnss-mdns && cnt=0 || ( sleep 20; let cnt=$cnt-1 )
	sync
    done

    # install user to run the scripts - do not provide root priviledges:
    sudo useradd -m -G audio,dialout ov || echo "user already exists."
    sync

    # get autorun file:
    rm -f autorun
    if test -e /usr/share/ovclient/tools/autorun; then
	# prefer the version provided with ov-client package:
	cp /usr/share/ovclient/tools/autorun .
    else
	# if not available then install from git:
	wget https://github.com/gisogrimm/ov-client/raw/master/tools/pi/autorun
    fi
    chmod a+x autorun
    sync

    # update real-time priority priviledges:
    sudo sed -i -e '/.audio.*rtprio/ d' -e '/.audio.*memlock/ d' /etc/security/limits.conf
    echo "@audio - rtprio 99"|sudo tee -a /etc/security/limits.conf
    echo "@audio - memlock unlimited"|sudo tee -a /etc/security/limits.conf
    sync

    # register autorun script in /etc/rc.local:
    sudo sed -i -e '/exit 0/ d' -e '/.*autorun.*autorun/ d' -e '/.*home.pi.install.*home.pi.install/ d' -i /etc/rc.local
    echo "test -x /home/pi/autorun && su -l pi /home/pi/autorun &"|sudo tee -a /etc/rc.local
    echo "exit 0"|sudo tee -a /etc/rc.local
    sync

    # setup host name
    HOSTN=$(ov-client_hostname)
    echo "ovbox${HOSTN}" | sudo tee /etc/hostname
    sudo sed -i "s/127\.0\.1\.1.*/127.0.1.1\tovbox${HOSTN}/g" /etc/hosts
    sync

    # configure country code and prepare WiFi config:
    sudo touch /boot/ovclient-wifi.txt
    sync

    # activate overlay image to avoid damage of the SD card upon power off:
    sudo raspi-config nonint enable_overlayfs
    sync

    # ready, reboot:
    sudo shutdown -r now
)
