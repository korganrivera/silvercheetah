#!/bin/sh
/home/korgan/code/silvercheetah/silvercheetah
sudo -u korgan DISPLAY=:0 DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus notify-send "SILVERCHEETAH" "I am the cheetah!" --icon=starred
cp ~/code/silvercheetah/tss.log ~/Dropbox/tss.log
