# SILVERCHEETAH
This is a tool I use to process .csv files generated by the Wahoo Fitness app while using the Kinetic trainer.

I specifically use this code with the following equipment:
* Wahoo Blue SC sensor
* Wahoo Fitness app
* Kinetic Road Machine

Silvercheetah can do the following:
* scan a folder for Wahoo csv files
* calculate NP, FTP, IF, and TSS from speed values
* interpolates missing workouts
* calculate ATL, CTL, TSB from TSS values
* write these to tss.log

I've set my Wahoo Fitness app to share csv files to a folder in Dropbox.

Silvercheetah can be set to run automatically with `incron` when new files are uploaded.

tss.log data can be displayed using gnuplot if you want.

I also use the following to get everything to run like I like it:
* Arch
* systemd
* notify-send
* incron

## HOW TO USE
* run `./silvercheetah`
* silvercheetah will create a config file for you to edit.
* put the path of your wahoo files in the config file.
* run `./silvercheetah`
* Files found will be processed and a tss.log will be created.


### What can I do with tss.log?
tss.log contains virtual power calculations using speed data taken from your wahoo csv files. It contains a unix timestamp, NP, FTP, IF, TSS, CTL, ATL, and TSB. This file can be plotted using gnuplot or whatever so you can view your training progress in a graph. You'll then be able to use this data to decide what your fitness level is, how much fatigue you can handle before burnout, and your daily TSS goals to improve your fitness as efficiently as possible.

### Can I use this code for training done with a different bike trainer?
Nope. The calculations are specifically tailored to the Kinetic Road Machine.

### Can I use this code for rides done outdoors?
Nope. Same as the above answer.

### Will this work with files other than the Wahoo Fitness .csv files?
Nope. The code expects the files to be of that specific format.

### How do I get csv files from the Wahoo Fitness app?
Once you've completed your workout with the Wahoo Fitness app, scroll to the bottom of your workout summary and click the 'share' icon. Choose 'csv'. You'll then be asked where to save it. I choose a folder in my Dropbox. Put the path of your files in the `config` file.

### How are you calculating FTP without an FTP test?
Look at each ride before the current ride. In each of those rides, find the highest power produced over a duration of 20 minutes. Multiply this by 0.95. This is your estimated FTP.

### This code sucks.
ikr. But if you have the same equipment that I do, it beats paying a subscription just to get the same data. :)

## How I get everything to run correctly
I personally run this code using a script that runs `silvercheetah`
and `notify-send` to let me know something happened. The 4-hour problem was
getting `notify-send` to run within a script. Turns out to be a solution
involving d-bus. I've no idea what that is (yet) but here's the script I use:

/home/korgan/code/silvercheetah/sc.sh:
```
#!/bin/sh
if [ -r ~/.dbus/Xdbus ]; then
  . ~/.dbus/Xdbus
fi
/home/korgan/code/silvercheetah/silvercheetah
notify-send "I am the cheetah!"
```
Remember to create the `~/.dbus` folder before running that.
Then I have a script which runs when I login and writes to that Xdbus file, and that one looks like this:

~/bin/dbus_silvercheetah.sh
```
#!/bin/sh
touch $HOME/.dbus/Xdbus
chmod 600 $HOME/.dbus/Xdbus
env | grep DBUS_SESSION_BUS_ADDRESS > $HOME/.dbus/Xdbus
echo 'export DBUS_SESSION_BUS_ADDRESS' >> $HOME/.dbus/Xdbus
exit 0
```
and then in `~/.config/openbox/autostart`, I have this:
```
# run this weird fucking dbus command so notify-send will fucking work with silvercheetah
/home/korgan/bin/dbus_silvercheetah.sh &
```
lastly, install `incron`, run `incrontab -e` and put this in there:
```
/home/korgan/Dropbox/cycling_files/csv_files	IN_MODIFY,IN_CREATE,IN_DELETE,IN_MOVE	/home/korgan/code/silvercheetah/sc.sh
```
Obviously this is for my laptop, but basically you put this in there:
```
<path of your csv files> IN_MODIFY,IN_CREATE,IN_DELETE,IN_MOVE <path of sc.sh>
```
To get incron to run now and also on startup, you have to do this:

```
systemctl start incrond.service
systemctl enable incrond.service
```

I think that's everything. It's a total hack but it works.

EDIT: one more thing. Doing the above like I did it, silvercheetah will run everytime you upload or remove a csv file from the Wahoo app. However, if you go a few days without working out, silvecheetah won't update the tss.log to show this unless you either run it manually or set it up to also run after midnight using cron, or something like cron. So you'd have both cron and incron running things. That seems like the best way if you need tss.log completely current at all times.


## TO-DO
I've put my own specific paths in `silvercheetah.c` and I need to change that so that anyone can set them in config.
Also, it would be nice if I wrote a script to set up all of the above nonsense so it was easier for someone else to use.

After that, I need to write another program that will be like your coach. It will tell you where you currently are in your training, and what you need to do next as far as TSS. I've written a modelling function that can match TSS to time and speed on the bike, so once it calculates how much TSS you need today, it can also recommend a duration and speed to work at to obtain it. This will be a separate program from silvercheetah since silvercheetah ran run on its own without intervention. You should also be able to set goals and have it taper your workout on approach to these.

Need to refactor my code also. There's some repetition I think I could avoid.

