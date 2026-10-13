
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include "secrets.h"
#include "ca_cert.h"

const char* GATEWAY_IP = "192.168.1.3";
const uint16_t GATEWAY_PORT = 853;
const char* GATEWAY_HOST = "gateway.lab";

BearSSL::WiFiClientSecure tlsClient;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("PS-004 Verified TLS Test");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWi-Fi connection failed.");
    return;
  }

  Serial.println("\nWi-Fi connected!");
  Serial.print("ESP IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("DNS server: ");
  Serial.println(WiFi.dnsIP());
  IPAddress resolvedIP;
  Serial.println("Checking gateway.lab DNS...");

  if (WiFi.hostByName(GATEWAY_HOST, resolvedIP)) {
    Serial.print("gateway.lab resolves to: ");
    Serial.println(resolvedIP);
  } else {
    Serial.println("DNS resolution failed for gateway.lab");
  }

  BearSSL::X509List trustAnchor(CA_CERT);
  tlsClient.setTrustAnchors(&trustAnchor);
  tlsClient.setTimeout(10);

  Serial.println("Connecting with TLS certificate verification...");
 
 if (tlsClient.connect(GATEWAY_HOST, GATEWAY_PORT)) {
    Serial.println("TLS connection: SUCCESS");
    Serial.println("Gateway certificate verification: PASSED");
    tlsClient.stop();
  
  } else {
    char errorBuffer[160] = {0};

    Serial.println("TLS connection or certificate verification: FAILED");

    int errorCode = tlsClient.getLastSSLError(
      errorBuffer,
      sizeof(errorBuffer)
    );

    Serial.print("TLS error code: ");
    Serial.println(errorCode);

    Serial.print("TLS error details: ");
    Serial.println(errorBuffer);

    tlsClient.stop();
  }

}

void loop() {
}
