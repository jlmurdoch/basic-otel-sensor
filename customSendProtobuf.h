#include <WiFi.h>
#include <WiFiClientSecure.h>

#define SSL 1 

void rejoinWifi();

int sendProtobuf(char *host, int port, char *uri, char *apikey, uint8_t *buf, size_t bufsize);