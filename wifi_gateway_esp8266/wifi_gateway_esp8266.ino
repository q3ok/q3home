/**
 * SPI <-> MQTT gateway for ESP8266
 * part of q3home system
 *
 * Copyright (C) 2015-2017 q3ok
 * 
 * Configuration in configuration.h should be adjusted to your needs
 * 
 * Protocol:
 * From serial (at 115200)
 * Q3|[DEVICE_ADDR][SENSOR_NAME]|[VALUE]
 * If used with q3i2c, then:
 * DEVICE_ADDR - numbers eg. two digits (10, 11, ...)
 * SENSOR_NAME - string eg. four letters (MOVE, LIGH, TEMP, ...)
 * VALUE - numbers eg. numeric value, separated with comma
 * eg.
 * Q3|10TEMP|23,45
 *
 */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <FS.h>
#include <PubSubClient.h>
#include "configuration.h"
#include "HtmlGenerator.h"
#include "ConfigStore.h"

const String dir_sensors = "/sensors/";

ConfigStore conf("/configuration.db");

ESP8266WebServer server ( 80 );

WiFiClient mqttClientH;
PubSubClient mqttClient(mqttClientH);

void checkWiFiConnection() {
  if ( WiFi.status() == WL_CONNECTED ) return;
  saveLog("Lost signal, reconnecting WiFi...");
  delay(1000);
  WiFi.begin( ssid, password);
  int waitForConnection = 0;
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    if ( ++waitForConnection > 5 ) {
      checkWiFiConnection();
      break;
    }
  }
  saveLog("WiFi connected.");
}

/* taken from https://github.com/zenmanenergy/ESP8266-Arduino-Examples/blob/master/helloWorld_urlencoded/urlencode.ino */
String urlencode(String str) {
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
    
}

void postDataToUrl(String sensorName, String sensorValue) {
  String query = conf.get("sendurl");
  query.concat("?name=");
  query.concat(urlencode(sensorName));
  query.concat("&value=");
  query.concat(urlencode(sensorValue));
  
  HTTPClient http;
  http.begin(query);
  http.GET();
  http.end();
  
}

void postDataToMQTT(String sensorName, String sensorValue) {
  String devaddr = sensorName.substring(0, sensorName.length() - 4);
  String sensname = sensorName.substring(devaddr.length());

  String devname = conf.get("dev" + devaddr);
  String sensorname = conf.get("sensor" + sensname);

  if (devname.length() < 1 || sensorname.length() < 1) {
    saveLog("Device " + devaddr + " or sensor " + sensname + " are not registered yet!");
    return; /* device or sensor not registered yet */
  }
  
  String topic = conf.get("mqttbasetopic") + devname + "/" + sensorname;
  mqttClient.publish( topic.c_str(), sensorValue.c_str() );
}

bool isSensorNameOk(String sensorName) {
  for (int i=0;i<sensorName.length();i++) {
    if ( sensorName[i]<48 || sensorName[i]>90 || (sensorName[i]>57 && sensorName[i]<65) ) {
      return false;
    }
  }
  return true;
}

void parseInputData(String input) {
  String t1, t2, t3;
  int t1d, t2d;
  String delimiter = "|";
  t1d = input.indexOf(delimiter);
  if ( t1d == -1 ) return;
  t1 = input.substring(0, t1d);
  if ( t1 != "Q3" ) return;
  t2d = input.indexOf(delimiter, t1d + 1);
  t2 = input.substring(t1d + 1, t2d);
  t3 = input.substring(t2d + 1, input.length());

  if (!isSensorNameOk(t2)) return;

  String fname = dir_sensors;
  fname.concat(t2);
  fname.concat(".txt");
  File f = SPIFFS.open(fname, "w");
  f.println(t3);
  f.close();

  if (conf.get("mqtton").toInt() == 1) {
    postDataToMQTT(t2, t3);
  }

  if (conf.get("sendon").toInt() == 1) {
    postDataToUrl(t2, t3);
  }
  
}

void clearLog() {
  SPIFFS.remove("/log.txt");
}

void saveLog(String txt) {
  File logfile = SPIFFS.open("/log.txt", "a");
  logfile.println(txt);
  logfile.close();
}

void handleDataRequest() {
  String r = server.arg("r");
  String fname = dir_sensors;
  fname.concat(r);
  fname.concat(".txt");
  if (!SPIFFS.exists(fname)) {
    return handleNotFound();
  }
  File f = SPIFFS.open(fname, "r");
  String data = f.readStringUntil('\n');
  data.replace('\r',' ');
  data.trim();
  f.close();
  server.send( 200, "text/plain", data );
}

