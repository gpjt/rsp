#!/bin/bash
sudo apt-get update

sudo apt-get install -y vim build-essential git

sudo apt-get install python-dev
sudo apt-get install python-pip

sudo pip install fabric
sudo pip install pexpect
sudo pip install pyflakes
sudo pip install requests

git clone http://luajit.org/git/luajit-2.0.git
cd luajit-2.0
make && sudo make install
