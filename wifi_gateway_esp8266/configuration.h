#ifndef configuration_h
#define configuration_h

/* gateway IP configuration (static) */
IPAddress static_ip = {192,168,1,2};
IPAddress gateway = {192,168,1,1};
IPAddress mask = {255,255,255,0};

/* WiFi connection details */
const char *ssid = "SSID";
const char *password = "PASSWORD";

/* password for Factory reset */
const String format_password = "FACTORYRESETPASSWORD";

#endif
