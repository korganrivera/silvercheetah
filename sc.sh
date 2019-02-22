#!/bin/sh
if [ -r ~/.dbus/Xdbus ]; then
  . ~/.dbus/Xdbus
fi
/home/korgan/code/silvercheetah/silvercheetah
notify-send "I am the cheetah!"
