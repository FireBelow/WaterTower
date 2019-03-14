#!/bin/sh

#Get Weather data from OpenWeatherMap.org
# 1>>file_name appends stdOUT to file
# 2>>file_name appends stdERR to file

#TODO:

echo "***Update List"
#apt list --installed
sudo apt-get update
echo "***Upgrade Packages"
sudo apt-get upgrade
echo "***Autoremove Old Packages"
sudo apt-get autoremove
# remove old archive package downloads
# sudo apt-get autoclean
# remove package downloads
# sudo apt-get clean

#sudo apt-get autoclean
#sudo apt-get purge
#echo "***Update Pi Firmware"
#sudo rpi-update		#updates RaspPi firmware
#echo "***Update pip3"
# pip --version
# which pip
# pip3 --version
# which pip3
#sudo pip3 install pip3 --upgrade
#sudo pip3 install numpy --upgrade  #this broke the python3-numpy installation so had to "sudo pip3 uninstall numpy" then "sudo apt-get install python3-numpy"
# sudo pip3 install scipy --Upgrade  #this also forces numpy pip3 upgrade which breaks the numpy library link
#sudo pip3 install matplotlib --upgrade
# sudo pip3 install setuptools --upgrade
#sudo pip3 install pandas --upgrade
#sudo pip3 install requests --upgrade
# sudo pip3 install pyopenssl --upgrade
# sudo pip3 install pip3 --upgrade

#sudo pip3 install $(pip list --outdated | awk '{ print $1 }') --upgrade
echo "***Done!"

exit 0