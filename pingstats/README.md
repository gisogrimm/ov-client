# Measure ping time statistics

To measure the ping time statistics, please install TASCAR for data
logging, and Octave for visualization:

````
sudo apt install tascarpro octave
````

Then start jack. Starting TASCAR with the datalogging file will
automatically start the ov-client on your system with activated ping
time logging:

````
tascar datalogging.tsc
````

Please note that only the ping times from your client to the other
peers in the same session as your client are recorded.

Press `start` to start recording of ping times, and `stop` to stop
it. After recording your data, start Octave. In Octave, please type:

````
load unnamed_20201120_172752.mat
plot_pinghist(ping)
````

