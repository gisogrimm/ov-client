#!/bin/bash
(
    # minimal error handling:
    trap "echo An error occured.;exit 1" ERR

    export DEBIAN_FRONTEND=noninteractive

    echo "update apt database:"
    sudo -E apt-get update --assume-yes

    echo "Install CA certificates:"
    sudo -E apt-get install --no-install-recommends --assume-yes ca-certificates

    echo "install git, build-essential and other basic stuff:"
    sudo -E apt-get install --no-install-recommends --assume-yes git build-essential xxd gettext-base lsb-release

    echo "install dependencies of cli tool:"
    sudo -E apt-get install --no-install-recommends --assume-yes liblo-dev libcurl4-openssl-dev libasound2-dev libeigen3-dev libfftw3-dev libfftw3-single3 libgsl-dev libjack-jackd2-dev libltc-dev libmatio-dev libsndfile1-dev libsamplerate0-dev nlohmann-json3-dev libxerces-c-dev libgtkmm-3.0-dev libcairomm-1.0-dev

    echo "clone or update git repo:"
    (cd ov-client && git clean -ffx && git pull) || git clone https://github.com/gisogrimm/ov-client

    echo "move to development version:"
    (cd ov-client && git clean -ffx && git checkout development && git pull)

    echo "update submodules:"
    make -C ov-client gitupdate

    echo "clean repo:"
    make -C ov-client clean

    echo "build tools:"
    make -j 5 -C ov-client

    echo "package tools:"
    make -C ov-client packaging

    echo "install new debian packages:"
    sudo -E apt-get install --assume-yes --no-install-recommends ./ov-client/packaging/deb/debian/*/ovbox-cli_*.deb

    echo "install rtirq-init:"
    sudo -E apt-get install --assume-yes --no-install-recommends rtirq-init

    echo "update real-time priority priviledges:"
    sudo sed -i -e '/.audio.*rtprio/ d' -e '/.audio.*memlock/ d' /etc/security/limits.conf
    echo "@audio - rtprio 99"|sudo tee -a /etc/security/limits.conf
    echo "@audio - memlock unlimited"|sudo tee -a /etc/security/limits.conf

    echo "install user to run the scripts - do not provide root priviledges:"
    sudo useradd -m -G audio,dialout ov || echo "user ov already exists."

    echo "install autorun script:"
    sudo cp ov-client/tools/pi/autorun /home/pi/
    sudo chmod a+x /home/pi/autorun
    sudo chown pi:pi /home/pi/autorun

    echo "register autorun script in /etc/rc.local:"
    sudo sed -i -e '/exit 0/ d' -e '/.*autorun.*autorun/ d' -e '/.*home.pi.install.*home.pi.install/ d' -i /etc/rc.local
    echo "test -x /home/pi/autorun && su -l pi /home/pi/autorun &"|sudo tee -a /etc/rc.local
    echo "exit 0"|sudo tee -a /etc/rc.local

    echo "setup host name:"
    HOSTN=$(ov-client_hostname)
    echo "ovbox${HOSTN}" | sudo tee /etc/hostname
    sudo sed -i "s/127\.0\.1\.1.*/127.0.1.1\tovbox${HOSTN}/g" /etc/hosts
    sync

    echo "configure country code and prepare WiFi config:"
    sudo touch /boot/ovclient-wifi.txt
    sync

    echo "activate overlay image to avoid damage of the SD card upon power off:"
    sudo raspi-config nonint enable_overlayfs
    sync

    echo "ready, reboot:"
    sudo shutdown -r now

    sync
    echo "successfully installed ovbox system!"
)
