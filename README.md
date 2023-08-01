Room sound intensity meter
============

### Project status

> This section will be removed after development is complete.

- At first the XIAO ESP32 could be programmed by following the guide linked in the setup section. However, it now throws `Failed uploading: uploading error: exit status 2` whenever *any* code is uploaded.
- The audio input from the mic was succesfully read via I2S and plotted.

### To-do

> This section will be removed after development is complete.

- [ ] Determine why the ESP32 can't be programmed via the Arduino IDE.
- [ ] Determine why the audio input is in 16-bit format instead of 24-bit like the datasheet says.
- [ ] Verify that the maximum and minimum inputs are 120 and 33 dB SPL.
- [ ] Convert the microphone input to decibels (SPL).
- [ ] Design the block diagram for the alert system.
- [ ] Write the docs.

### What is this repository for? ###

* This repository was created to develop an ESP32-based ambient noise level meter, that can send alerts via instant messaging.

### Microcontroller info

* Model: Seeed Studio XIAO ESP32C3
* [Documentation page](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/)
* [Pinout diagram](https://files.seeedstudio.com/wiki/XIAO_WiFi/pin_map-2.png)
* [ESP32 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)

### Microphone info

* Model: INMP441 I2S digital microphone
* [Datasheet](https://invensense.tdk.com/wp-content/uploads/2015/02/INMP441.pdf)

### How do I set up? ###

* Install Git
* Install Arduino IDE
* Follow these [instructions](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/#getting-started) to set up the XIAO ESP32 in the Arduino IDE
* Connect the components as shown in this diagram

![Connection diagram for the XIAO ESP32C3 microcontroller and the INMP441 digital microphopne.](./connection_diagram.png)

* Clone this repo and upload `main.ino` to the ESP32

### Contribution guidelines ###

* If you want to propose a change or need to modify the code for any reason first clone this [repository](https://github.com/DeltaLabo/rsim) to your PC and create a new branch for your changes. Once your changes are complete and fully tested ask the administrator permission to push this new branch into the source.
* If you just want to do local changes instead you can download a zip version of the repository and do all changes locally in your PC. 

### Who do I talk to? ###

* [Juan J. Rojas](mailto:juan.rojas@itcr.ac.cr)
