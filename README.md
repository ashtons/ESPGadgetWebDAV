# ESPGadgetWebDAV
WebDAV server running on an ESP32 wifi module

Currently supports basic PROPFIND, MOVE, DELETE, PUT and GET requests

Uses SPIFFS filesystem, formats filesystem on initial run.

Doesn't support folders, only files in the root folder

Tested on ESP8266-01, with 1MB flash 

Updated for ESP32 with 4MB flash  [TTGO T-Display ESP32](http://www.lilygo.cn/prod_view.aspx?TypeId=50033&Id=1126&FId=t3:50033:3)

Starts in AP mode

SSID: ESPGadget

Password: Admin12345

### Sample queries

#### Get a list of files
    curl -v --data "" --header "depth:1"  --header "Content-Type: text/xml" --request PROPFIND http://192.168.4.1/
  
 
#### Download a file 
    curl -v "http://192.168.4.1/test.txt"

#### Upload a file
    curl -T test.txt http://192.168.4.1/test.txt

#### Move/Rename a file
    curl -X MOVE --header 'Destination: http://192.168.4.1/new_name.txt' 'http://192.168.4.1/test.txt'


## Add ESP tools to Arduino IDE
Open Arduino IDE

Preferences

Set Additional Boards Manager URLs to "https://dl.espressif.com/dl/package_esp32_index.json, http://arduino.esp8266.com/stable/package_esp8266com_index.json"

Open the Boards Manager and search for ESP32, click install


## Problems install ESP tools on MacOS

https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/mac.md

