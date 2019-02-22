# SILVERCHEETAH
This is a tool I use to process .csv files generated by the Wahoo Fitness app while using the Kinetic trainer.

I specifically use this code with the following equipment:
* Wahoo Blue SC sensor
* Wahoo Fitness app
* Kinetic Road Machine

Silvercheetah can do the following:
* scan a folder for new Wahoo csv files
* strip speed values from each file
* calculate FTP and TSS from speed values
* interpolates missing workouts
* calculate ATL, CTL, TSB from TSS values
* write these to tss.log

I've set my Wahoo Fitness app to share csv files to a folder in Dropbox.

Silvercheetah can be set to run with inotify or cron.

tss.log can be displayed using gnuplot if you want.

## HOW TO USE
* The default folder for your wahoo csv files is ./wahoo_csv_files.
* If it doesn't exist, create it.
* put some wahoo csv files in there.
* run `./silvercheetah`
* Files found in there will be processed and tss.log will be created.

### What if I want to use a different folder?
* Put the folder path that you want to use on the first line of the config file, no trailing slash. For example, `~/Dropbox/wahoo_files`
* Run `./silvercheetah`
* Files found in there will be processed and a new tss.log will be created.

### What can I do with tss.log?
tss.log contains virtual power calculations using speed data taken from your wahoo csv files. It contains values for FTP, TSS, CTL, ATL, TSB. This file can be plotted using gnuplot or whatever so you can view your training progress in a graph. You'll then be able to use this data to decide what your fitness level is, how much fatigue you can handle before burnout, and your daily TSS goals to improve your fitness as efficiently as possible.

### Can I use this code for training done with a different bike trainer?
Nope. The calculations are specifically tailored to the Kinetic Road Machine.

### Can I use this code for rides done outdoors?
Nope. Same as the above answer.

### Will this work with files other than the Wahoo Fitness .csv files?
Nope. The code expects the files to be of that specific format.

### How do I get csv files from the Wahoo Fitness app?
Once you've completed your workout with the Wahoo Fitness app, scroll to the bottom of your workout summary and click the 'share' icon. Choose 'csv'. You'll then be asked where to save it. I choose a folder in my Dropbox. Put the path of your files in the `config` file: see "What if I want to use a different folder?" above.

### This code sucks.
ikr. But if you have the same equipment that I do, it beats paying a subscription just to get the same data. :)

### How can I plot tss.log with gnuplot?
* install gnuplot
* run `gnuplot`
* enter `set datafile separator ","`
* enter `plot 'tss.log' u 1:2 w l title 'FTP', 'tss.log' u 1:3 w l title 'TSS', 'tss.log' u 1:4 w l title 'CTL (fitness)', 'tss.log' u 1:5 w l title 'ATL (fatigue)', 'tss.log' u 1:6 w l title 'TSB (freshness)'`

## NOTES
I personally run this code using a script that runs silvercheetah and notify-send to let me know something happened. The 4-hour problem was getting notify-send to run within a script. Turns out to be a solution involving d-bus. I've no idea what that is (yet) but here's the script I use:

sc.sh:
```
#!/bin/sh
if [ -r ~/.dbus/Xdbus ]; then
  . ~/.dbus/Xdbus
fi
/home/korgan/code/silvercheetah/silvercheetah
notify-send "I am the cheetah!"
```

Then I have a script which runs when I login that writes that Xdbus file, and that one looks like this:

~/bin/dbus_silvercheetah.sh
```
#!/bin/sh
touch $HOME/.dbus/Xdbus
chmod 600 $HOME/.dbus/Xdbus
env | grep DBUS_SESSION_BUS_ADDRESS > $HOME/.dbus/Xdbus
echo 'export DBUS_SESSION_BUS_ADDRESS' >> $HOME/.dbus/Xdbus
exit 0
```
and then in ~/.config/openbox/autostart, I have this:
```
# run this weird fucking d-bus command so notify-send will fucking work for silvercheetah
/home/korgan/bin/dbus_silvercheetah.sh &
```

## TO-DO
I've put my own specific paths in silvercheetah.c and I need to change that so that anyone can set them in config.

