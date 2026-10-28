
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include "ca_cert.h"
#include <time.h>

const char* WIFI_SSID = "PIRATE";
const char* WIFI_PASSWORD = "JACKSPARROW1825";

const char* DOT_HOST = "gateway.lab";
const IPAddress DOT_IP(192, 168, 1, 5);
const uint16_t DOT_PORT = 853;

BearSSL::WiFiClientSecure dotClient;
BearSSL::X509List trustAnchor(CA_CERT);

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("DNSGuard ESP8266: secure DoT connection test");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");
    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED &&
           millis() - startTime < 20000) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("ERROR: Wi-Fi connection failed.");
        return;
    }

    Serial.print("ESP8266 IP: ");
    Serial.println(WiFi.localIP());

    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    Serial.print("Waiting for clock synchronization");
    time_t now = time(nullptr);
    unsigned long clockStart = millis();

    while (now < 1760000000 &&
           millis() - clockStart < 20000) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }

    Serial.println();

    if (now < 1760000000) {
        Serial.println("ERROR: Clock synchronization failed.");
        return;
    }

    Serial.print("Clock synchronized. Unix time: ");
    Serial.println((unsigned long)now);

    // Configure CA trust before attempting TLS.
    dotClient.setTrustAnchors(&trustAnchor);
    dotClient.setTimeout(10000);

    Serial.print("Connecting to ");
    Serial.print(DOT_HOST);
    Serial.print(" via ");
    Serial.print(DOT_IP);
    Serial.print(":");
    Serial.println(DOT_PORT);

    // Verify the server identity using the certificate hostname.
    // The certificate must contain DNS:gateway.lab in its SAN.
    if (!dotClient.connect(DOT_IP, DOT_PORT)) {
        Serial.println("ERROR: TLS connection or certificate verification failed.");

        char errorBuffer[160] = {0};
int errorCode = dotClient.getLastSSLError(
    errorBuffer, sizeof(errorBuffer)
);

Serial.print("TLS error code: ");
Serial.println(errorCode);
Serial.print("TLS error details: ");
Serial.println(errorBuffer);
        return;
    }

    Serial.println("SUCCESS: TLS connection and certificate verification succeeded.");
    dotClient.stop();
}

void loop() {
    delay(1000);
}
