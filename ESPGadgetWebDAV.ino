
#include "FS.h"
#include "SPIFFS.h"
#include "WiFi.h"
#include <EEPROM.h>

#define FLASH_TEXT(name)   const char *name
#define BUFFER_SIZE 110
#define MAX_STRING 60

const char *default_ssid = "ESPGadget";
const char *default_password = "Admin12345";
char stored_ssid[32];
char stored_password[32];

FLASH_TEXT(HTTP_NOT_FOUND) = "HTTP/1.1 404 Not Found";
FLASH_TEXT(HTTP_200_FOUND) = "HTTP/1.1 200 OK";
FLASH_TEXT(HTTP_201_CREATED) = "HTTP/1.1 201 CREATED";
FLASH_TEXT(HTTP_201_MOVED) = "HTTP/1.1 201 MOVED";
FLASH_TEXT(HTTP_204_NO_CONTENT) = "HTTP/1.1 204 No Content";
FLASH_TEXT(HTTP_207_FOUND) = "HTTP/1.1 207 Multi Status";
FLASH_TEXT(HTTP_405_METHOD_NOT_ALLOWED) = "HTTP/1.1 405 Method Not Allowed";
FLASH_TEXT(HTTP_OPTIONS_HEADERS) = "Allow: PROPFIND, GET, DELETE, PUT, MOVE\nDAV: 1, 2";

FLASH_TEXT(HTTP_XML_CONTENT) = "Content-Type: application/xml;";
FLASH_TEXT(HTTP_HTML_CONTENT) = "Content-Type: text/html";
FLASH_TEXT(HTTP_CONTENT_TYPE) = "Content-Type: ";
FLASH_TEXT(HTTP_CONTENT_LENGTH) = "Content-Length: ";
FLASH_TEXT(MIME_JPEG) = "image/jpeg";
FLASH_TEXT(MIME_PNG) = "image/png";
FLASH_TEXT(MIME_TXT) = "text/plain";
FLASH_TEXT(MIME_BIN) = "application/octet-stream";
FLASH_TEXT(MULTISTATUS_START) = "<?xml version=\"1.0\" ?><D:multistatus xmlns:D=\"DAV:\">";
FLASH_TEXT(MULTISTATUS_END) = "</D:multistatus>";
FLASH_TEXT(RESPONSE_START) = "<D:response>";
FLASH_TEXT(RESPONSE_END) = "</D:response>";
FLASH_TEXT(HREF_START) = "<D:href>";
FLASH_TEXT(HREF_END) = "</D:href>";
FLASH_TEXT(PROPSTAT_START) = "<D:propstat>";
FLASH_TEXT(PROPSTAT_END) = "</D:propstat>";
FLASH_TEXT(PROP_START) = "<D:prop>";
FLASH_TEXT(PROP_END) = "</D:prop>";
FLASH_TEXT(CONTENTLEN_START) = "<D:getcontentlength>";
FLASH_TEXT(CONTENTLEN_END) = "</D:getcontentlength>";
FLASH_TEXT(RESOURCETYPE_START) = "<D:resourcetype>";
FLASH_TEXT(RESOURCETYPE_END) = "</D:resourcetype>";
FLASH_TEXT(RESOURCE_COLLECTION) = "<D:collection/>";
FLASH_TEXT(CREATEDATE_START) = "<D:creationdate>";
FLASH_TEXT(CREATEDATE_END) = "</D:creationdate>";
FLASH_TEXT(MODDATE_START) = "<D:getlastmodified>";
FLASH_TEXT(MODDATE_END) = "</D:getlastmodified>";
FLASH_TEXT(STATUS_OK) = "<D:status>HTTP/1.1 200 OK</D:status>";

WiFiServer server(80);


char stringBuffer[MAX_STRING];
char replaceBuffer[MAX_STRING];
char currentLineBuffer[MAX_STRING];



static char *str_replace(char *input, char *match, const char *substitute)
{
  byte offset = 0;
  char *search;
  while (search = strstr(input, match)) {
    memcpy(replaceBuffer + offset, input, search - input);
    offset += search - input;
    input = search + strlen(match);
    memcpy(replaceBuffer + offset, substitute, strlen(substitute));
    offset += strlen(substitute);
  }
  strcpy(replaceBuffer + offset, input);
  return replaceBuffer;
}

