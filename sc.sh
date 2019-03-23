#!/bin/sh
if [ -r ~/.dbus/Xdbus ]; then
  . ~/.dbus/Xdbus
fi
/home/korgan/code/silvercheetah/silvercheetah
notify-send "SILVERCHEETAH" "I am the cheetah!" --icon=starred
