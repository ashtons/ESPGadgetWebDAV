# ESPGadgetWebDAV
WebDAV server running on an ESP8266 wifi module

In development, currently supports basic PROPFIND, MOVE, PUT and GET requests

Uses SPIFFS filesystem, formats filesystem on initial run.
Doesn't support folders, only files in the root folder

Tested on ESP8266-01, with 1MB flash 

Starts in AP mode
SSID: ESPGadget
Password: Admin12345

Sample queries

    curl -v --data "" --header "depth:1"  --header "Content-Type: text/xml" --request PROPFIND http://192.168.4.1/
  
  
    curl -v "http://192.168.4.1/test.txt"
