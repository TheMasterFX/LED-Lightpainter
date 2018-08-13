/*
 * LED-Lightpainter - A DIY Pixelstick clone for Lightpainting using the ESP8266 and a WS2812 Strip (Neopixel)
 * 
 * Copyright (C) 2018 Timmo Hellemann 
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * 
 * 
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <FS.h>   // Include the SPIFFS library
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <string.h>
#include "LED_Painter.h"

ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80

File fsUploadFile;              // a File object to temporarily store the received file

//Gamma correction Table
const uint8_t gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };



struct Config{
  int no_of_leds;
  int led_pin;
  int line_time;
  int trigger_pin;
  char image_to_draw[32];
  char wifi_mode[4];
  char sta_ssid[32];
  char sta_pass[64];
  char ap_ssid[32];
  char ap_pass[32];
};

#define TRIGGER_PIN D2

const char *config_filename = "/config.json"; 
Config configuration = {60,14,20,TRIGGER_PIN,"/test.bmp","sta","YourSSID","YourPass","LED_PainterAP","ledpainter"};

String getContentType(String filename); // convert the file extension to the MIME type
bool handleFileRead(String path);       // send the right file to the client (if it exists)
void handleFileUpload();                // upload a new file to the SPIFFS
void handleFileUploadDialog();
void handleSuccess();
void handleFileList();
void handleConfig();
void handleBrowseWifi();
void handleRoot();
void handleTrigger();
int load_config();
int write_config();
int start_sta();
int start_ap();
void drawBMP(char *filename);

int start_sta(){
  wifiMulti.addAP(configuration.sta_ssid, configuration.sta_pass);   // add Wi-Fi networks you want to connect to

    
  Serial.println("Connecting ...");
  int i = 0;
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
    if(i > 10){
      Serial.println('\n');
      Serial.print("Failed to Connect: Timeout ");
      return -1;
    }
  }
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer
}

int start_ap(){
  WiFi.softAP(configuration.ap_ssid, configuration.ap_pass);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(configuration.ap_ssid);
  Serial.println("\" started");

  Serial.print("IP address:\t");
  Serial.println(WiFi.softAPIP());  

}

void setup() {
  

  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');

  pinMode(configuration.trigger_pin, INPUT_PULLUP);

  SPIFFS.begin();                           // Start the SPI Flash Files System

  //if no config file found, write config with defaults
  
  if(!SPIFFS.exists(config_filename)){
    write_config();    
  }
  else{
    load_config();
  }

  //Start AP mode when wifi mode is AP or Trigger-Pin is pressed
  if(!strcmp(configuration.wifi_mode, "sta") && digitalRead(configuration.trigger_pin) ){
    delay(1000); //wait a second to avoid bounce

    if(start_sta() < 0){
      start_ap();
    }    
  }
  else{
    start_ap();  
  }

  if (!MDNS.begin("esp8266")) {             // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");

  server.on("/upload", HTTP_GET, []() {                 // if the client requests the upload page
    handleFileUploadDialog();
  });

  server.on("/list", HTTP_GET, [](){
      handleFileList();
  });

  server.on("/config", HTTP_GET, [](){
      handleConfig();
  });

  server.on("/upload", HTTP_POST,                       // if the client posts to the upload page
    [](){ server.send(200); },                          // Send status 200 (OK) to tell the client we are ready to receive
    handleFileUpload                                    // Receive and save the file
  );

  server.on("/action", HTTP_GET, [](){
    handleTrigger();
  });

   server.on("/", HTTP_GET, [](){
    handleRoot();
  });

  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  
  if(digitalRead(configuration.trigger_pin) == 0){
    delay(200); //make sure not boucing
    //still pressed
    if(digitalRead(configuration.trigger_pin) == 0){
      Serial.println(millis());
      delay(3000); //just wait 3 seconds until drawing bitmap
      drawBMP(configuration.image_to_draw);
      Serial.print("Drawing done ");
      Serial.println(millis());
    }
  }
}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".bmp")) return "image/bmp";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

void handleRoot(){
    String page = FPSTR(HTTP_HEAD);
    page.replace("{v}", "LED-Lightpainter");
    page += FPSTR(HTTP_STYLE);
    page += FPSTR(HTTP_JS_IMAGE);
    page += FPSTR(HTTP_HEAD_END);
    page += F("<h1>LED-Lightpainter</h1><br />");
    page += F("<a href=\"/config\">Configuration</a><br />");
    page += F("<a href=\"/upload\">Upload File</a><br />");
    page += F("<a href=\"/list\">Select Image</a><p />");
    page += F("<form action=\"/action\" method=\"get\"><button name=\"action\" value=\"trigger\" type=\"submit\">Draw Image</button></form>");

    page += FPSTR(HTTP_END);

    server.send(200, "text/html", page);


}

void handleTrigger(){
    String page = FPSTR(HTTP_HEAD);
    page.replace("{v}", "Trigger");
    page += FPSTR(HTTP_STYLE);
    page += FPSTR(HTTP_JS_IMAGE);
    page += FPSTR(HTTP_HEAD_END);
    page += F("<h1>LED-Lightpainter</h1><br />");
    page += F("<a href=\"/\">Back to Index</a><br />");
    if(server.hasArg("action")){
      if(server.arg("action").equals("trigger")){
        page += F("Drawing in 3 Seconds<br />");
        
        drawBMP(configuration.image_to_draw);
      }
    }
    page += FPSTR(HTTP_END);

    server.send(200, "text/html", page);
    

}


bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client

    
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    Serial.println(String("\tSent size: ") + sent);
    return true;
    
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void handleFileUpload(){ // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      handleSuccess();
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void handleFileUploadDialog(){
    String page = FPSTR(HTTP_HEAD);
    page.replace("{v}", "File Upload");
    page += FPSTR(HTTP_STYLE);
    page += FPSTR(HTTP_JS_IMAGE);
    page += FPSTR(HTTP_HEAD_END);
    page += F("<h1>File Upload</h1><br />");
    page += F("<form action=\"/upload\" method=\"POST\" enctype=\"multipart/form-data\">");
    page += F("<input type=\"file\" name=\"data\">");
    page += F("<input type=\"submit\" value=\"Upload\">");
    page += F("</form>");
    
    page += FPSTR(HTTP_END);

    server.send(200, "text/html", page);

}

void handleSuccess(){
  String page = FPSTR(HTTP_HEAD);
    page.replace("{v}", "Upload Success");
    page += FPSTR(HTTP_STYLE);
    page += FPSTR(HTTP_HEAD_END);

    page += F("<h1>ESP8266 SPIFFS File Upload Successful</h1>");
    page += F("<p><a href=\"/\">Home</a></p>");

    page += FPSTR(HTTP_END);
    server.send(200, "text/html", page);
}

const String formatBytes(size_t const& bytes) {            // lesbare Anzeige der Speichergrößen
  return (bytes < 1024) ? String(bytes) + " Byte" : (bytes < (1024 * 1024)) ? String(bytes / 1024.0) + " KB" : String(bytes / 1024.0 / 1024.0) + " MB";
}


void handleFileList(){
    FSInfo fs_info;  SPIFFS.info(fs_info);    // Füllt FSInfo Struktur mit Informationen über das Dateisystem
    Dir dir = SPIFFS.openDir("/");            // Auflistung aller im Spiffs vorhandenen Dateien
    String page = FPSTR(HTTP_HEAD);
    int i=0;
    page.replace("{v}", "List Images");
    page += FPSTR(HTTP_STYLE);
    page += FPSTR(HTTP_JS_IMAGE);
    page += FPSTR(HTTP_HEAD_END);
    page += F("<form action=\"/config\" method=\"get\">");
    page += F("<select name=\"image\" size=\"10\" onchange=\"setImage(this)\">");
    while (dir.next()) {
        if(!dir.fileName().endsWith(".bmp"))
          continue;
        //page += F("<option value=\"") + dir.fileName().substring(1) + "\">" + dir.fileName().substring(1) + F("</option>");
        page += F("<option value=\"");
        page += dir.fileName(); //with "/"
        page += String("\">") + dir.fileName().substring(1) ;
        page += F("</option>");
    }
    page += F("</select>");
    page += F("<button type=\"submit\">Select</button></form>");
    page += F("<br /><img id=\"PrevImg\" src=\"\" />");

    page += FPSTR(HTTP_END);

    server.send(200, "text/html", page);

}

void handleConfig(){
  String filename;    

  if(server.args() > 0){
    
    if(server.hasArg("no_LEDs"))
      configuration.no_of_leds = server.arg("no_LEDs").toInt();
    if(server.hasArg("LED_pin"))
      configuration.led_pin = server.arg("LED_pin").toInt();
    if(server.hasArg("line_time"))
      configuration.line_time = server.arg("line_time").toInt();  
    if(server.hasArg("trigger_pin"))
      configuration.led_pin = server.arg("LED_pin").toInt();
    if(server.hasArg("image")){
      filename = server.arg("image");
      if(!filename.startsWith("/")) filename = "/"+filename;
      filename.toCharArray(configuration.image_to_draw,sizeof(configuration.image_to_draw));  
    }
    if(server.hasArg("wifi_mode"))
      server.arg("wifi_mode").toCharArray(configuration.wifi_mode,sizeof(configuration.wifi_mode)); 
   
    if(server.hasArg("sta_ssid")){
      server.arg("sta_ssid").toCharArray(configuration.sta_ssid,sizeof(configuration.sta_ssid));  
    }
    if(server.hasArg("sta_pass")){
      server.arg("sta_pass").toCharArray(configuration.sta_pass,sizeof(configuration.sta_pass));  
    }
    if(server.hasArg("ap_ssid")){
      server.arg("ap_ssid").toCharArray(configuration.ap_ssid,sizeof(configuration.ap_ssid));  
    }
    if(server.hasArg("ap_pass")){
      server.arg("ap_pass").toCharArray(configuration.ap_pass,sizeof(configuration.ap_pass));  
    }
  }
    //if store arg is passed write to filesystem else it only temporary 
    if(server.hasArg("action")){
      if(server.arg("action").equals("store"))
        write_config();
      if(server.arg("action").equals("browse_file")){
        handleFileList();
        return;
      }
      if(server.arg("action").equals("browse_wifi")){
        handleBrowseWifi();
        return;
      }
    }
      
  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "Config");
  page += FPSTR(HTTP_STYLE);
  page += FPSTR(HTTP_HEAD_END);

  page += F("<h1>LED-Lightpainter Config</h1><br />");
  page += F("<form method=\"get\">");
  page += F("LEDs: <input type=\"text\" name=\"no_LEDs\" value=\"");
  page += configuration.no_of_leds;
  page += F("\" /><br />");
  page += F("LED Pin: <input type=\"text\" name=\"LED_pin\" value=\"");
  page += configuration.led_pin;
  page += F("\" /><br />");
  page += F("Line Time: <input type=\"text\" name=\"line_time\" value=\"");
  page += configuration.line_time;
  page += F("\" /><br />");
  page += F("Trigger Pin: <input type=\"text\" name=\"trigger_pin\" value=\"");
  page += configuration.trigger_pin;
  page += F("\" /><br />");
  page += F("Image: <input type=\"text\" name=\"image\" value=\"");
  page += configuration.image_to_draw;
  page += F("\" /><p />");
  page += F("<button type=\"submit\" name=\"action\" value=\"browse_file\">Browse</button><p />");

  page += F("<h2>WiFi</h2><br />");

  page += F("<input class=\"radio\" type=\"radio\" name=\"wifi_mode\" value=\"sta\"");
  if(!strcmp(configuration.wifi_mode,"sta")) page += F(" checked");
  page += F("/>Station<br />");
  page += F("SSID: <input type=\"text\" name=\"sta_ssid\" value=\"");
  page += configuration.sta_ssid;
  page += F("\" /><br />");
  page += F("Password: <input type=\"text\" name=\"sta_pass\" value=\"");
  page += configuration.sta_pass;
  page += F("\" /><p />");

  page += F("<button type=\"submit\" name=\"action\" value=\"browse_wifi\">Browse WiFi</button><p />");
  page += F("<input class=\"radio\" type=\"radio\" name=\"wifi_mode\" value=\"client\"");
  if(!strcmp(configuration.wifi_mode,"ap")) page += F(" checked");
  page += F(" />AP<br />");
  page += F("SSID: <input type=\"text\" name=\"ap_ssid\" value=\"");
  page += configuration.ap_ssid;
  page += F("\" /><br />");
  page += F("Password: <input type=\"text\" name=\"sta_pass\" value=\"");
  page += configuration.ap_pass;
  page += F("\" /><p />");
  page += F("<button type=\"submit\" name=\"action\" value=\"set\">Set Temporary</button><p />");
  page += F("<button type=\"submit\" name=\"action\" value=\"store\">Store</button>");
  page += F("</form>");
  page += F("</div></body></html>");

  page += FPSTR(HTTP_END);

  server.send(200, "text/html", page);

}

void handleBrowseWifi(){
  int numberOfNetworks = WiFi.scanNetworks();

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "Browse Wifi");
  page += FPSTR(HTTP_STYLE);
  page += FPSTR(HTTP_HEAD_END);

  page += F("<form action=\"/config\" method=\"get\">");
  page += F("<select name=\"sta_ssid\" size=\"10\">");
  for(int i =0; i<numberOfNetworks; i++){
    page += F("<option value=\"");
    page += WiFi.SSID(i);
    page += String("\">") + WiFi.SSID(i) ;
    page += F("</option>");

  }
  page += F("</select>");
  page += F("<button type=\"submit\">Select</button></form>");
  page += FPSTR(HTTP_END);

  server.send(200, "text/html", page);

}

int write_config(){
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  File file = SPIFFS.open(config_filename, "w");

  root["no_LEDs"] = configuration.no_of_leds;
  root["LED_pin"] = configuration.led_pin;
  root["line_time"] = configuration.line_time;
  root["trigger_pin"] = configuration.trigger_pin;
  root["image"] = configuration.image_to_draw;

  root["wifi_mode"] = configuration.wifi_mode;
  
  root["sta_ssid"] = configuration.sta_ssid;
  root["sta_pass"] = configuration.sta_pass;
  root["ap_ssid"] = configuration.ap_ssid;
  root["ap_pass"] = configuration.ap_pass;

  if (root.printTo(file) == 0) {
    Serial.println(F("Failed to write to file"));
    file.close();
    return -1;
  }
  file.close();
  return 0;
}

int load_config(){
    DynamicJsonBuffer jsonBuffer;
    File file = SPIFFS.open(config_filename, "r");
    JsonObject &root = jsonBuffer.parseObject(file);;
        
    configuration.no_of_leds = root["no_LEDs"];
    configuration.led_pin = root["LED_pin"];
    configuration.line_time = root["line_time"];
    configuration.trigger_pin = root["trigger_pin"];
    strncpy(configuration.image_to_draw, root["image"], sizeof(configuration.image_to_draw));
    strncpy(configuration.wifi_mode,root["wifi_mode"],sizeof(configuration.wifi_mode));
    
    strncpy(configuration.sta_ssid, root["sta_ssid"], sizeof(configuration.sta_ssid));
    strncpy(configuration.sta_pass, root["sta_pass"], sizeof(configuration.sta_pass));
    strncpy(configuration.ap_ssid, root["ap_ssid"], sizeof(configuration.ap_ssid));
    strncpy(configuration.ap_pass, root["ap_pass"], sizeof(configuration.ap_pass));
    file.close();
    return 0;
}

uint16_t read16(File& f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File& f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

void drawBMP(char *filename) {
  File     bmpFile;
  int16_t  bmpWidth, bmpHeight;   // Image W+H in pixels
  //uint8_t  bmpDepth;            // Bit depth (must be 24) but we dont use this
  uint32_t bmpImageoffset;        // Start address of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  //uint8_t  sdbuffer[3 * BUFF_SIZE];    // SD read pixel buffer (8 bits each R+G+B per pixel)
  uint8_t * sdbuffer = (uint8_t *)malloc(configuration.no_of_leds *3);
  boolean  goodBmp = false;            // Flag set to true on valid header parse
  int16_t  w, h, row, col,i;             // to store width, height, row and column
  //uint8_t  r, g, b;   // brg encoding line concatenated for speed so not used
  uint8_t rotation;     // to restore rotation
  uint8_t  tft_ptr = 0;  // buffer pointer

  

  SPIFFS.begin();
  // Check file exists and open it
  if ((bmpFile = SPIFFS.open(filename, "r")) == NULL) {
    Serial.println(F("File not found")); // Can comment out if not needed
    free(sdbuffer);
    return;
  }
  pinMode(configuration.led_pin, OUTPUT);

  Adafruit_NeoPixel pixels = Adafruit_NeoPixel(configuration.no_of_leds, configuration.led_pin, NEO_GRB + NEO_KHZ800);
  

  // Parse BMP header to get the information we need
  if (read16(bmpFile) == 0x4D42) { // BMP file start signature check
    read32(bmpFile);       // Dummy read to throw away and move on
    read32(bmpFile);       // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    read32(bmpFile);       // Dummy read to throw away and move on
    bmpWidth  = read32(bmpFile);  // Image width
    bmpHeight = read32(bmpFile);  // Image height

    //if (read16(bmpFile) == 1) { // Number of image planes -- must be '1'
    // Only proceed if we pass a bitmap file check
    if ((read16(bmpFile) == 1) && (read16(bmpFile) == 24) && (read32(bmpFile) == 0)) { // Must be depth 24 and 0 (uncompressed format)
      //goodBmp = true; // Supported BMP format -- proceed!
      // BMP rows are padded (if needed) to 4-byte boundary
      rowSize = (bmpWidth * 3 + 3) & ~3;
      // Crop area to be loaded
      w = bmpWidth;
      if(w > configuration.no_of_leds){
        Serial.println(F("Error Image is bigger than LED no"));
        pinMode(configuration.led_pin, INPUT);
        free(sdbuffer);
        bmpFile.close();
        return;
      }
      h = bmpHeight;
      Serial.println(w);
      Serial.println(h);
      
      for (uint32_t pos = bmpImageoffset; pos < bmpImageoffset + h * rowSize ; pos += rowSize) {
        if (bmpFile.position() != pos) {
          bmpFile.seek(pos,SeekSet);
          bmpFile.read(sdbuffer, configuration.no_of_leds *3);
        }
        
        uint16_t buffer_pos=0;
          for(i = 0; i < w; i++) {
            
            pixels.setPixelColor(i, gamma8[sdbuffer[buffer_pos+2]], gamma8[sdbuffer[buffer_pos+1]], gamma8[sdbuffer[buffer_pos]]);
            
            buffer_pos +=3;
          }
          
           pixels.show();
          
          delay(configuration.line_time);
        }
      }
      
    }
  
  bmpFile.close();
  
  //Clear pixels
  pixels.clear();
  pixels.show();          

  //switch pin back to input
  pinMode(configuration.led_pin, INPUT);
  free(sdbuffer);
  return;
 }