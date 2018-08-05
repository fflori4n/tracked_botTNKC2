#!/bin/bash
file="$1"
sudo g++ -o prog $file.cpp `pkg-config --cflags --libs cairo gtk+-3.0 `  -pthread -Wfatal-errors -std=gnu++11 # -lcurl #--libs


#sudo apt-get install libcairo2-dev
#sudo apt-get install libgtk-3-dev