void handlePureRequest() {
  String fname = server.arg("r");
  if (!SPIFFS.exists(fname)) {
    return handleNotFound();
  }
  File f = SPIFFS.open(fname, "r");
  String data = "";
  String temp;
  while (f.available()) {
    temp = f.readStringUntil('\n');
    temp.replace('\r',' ');
    temp.trim();
    data += temp;
    data += '\n';
  }
  f.close();
  server.send( 200, "text/plain", data );
}

void handleFormatRequest() {
  String resp;
  String pass = server.arg("pass");
  if (pass != format_password) {
    saveLog("Incorrect password for reset request");
    resp = "Password incorrect. Your attempt has been logged.";
    server.send( 200, "text/plain", resp );
    return;
  }

  if (SPIFFS.format()) {
    resp = "Factory reset OK";
  } else {
    resp = "Factory reset FAILED";
  }
  server.send(200, "text/plain", resp);

}

void handleGetAllSensorsData() {
  Dir dirH = SPIFFS.openDir(dir_sensors);
  String resp = "";
  String temp;
  while (dirH.next()) {
    resp += dirH.fileName().substring(0,dirH.fileName().indexOf(".txt"));
    resp += "|";
    File f = dirH.openFile("r");
    temp = f.readStringUntil('\n');
    temp.replace('\r',' ');
    temp.trim();
    resp += temp;
    resp += '\n';
    f.close();
  }
  server.send(200, "text/plain", resp);
}

void handleBrowse() {
  String dir = server.arg("dir");
  if (dir.length() < 1) {
    server.send(404, "text/plain", "404: Not Found");
  }
  String data = HtmlGenerator::h(1, "Files in " + String(dir));
  data += "<ul>";
  Dir dirH = SPIFFS.openDir(dir);
  while (dirH.next()) {
    data.concat("<li><a href=\"/pure?r=");
    data.concat(dirH.fileName());
    data.concat("\" target=\"_blank\">");
    data.concat(dirH.fileName());
    data.concat("</a></li>");
  }
  server.send(200, "text/html", data);

}

void handleNotFound() {
  server.send( 404, "text/plain", "Not Found" );
}

