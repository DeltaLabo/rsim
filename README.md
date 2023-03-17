Room sound intensity meter
============

#CAMBIAR TODO

### What is this repository for? ###

* This repository was created to develop and ESP32 based embedded logger and cloud uploader for wind turbine power generation, and wind velocity data for LIENE.

### Turbine info

* [Info page](https://mwands.com/missouri-raptor-g4-wind-turbine-generator) 

### Anemometer info

* Model: Inspeed Vortex
* [Documentation page](http://www.old.inspeed.com/anemometers/Vortex_Wind_Sensor.asp)

### Microcontroller info

* Model: HELTEC LoRa 32
* [Documentation page](https://heltec.org/project/wifi-lora-32/)
* [Pinout diagram](https://resource.heltec.cn/download/WiFi_LoRa_32/WIFI_LoRa_32_V2.pdf)
* [Schematic](https://resource.heltec.cn/download/WiFi_LoRa_32/V2/WIFI_LoRa_32_V2(868-915).PDF)
* [ESP32 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf) 

### Enclousure

* [Page](https://www.se.com/es/es/product/NSYCRN33200P/spacial-crn-plain-door-with-mount.plate.-h300xw300xd200-ip66-ik10-ral7035../) 
* [Page](https://mazcr.com/gabinetes-cajas-y-accesorios-plasticos/458040-caja-paso-162x212x110.html)

### How do I get set up? ###

* Install Git
* Install Arduino IDE
* Follow this [instructions](https://heltec-automation-docs.readthedocs.io/en/latest/esp32/quick_start.html) 
* Install Heltec ESP32 Library in the Arduino Library Manager
Open Arduino IDE, then Select `Sketch`->`Include Library`->`Manage Libraries...`
Search `Heltec ESP32` and install it.
* Install Adafruit MQTT Library in the Arduino Library Manager
Open Arduino IDE, then Select `Sketch`->`Include Library`->`Manage Libraries...`
Search `Adafruit MQTT` and install it.

### Library source code and examples
* [Adafruit_MQTT_Library](https://github.com/adafruit/Adafruit_MQTT_Library)

### Contribution guidelines ###

* If you want to propose a review or need to modify the code for any reason first clone this [repository](https://github.com/DeltaLabo/anemos) in your PC and create a new branch for your changes. Once your changes are complete and fully tested ask the administrator permission to push this new branch into the source.
* If you just want to do local changes instead you can download a zip version of the repository and do all changes locally in your PC. 

### Who do I talk to? ###

* [Juan J. Rojas](mailto:juan.rojas@itcr.ac.cr)
* [Nestor Martinez](mailto:arnold7martinez@gmail.com)
