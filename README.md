# Monitor SD card health and status using Raspberry Pi
Get S.M.A.R.T data for Transcend SD card.

-------------------------
The various statuses of MicroSD products are located in the SMART block of the device. 
By reading and analyzing the SMART block, you can get the product name, serial number, firmware version, Erase count, health, etc. of the device, so as to monitor and find the 
device early potential problems. This document provides a way to use the the test tool that get SMART block to enable SMART command via the native MicroSD port on embedded linux such as Raspberry Pi or Wiren Board.

Hardware requirements
-------------------------
* Embedded Linux with native SD interface (/dev/mmcblk*)
* Transcend Micro SD 430T / 450I

Usage
-------------------------

```
$ sudo ./transcend-sdcard-health –h
``` 
1. Get SMART buffer by following command.
```
$ sudo ./transcend-sdcard-health smtbuffer /dev/mmcblk0
``` 
2. Get SMART information by following command.
```
$ sudo ./transcend-sdcard-health smart /dev/mmcblk0
``` 
3. Get Card life by following command.
```
$ sudo ./transcend-sdcard-health health /dev/mmcblk0
``` 

Command Execution
-------------------------
1. To dump SMART raw data</br></br>
![Buffer](https://github.com/transcend-information/RaspberryPi-SDcard-SMARTQuery/blob/main/smtbuffer.png)

2. To show SMART info</br></br>
![SMART](https://github.com/transcend-information/RaspberryPi-SDcard-SMARTQuery/blob/main/smart.png)

3. To show card life(Heath)</br></br>
![Heath](https://github.com/transcend-information/RaspberryPi-SDcard-SMARTQuery/blob/main/health.png)