void handleConfig() {
  String resp;
  
  int deviceCount = server.arg("deviceCount").toInt();
  int sensorCount = server.arg("sensorCount").toInt();
  
  if ( server.arg("newDeviceCount").toInt() != 0 ) {
    deviceCount = server.arg("newDeviceCount").toInt();
  }
  
  if ( server.arg("newSensorCount").toInt() != 0 ) {
    sensorCount = server.arg("newSensorCount").toInt();
  }

  /* first load of this page, all the data should be loaded from memory (if exists) */
  if ( server.args() == 0 ) {
    deviceCount = conf.get("deviceCount").toInt();
    sensorCount = conf.get("sensorCount").toInt();
  }

  if (deviceCount < 1) deviceCount = 1;
  if (sensorCount < 1) sensorCount = 1;
  
  int devaddr[deviceCount];
  String devname[deviceCount];
  for (int i=0;i<deviceCount;i++) {
    if ( server.args() == 0 ) { /* more nice clear code VS less comparisions - 1:0 */
      devaddr[i] = conf.get("devaddr" + String(i)).toInt();
      devname[i] = conf.get("devname" + String(i));
    } else {
      devaddr[i] = server.arg("devaddr" + String(i)).toInt();
      devname[i] = server.arg("devname" + String(i)); // hint: sensor_name.toCharArray(pck.sensorName, 5);
    }
  }
  
  String sensorname[sensorCount];
  String sensorviewname[sensorCount];
  for (int i=0;i<sensorCount;i++) {
    if ( server.args() == 0 ) { /* more nice clear code VS less comparisions - 1:0 */
      sensorname[i] = conf.get("sensorname" + String(i));
      sensorviewname[i] = conf.get("sensorviewname" + String(i));
    } else {
  	  sensorname[i] = server.arg("sensorname" + String(i));
  	  sensorviewname[i] = server.arg("sensorviewname" + String(i));
    }
  }

  if ( server.arg("removeDevice").toInt() != 0 ) {
    deviceCount--;
    for (int i=server.arg("removeDevice").toInt();i<deviceCount;i++) {
      devaddr[i] = devaddr[i+1];
      devname[i] = devname[i+1];
    }
  }
  
  if ( server.arg("removeSensor").toInt() != 0 ) {
	  sensorCount--;
	  for (int i=server.arg("removeSensor").toInt();i<sensorCount;i++) {
		  sensorname[i] = sensorname[i+1];
		  sensorviewname[i] = sensorviewname[i+1];
	  }
  }

  bool mqtton = false;
  if (server.arg("mqtton").toInt() == 1) mqtton = true;
  String mqttserver = server.arg("mqttserver");
  String mqttport = server.arg("mqttport");
  String mqttbasetopic = server.arg("mqttbasetopic");

  bool sendon = false;
  if (server.arg("sendon").toInt() == 1) sendon = true;
  String sendurl = server.arg("sendurl");

  if ( server.args() == 0 ) {
    if (conf.get("mqtton").toInt() == 1) mqtton = true;
    mqttserver = conf.get("mqttserver");
    mqttport = conf.get("mqttport");
    mqttbasetopic = conf.get("mqttbasetopic");
    if (conf.get("sendon").toInt() == 1) sendon = true;
    sendurl = conf.get("sendurl");
  }

  if (server.arg("submit").length() > 0) {
    /* lets save the configuration !*/
    conf.autosave(AUTOSAVE_OFF);
    if (server.arg("mqtton").toInt() == 1) {
      conf.set("mqtton", "1");
      conf.set("mqttserver", mqttserver);
      conf.set("mqttport", mqttport);
      conf.set("mqttbasetopic", mqttbasetopic);
    } else {
      conf.set("mqtton", "0");
    }
    if (server.arg("sendon").toInt() == 1) {
      conf.set("sendon", "1");
      conf.set("sendurl", sendurl);
    } else {
      conf.set("sendon", "0");
    }
    conf.set("deviceCount", String(deviceCount));
    conf.set("sensorCount", String(sensorCount));
    for (int i=0;i<deviceCount;i++) {
      conf.set("devname" + String(i), devname[i]); /* to be able to modify config later */
      conf.set("devaddr" + String(i), String(devaddr[i])); /* to be able to modify config later */
      conf.set("dev" + String(devaddr[i]), devname[i]); /* to easily read while sending to mqtt */
    }
    for (int i=0;i<sensorCount;i++) {
      conf.set("sensorname" + String(i), sensorname[i]);
      conf.set("sensorviewname" + String(i), sensorviewname[i]);
      conf.set("sensor" + sensorname[i], sensorviewname[i]); /* as with devices, to easily read */
    }
    conf.autosave(AUTOSAVE_ON);
  }
  
  resp += HtmlGenerator::header("Configuration");
  resp += HtmlGenerator::h(1, "Gateway configuration");
  //resp += HtmlGenerator::p("Args: " + String(server.args()) );
  resp += HtmlGenerator::formStart("post", "/config");
  resp += HtmlGenerator::divStart("width: 90%");

  /* mqtt config */
  resp += HtmlGenerator::h(2, "MQTT");
  resp += HtmlGenerator::checkbox("MQTT Enabled", "mqtton", mqtton);
  resp += HtmlGenerator::textinput("MQTT Server", "mqttserver", mqttserver);
  resp += HtmlGenerator::textinput("Port", "mqttport", mqttport);
  resp += HtmlGenerator::textinput("Base topic (eg. Home/)", "mqttbasetopic", mqttbasetopic);

  /* send data somewhere config */
  resp += HtmlGenerator::h(2, "Send sensor data to url");
  resp += HtmlGenerator::checkbox("Enabled", "sendon", sendon);
  resp += HtmlGenerator::textinput("URL", "sendurl", sendurl);
  
  /* devices section */
  resp += HtmlGenerator::hidden("deviceCount", String(deviceCount) );
  resp += HtmlGenerator::h(2, "Devices");
  resp += HtmlGenerator::button("(+) Add device", "newDeviceCount", "submit", String( deviceCount +1 ) );
  resp += HtmlGenerator::divStart("clear:both; display:block;") + HtmlGenerator::divEnd();
  for (int i=0; i < deviceCount; i++) {
    resp += HtmlGenerator::divStart("display: inline-block; width: 300px; border: 1px solid;");
	  resp += HtmlGenerator::h(4, "Device object");
    resp += HtmlGenerator::textinput("Device address (dec)", "devaddr" + String(i), String(devaddr[i]) );
	  resp += HtmlGenerator::textinput("Device name", "devname" + String(i), devname[i] );
    if (deviceCount > 1) resp += HtmlGenerator::button("(-) Remove", "removeDevice", "submit", String(i) );
    resp += HtmlGenerator::divEnd();
  }
  resp += HtmlGenerator::divStart("clear:both; display:block;") + HtmlGenerator::divEnd();
  /* end of devices section */
  
  /* sensors secton */
  resp += HtmlGenerator::hidden("sensorCount", String(sensorCount) );
  resp += HtmlGenerator::h(2, "Sensors");
  resp += HtmlGenerator::button("(+) Add sensor", "newSensorCount", "submit", String( sensorCount +1 ) );
  resp += HtmlGenerator::divStart("clear:both; display:block;") + HtmlGenerator::divEnd();
  for (int i=0; i < sensorCount; i++) {
  	resp += HtmlGenerator::divStart("display: inline-block; width: 300px; border: 1px solid;");
  	resp += HtmlGenerator::h(4, "Sensor type object");
  	resp += HtmlGenerator::textinput("Device sensor name (eg. TEMP/HUMI/LIGH/MOVE/SNDS/...)", "sensorname" + String(i), sensorname[i] );
  	resp += HtmlGenerator::textinput("User-friendly name (eg. Temperature)", "sensorviewname" + String(i), sensorviewname[i] );
  	if (sensorCount > 1) resp += HtmlGenerator::button("(-) Remove", "removeSensor", "submit", String(i) );
  	resp += HtmlGenerator::divEnd();
  }
  resp += HtmlGenerator::divStart("clear:both; display:block;") + HtmlGenerator::divEnd();
  /* end of sensors section */
  
  resp += HtmlGenerator::submit("Save");
  resp += HtmlGenerator::divEnd();
  resp += HtmlGenerator::formEnd();
  resp += HtmlGenerator::footer();
  
  server.send (200, "text/html", resp);
}