void ListFiles(WiFiClient client, const char *folderPath, File folder, int format) {
  Serial.println(F("Getting list of files"));
  Serial.println(folderPath);
  client.println(MULTISTATUS_START);
  File root = SPIFFS.open("/");
  File entry = root.openNextFile();
  if (!entry) {
    Serial.println("No files found");
  }
  while (entry) {
    client.print(RESPONSE_START);
    client.print(HREF_START);
    //client.print(folderPath);
    client.print(entry.name());
    //entry.printName(&client);
    client.print(HREF_END);
    client.print(PROPSTAT_START);
    client.print(PROP_START);
    if (entry.isDirectory()) {
      client.print(RESOURCETYPE_START);
      client.print(RESOURCE_COLLECTION);
      client.print(RESOURCETYPE_END);
    } else {
      client.print(CONTENTLEN_START);
      client.print(entry.size(), DEC);
      client.print(CONTENTLEN_END);
    }
    client.print(MODDATE_START);
    //entry.printModifyDateTime(&client);
    client.print(MODDATE_END);
    client.print(CREATEDATE_START);
    //entry.printCreateDateTime(&client);
    client.print(CREATEDATE_END);
    client.print(PROP_END);
    client.print(STATUS_OK);
    client.print(PROPSTAT_END);
    client.print(RESPONSE_END);

    entry.close();
    entry = root.openNextFile();
  }
  client.println(MULTISTATUS_END);

}



void not_found_404(WiFiClient client) {
  client.println(HTTP_NOT_FOUND);
  client.println(HTTP_HTML_CONTENT);
  client.println();
}

void not_allowed_405(WiFiClient client) {
  client.println(HTTP_405_METHOD_NOT_ALLOWED);
  client.println();
}

unsigned long readNextLongValue(WiFiClient client) {
  char rest_of_line[10];
  byte index = 0;
  unsigned long result = 0;
  while (client.connected()) {
    char c = client.read();
    if (c != ' ' && c != '\n' && c != '\r') {
      rest_of_line[index] = c;
      index++;
      if (index >= 10) {
        break;
      }
      continue;
    } else if (c == '\r') {
      client.read();                         //\n
      break;
    }
  }
  rest_of_line[index] = 0;
  result = atol(rest_of_line);
  return result;
}

unsigned long readContentLength(WiFiClient client) {
  unsigned long content_length = 0;
  while (client.connected()) {
    bool result = client.findUntil("ngth:", "\n");
    if (result) {
      break;
    }
  }
  content_length = readNextLongValue(client);
  Serial.println(content_length);
  return content_length;
}

char *readToEndOfLine(WiFiClient client) {
  byte index = 0;
  index = 0;
  while (client.connected()) {
    char c = client.read();
    if (c != '\n' && c != '\r') {
      currentLineBuffer[index] = c;
      index++;
      continue;
    } else {
      currentLineBuffer[index] = 0;
      break;
    }
  }
  return currentLineBuffer;
}

char *readDestination(WiFiClient client) {
  while (client.connected()) {
    bool result = client.findUntil("tion:", "\n");
    if (result) {
      break;
    }
  }
  client.read();         //space
  return readToEndOfLine(client);
}

void readUntilBody(WiFiClient client) {
  //Read until \r\n\r\n  
  while (client.connected()) {
    bool result = client.findUntil("\r\n\r\n", "\r\n\r\n");
    if (result) {
      break;
    }
  }
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println(F("Started"));
  Serial.println(default_ssid);
  Serial.println(default_password);
  if (!SPIFFS.begin(true)) {
    Serial.println(F("An Error has occurred while mounting SPIFFS"));
    return;
  }

  WiFi.softAP(default_ssid, default_password);

  //while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }

  IPAddress myIP = WiFi.softAPIP();
  Serial.print(F("AP IP address: "));
  Serial.println(myIP);
  server.begin();
  Serial.println(F("Server started."));
  EEPROM.begin(512);
  byte firstInstall = 0;
  EEPROM.get( 0, firstInstall );
  if (firstInstall != 128) {
    Serial.println(F("First install, formatting filesystem"));
    SPIFFS.format();
    firstInstall = 128;
    EEPROM.put(0, firstInstall);
    EEPROM.commit();
    Serial.println(F("File system formatted"));
    File dataFile = SPIFFS.open("test.txt", "w");
    dataFile.print("Hello World!");
    dataFile.close();
  } else {
    Serial.println(F("Not first install, don't format"));
  }
  Serial.print(F("###### Total bytes: "));
  Serial.println(SPIFFS.totalBytes());
  Serial.print(F("###### Used bytes: "));
  Serial.println(SPIFFS.usedBytes());
  //ESP.wdtDisable();         //Disable watchdog timer
}

