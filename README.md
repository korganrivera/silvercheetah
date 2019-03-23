# SILVERCHEETAH
This is a tool I use to process .csv files generated by the Wahoo Fitness app while using the Kinetic trainer. Produces a file called tss.log which contains a ton of information about your workouts, like NP, FTP, TSS, duration, ATL, CTL, TSB.

My other program cyclecoach can take tss.log and give you training advice to optimise your fitness while staying without your freshness threshold.

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

Silvercheetah can be set to run automatically with `incron` and `fcron` when new files are uploaded.

tss.log data can also be displayed using gnuplot if you want.

I also use the following to get everything to run like I like it:
* Arch
* systemd
* notify-send
* incron
* fcron

## HOW TO USE
* run `./silvercheetah`
* silvercheetah will create a config file for you to edit.
* put the path of your wahoo files in the config file.
* run `./silvercheetah`
* Files found will be processed and a tss.log will be created.

To automate silvercheetah, follow the instructions below.

### What can I do with tss.log?
tss.log contains virtual power calculations using speed data taken from your wahoo csv files. It contains a unix timestamp, NP, FTP, IF, TSS, CTL, ATL, and TSB. This file can be plotted using gnuplot or whatever so you can view your training progress in a graph. You'll then be able to use this data to decide what your fitness level is, how much fatigue you can handle before burnout, and your daily TSS goals to improve your fitness as efficiently as possible. tss.log can also be further processed with `cyclecoach`.

### Can I use this code for training done with a different bike trainer?
Nope. The calculations are specifically tailored to the Kinetic Road Machine.

### Can I use this code for rides done outdoors?
Nope. Same as the above answer.

### Will this work with files other than the Wahoo Fitness .csv files?
Nope. The code expects the files to be of that specific format.

### How do I get csv files from the Wahoo Fitness app?
Once you've completed your workout with the Wahoo Fitness app, scroll to the bottom of your workout summary and click the 'share' icon. Choose 'csv'. You'll then be asked where to save it. I choose a folder in my Dropbox. Put the path of your files in the `config` file.

### How are you calculating FTP without an FTP test?
Look at each ride before the current ride. In each of those rides, find the highest power produced over a duration of 20 minutes. Multiply this by 0.95. This is your workout's FTP. Then, your current FTP is the highest workout_FTP that you've obtained within the last few months. Boom - no need to take scheduled FTP tests. Unless you're a masochist.

## How I get everything to run automatically

Create a working folder first.
`mkdir ~/silvercheetah`

Put the repository in there, and run `make`. Then run ./silvercheetah manually one time. This will create a config file into which you will put the path of your csv files. If everything went well, you can run ./silvercheetah again and it will then create a file called tss.log (assuming you actually have wahoo csv files in the given folder.)

Once that's successful, follow these steps to get silvercheetah to run autonomously:

Make this folder
`mkdir ~/.dbus`

Install notify-send if you haven't already if you want on-screen notifications when silvercheetah detects your file changes. (recommended)

create ~/silvercheetah/sc.sh. This is a script that will run silvercheetah and notify-send.
```
#!/bin/sh
if [ -r ~/.dbus/Xdbus ]; then
  . ~/.dbus/Xdbus
fi
~/silvercheetah/silvercheetah
notify-send "I am the cheetah!"
```
create ~/bin/dbus_sc.sh. This dbus stuff is needed so the script can push notify-send messages to the screen.
```
#!/bin/sh
touch $HOME/.dbus/Xdbus
chmod 600 $HOME/.dbus/Xdbus
env | grep DBUS_SESSION_BUS_ADDRESS > $HOME/.dbus/Xdbus
echo 'export DBUS_SESSION_BUS_ADDRESS' >> $HOME/.dbus/Xdbus
exit 0
```
Add this to your autostart. With Openbox, I do this:

in `~/.config/openbox/autostart`:

`~/bin/dbus_sc.sh &`

If you don't have incron installed already, do that. Make sure it will run now and on startup. With systemd, I do this
```
systemctl start incrond.service
systemctl enable incrond.service
```
Add this line to your incrontab:
```
/home/korgan/Dropbox/cycling_files/csv_files	IN_MODIFY,IN_CREATE,IN_DELETE,IN_MOVE	~/silvercheetah/sc.sh
```
Obviously this is the line I use. Change the above so that it uses the path that you keep your csv files in.

Now your tss.log will update every time a file is added, deleted, or changed. However, I also want it to update at midnight just in case I skipped a workout. I also want this to happen reliably if the laptop has been shutdown at any point. For this, I use fcron.
So, install fcron, enable and start that in systemctl, then add this line to your fcrontab:
```
# run silvercheetah at midnight so cycling off-days are appended
0 0 * * * /home/korgan/code/silvercheetah/sc.sh
@reboot /home/korgan/code/silvercheetah/sc.sh
```
I think that's everything. It's a total hack but it works.

## TO-DO
I've put my own specific paths in `silvercheetah.c` and I need to change that so that anyone can set them in config.
Also, it would be nice if I wrote a script to set up all of the above nonsense so it was easier for someone else to use.
Need to refactor my code also. There's some repetition I think I could avoid.

## Issues
So this isn't a big deal but all the timestamps are in UTC and I don't do any local time conversion or account for Daylight Saving Time. This means that if you, say, do a workout one morning and do another workout the next night, they might show up in your tss.log as happening a whole day apart even though in your timezone they happened on consecutive days. But what happened is that a whole day in UTC time occured between those workouts. If this bothers you, go ahead and fork the code and write your own local time handling routines; I'm probably not going to fix this.