void handleRoot() {
  String resp = HtmlGenerator::header("SPI-MQTT Gateway");

  resp += "<script type=\"text/javascript\">function requestReset() { var pass = prompt(\"Please enter password\"); document.location.href = \"/format?pass=\" + pass; }</script>";
  
  resp += HtmlGenerator::h(1, "SPI <-> MQTT Gateway");
  resp += HtmlGenerator::divStart();
  resp += HtmlGenerator::listStart();
  resp += HtmlGenerator::listItem( HtmlGenerator::a("Configuration", "/config") );
  resp += HtmlGenerator::listItem( HtmlGenerator::a("Browse", "/browse?dir=/") );
  resp += HtmlGenerator::listItem( HtmlGenerator::a("All sensors", "/alldata") );
  resp += HtmlGenerator::listItem( HtmlGenerator::a("Factory reset", "javascript:requestReset()") );
  resp += HtmlGenerator::listEnd();
  resp += HtmlGenerator::divEnd();
  resp += HtmlGenerator::footer();

  server.send ( 200, "text/html", resp);

}

void mqttReconnect() {
  if (!mqttClient.connected()) {
    IPAddress mqtt_server;
    mqtt_server.fromString(conf.get("mqttserver"));
    int mqtt_port = conf.get("mqttport").toInt();
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.connect( String(ESP.getChipId()).c_str() );
    delay(1000);
  }
}

void setup() {
  for (;;) {
    /* try to init SPIFFS, if not, then format */
    if ( !SPIFFS.begin() ) {
      SPIFFS.format();
    } else {
      break;
    }
  }
  clearLog();
  Serial.begin(115200);

  saveLog("Connecting to AP");

  WiFi.mode(WIFI_STA);
  WiFi.begin( ssid, password );
  WiFi.config( static_ip, gateway, mask);
  while ( WiFi.status() != WL_CONNECTED ) {
    yield();
    delay(500);
  }
  saveLog("WiFi Connected");

  server.on( "/", handleRoot );
  server.on( "/data", handleDataRequest );
  server.on( "/alldata", handleGetAllSensorsData );
  server.on( "/pure", handlePureRequest );
  server.on( "/browse", handleBrowse );
  server.on( "/format", handleFormatRequest );
  server.on( "/config", handleConfig );
  
  server.onNotFound ( handleNotFound );
  server.begin();

  saveLog("Server started");

  if (conf.get("mqtton").toInt() == 1) {
    mqttReconnect();
    saveLog("MQTT Connected");
  }
  
  saveLog("Gateway READY");
}

void loop() {
  checkWiFiConnection();
  server.handleClient();
  mqttClient.loop();
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    data.replace('\r',' ');
    data.trim();
    parseInputData( data );
  }
}