void loop() {
  char request_line[BUFFER_SIZE];
  int index = 0;
  WiFiClient client = server.available();
  if (client) {
    index = 0;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c != '\n' && c != '\r') {
          request_line[index] = c;
          index++;
          if (index >= BUFFER_SIZE) {
            index = BUFFER_SIZE - 1;
          }
          continue;
        }
        request_line[index] = 0;
        Serial.println(request_line);
        (strstr(request_line, " HTTP"))[0] = 0;
        char *decodedRequest = str_replace(request_line, "%20", " ");
        //GET /folder/test.txt HTTP/1.1
        char *filename =  strcpy(decodedRequest, strstr(decodedRequest, " ") + 1);
        Serial.println(F("Working filename:"));
        Serial.println(filename);
        if (strstr(request_line, "PROPFIND ") != 0) {
          //curl --data "" --header "depth:1"  --header "Content-Type: text/xml" --request PROPFIND http://192.168.4.1/
          File dataFile = SPIFFS.open(filename, "r");
          if (!dataFile) {
            not_found_404(client);
            break;
          }
          if (dataFile.isDirectory()) {
            client.println(HTTP_207_FOUND);
            client.println(HTTP_XML_CONTENT);
            client.println("");
            ListFiles(client, filename, dataFile, 0);
          } else {
            not_allowed_405(client);
          }
          dataFile.close();
        } else if (strstr(request_line, "GET ") != 0) {
          filename = filename + 1;
          Serial.println(filename);
          File dataFile = SPIFFS.open(filename, "r");
          if (!dataFile) {
            not_found_404(client);
            break;
          }
          if (dataFile.isDirectory()) {
            ListFiles(client, filename, dataFile, 1);
          } else {
            client.println(HTTP_200_FOUND);
            client.print(HTTP_CONTENT_TYPE);
            if (strstr(request_line, ".jpg") != 0) {
              client.println(MIME_JPEG);
            } else if (strstr(request_line, ".png") != 0) {
              client.println(MIME_PNG);
            } else if (strstr(request_line, ".txt") != 0) {
              client.println(MIME_TXT);
            } else {
              client.println(MIME_BIN);
            }
            client.print(HTTP_CONTENT_LENGTH);
            client.print(dataFile.size(), DEC);
            client.println();
            client.println();
            uint8_t buf[42];
            int16_t num_read;
            while (dataFile.available()) {
              num_read = dataFile.read(buf, 42);
              client.write(&buf[0], 42);
            }
          }
          dataFile.close();
        } else if (strstr(request_line, "MOVE ") != 0) {
          char *destination = readDestination(client);

          Serial.println(destination);
          if (strncmp(destination, "http", 4) == 0) {
            destination = strstr(destination, "//");
            destination = strstr(destination + 2, "/");
          }
          filename = filename + 1;
          destination = destination + 1;
          Serial.println(destination);
          if (SPIFFS.rename(filename, destination)) {
            client.println(HTTP_201_MOVED);
          } else {
            client.println(HTTP_NOT_FOUND);
          }
          client.println();

          break;

        } else if (strstr(request_line, "PUT ") != 0) {
          unsigned long content_length = readContentLength(client);
          readUntilBody(client);
          File dataFile = SPIFFS.open(filename,  FILE_WRITE);
          byte buf[150];
          int num_read = 0;
          unsigned long total_read = 0;
          while (total_read < content_length) {
            num_read = client.read(buf, 150);
            if (num_read > 0) {
              dataFile.write(buf, num_read);
              total_read = total_read + num_read;
            } else {
              delay(1);
            }
          }
          dataFile.close();
          client.println(HTTP_201_CREATED);
          client.println();
          Serial.println(F("Saved file."));
          break;
        } else if (strstr(request_line, "DELETE ") != 0) {
          File dataFile = SPIFFS.open(filename, "w");
          if (!dataFile) {
            not_found_404(client);
            break;
          }
          if (SPIFFS.remove(filename)) {
            client.println(HTTP_204_NO_CONTENT);
            client.println();
          } else {
            not_found_404(client);
          }
          dataFile.close();
        } else if (strstr(request_line, "OPTIONS ") != 0) {
          client.println(HTTP_200_FOUND);
          client.println(HTTP_OPTIONS_HEADERS);
          client.println();
        } else {
          not_allowed_405(client);
        }
        break;

      }

    }
    delay(1);
    client.stop();
  }
}
