
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <time.h>

#include "secrets.h"
#include "ca_cert.h"

const char* GATEWAY_HOST = "gateway.lab";
const uint16_t GATEWAY_PORT = 853;

IPAddress gatewayIP(192, 168, 1, 5);

BearSSL::WiFiClientSecure tlsClient;

// DNS query for gateway.lab, type A, class IN.
// Header: ID 0x1234, recursion desired.
const uint8_t dnsQuery[] = {
  0x12, 0x34,  // Transaction ID
  0x01, 0x00,  // Flags
  0x00, 0x01,  // One question
  0x00, 0x00,  // No answers
  0x00, 0x00,  // No authority records
  0x00, 0x00,  // No additional records
  0x07, 'g','a','t','e','w','a','y',
  0x03, 'l','a','b',
  0x00,        // Root label terminator
  0x00, 0x01,  // A record
  0x00, 0x01   // IN class
};

bool readExact(uint8_t* buffer, size_t length, uint32_t timeoutMs = 5000) {
  size_t received = 0;
  uint32_t started = millis();

  while (received < length && millis() - started < timeoutMs) {
    while (tlsClient.available() && received < length) {
      int value = tlsClient.read();
      if (value >= 0) {
        buffer[received++] = static_cast<uint8_t>(value);
      }
    }
    delay(1);
  }

  return received == length;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("PS-004 ESP8266 DNS-over-TLS Test");

  // 1. Connect to Wi-Fi.
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

  // 2. Synchronise the clock for certificate validity checks.
  Serial.println("\nSynchronising clock with NTP...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  time_t now = time(nullptr);
  attempts = 0;

  while (now < 1790000000 && attempts < 30) {
    delay(1000);
    now = time(nullptr);
    attempts++;
  }

  if (now < 1790000000) {
    Serial.println("NTP FAILED. Cannot reliably check certificate dates.");
    return;
  }

  tlsClient.setX509Time(now);
  Serial.print("Clock synchronised: ");
  Serial.println(ctime(&now));

  // 3. Configure the trusted CA.
  BearSSL::X509List trustAnchor(CA_CERT);
  tlsClient.setTrustAnchors(&trustAnchor);
  tlsClient.setTimeout(10);

  // 4. Connect to the configured gateway IP.
  // CA validation is enabled, but this IP-based connection does not
  // independently establish that the certificate hostname is gateway.lab.
  Serial.println("Connecting to DNS-over-TLS gateway...");

  if (!tlsClient.connect(gatewayIP, GATEWAY_PORT)) {
    char errorBuffer[160] = {0};
    int errorCode = tlsClient.getLastSSLError(
      errorBuffer, sizeof(errorBuffer)
    );

    Serial.println("TLS CONNECTION FAILED");
    Serial.print("TLS error code: ");
    Serial.println(errorCode);
    Serial.print("TLS error details: ");
    Serial.println(errorBuffer);
    tlsClient.stop();
    return;
  }

  Serial.println("TLS connection established.");

  // 5. Send DNS-over-TLS framing: two-byte network-order message length.
  const uint16_t queryLength = sizeof(dnsQuery);
  uint8_t lengthPrefix[2] = {
    static_cast<uint8_t>((queryLength >> 8) & 0xFF),
    static_cast<uint8_t>(queryLength & 0xFF)
  };

  if (tlsClient.write(lengthPrefix, sizeof(lengthPrefix)) != sizeof(lengthPrefix) ||
      tlsClient.write(dnsQuery, queryLength) != queryLength) {
    Serial.println("Failed to send DNS query.");
    tlsClient.stop();
    return;
  }

  Serial.println("DNS query sent over TLS.");

  // 6. Read the two-byte response length.
  uint8_t responsePrefix[2];

  if (!readExact(responsePrefix, sizeof(responsePrefix))) {
    Serial.println("Timed out reading DNS response length.");
    tlsClient.stop();
    return;
  }

  uint16_t responseLength =
      (static_cast<uint16_t>(responsePrefix[0]) << 8) |
      responsePrefix[1];

  if (responseLength < 12 || responseLength > 1500) {
    Serial.print("Invalid DNS response length: ");
    Serial.println(responseLength);
    tlsClient.stop();
    return;
  }

  // 7. Read the complete DNS response.
  uint8_t response[1500];

  if (!readExact(response, responseLength)) {
    Serial.println("Timed out reading DNS response.");
    tlsClient.stop();
    return;
  }

  tlsClient.stop();

  // 8. Check DNS transaction ID, response bit and response code.
  if (response[0] != 0x12 || response[1] != 0x34 ||
      !(response[2] & 0x80)) {
    Serial.println("Invalid DNS response header.");
    return;
  }

  uint8_t rcode = response[3] & 0x0F;

  if (rcode != 0) {
    Serial.print("DNS response error code: ");
    Serial.println(rcode);
    return;
  }

  Serial.println("DNS response: NOERROR");
  Serial.print("Response length: ");
  Serial.print(responseLength);
  Serial.println(" bytes");

  // 9. Locate an IPv4 A answer for gateway.lab.
  // This parser supports a compressed name pointer or the literal
  // gateway.lab owner name in the answer section.
  uint16_t questionCount =
      (static_cast<uint16_t>(response[4]) << 8) | response[5];
  uint16_t answerCount =
      (static_cast<uint16_t>(response[6]) << 8) | response[7];

  if (questionCount != 1) {
    Serial.println("Unexpected question count.");
    return;
  }

  size_t pos = 12;

  // Skip the question name.
  while (pos < responseLength && response[pos] != 0) {
    uint8_t labelLength = response[pos++];
    if ((labelLength & 0xC0) == 0xC0 ||
        pos + labelLength > responseLength) {
      Serial.println("Unexpected question-name encoding.");
      return;
    }
    pos += labelLength;
  }

  if (pos + 5 > responseLength) {
    Serial.println("Truncated DNS question.");
    return;
  }

  pos++;  // Name terminator
  pos += 4; // QTYPE and QCLASS

  for (uint16_t i = 0; i < answerCount; i++) {
    if (pos + 2 > responseLength) break;

    // Skip the owner name, including compression pointers.
    if ((response[pos] & 0xC0) == 0xC0) {
      pos += 2;
    } else {
      while (pos < responseLength && response[pos] != 0) {
        uint8_t labelLength = response[pos++];
        if (pos + labelLength > responseLength) {
          Serial.println("Malformed answer name.");
          return;
        }
        pos += labelLength;
      }
      if (pos >= responseLength) break;
      pos++;
    }

    if (pos + 10 > responseLength) break;

    uint16_t type =
        (static_cast<uint16_t>(response[pos]) << 8) | response[pos + 1];
    uint16_t klass =
        (static_cast<uint16_t>(response[pos + 2]) << 8) | response[pos + 3];
    uint16_t dataLength =
        (static_cast<uint16_t>(response[pos + 8]) << 8) | response[pos + 9];

    pos += 10;

    if (pos + dataLength > responseLength) {
      Serial.println("Truncated answer data.");
      return;
    }

    if (type == 1 && klass == 1 && dataLength == 4) {
      IPAddress answerIP(
        response[pos], response[pos + 1],
        response[pos + 2], response[pos + 3]
      );

      Serial.print("DNS A answer: ");
      Serial.println(answerIP);

      if (answerIP == gatewayIP) {
        Serial.println("DNS-over-TLS QUERY: SUCCESS");
      } else {
        Serial.println("DNS query succeeded, but returned a different IP.");
      }

      return;
    }

    pos += dataLength;
  }

  Serial.println("No IPv4 A answer found.");
}

void loop() {
}
