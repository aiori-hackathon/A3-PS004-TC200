# PS-004 — Testing and Validation

## 1. Purpose

This document records the test methodology, observed results, and validation status of the Gateway-Assisted DNSSEC prototype for ESP8266 IoT devices.

The system combines an ESP8266 NodeMCU client, a local gateway running Unbound, NSD-hosted DNS test zones, DNSSEC validation, and a planned DNS-over-TLS (DoT) communication path.

## 2. Test Environment

| Component | Configuration |
|---|---|
| IoT device | ESP8266 / NodeMCU |
| Firmware framework | Arduino |
| Build environment | Visual Studio Code and PlatformIO |
| Gateway environment | Ubuntu on WSL2 |
| Recursive resolver | Unbound |
| Authoritative DNS server | NSD |
| DNS transport target | TCP port `853` |
| Client TLS library | BearSSL / `WiFiClientSecure` |

## 3. Test Cases

### TC-01: ESP8266 Wi-Fi Connectivity

**Objective:** Verify that the ESP8266 connects to the configured wireless network.

**Procedure:**
1. Build and upload the firmware.
2. Open the serial monitor at `115200` baud.
3. Observe the Wi-Fi connection status and assigned IP address.

**Expected result:** The ESP8266 connects successfully and reports its IP address.

**Observed result:** Wi-Fi connectivity was successful in the current test environment.

**Status:** PASS

### TC-02: Unbound Configuration Validation

**Objective:** Check the syntax and validity of the Unbound configuration.

**Procedure:**

```bash
sudo unbound-checkconf
```

**Expected result:** Unbound reports that the configuration is valid.

**Observed result:** The configuration passed `unbound-checkconf`.

**Status:** PASS

### TC-03: DNSSEC Test-Zone Preparation

**Objective:** Prepare controlled DNS data for signed, unsigned, and DNSSEC-bogus scenarios.

**Test scenarios:**
- Valid signed DNS data
- Unsigned DNS data
- DNSSEC-bogus DNS data

**Procedure:**
1. Configure the test zones in NSD.
2. Query the relevant records using `dig`.
3. Save the responses and DNSSEC-related records.
4. Test validation behavior through the configured validating resolver.

**Observed result:** The test zones were prepared and DNS responses were obtained. Complete end-to-end validation results must be recorded separately.

**Status:** TEST INFRASTRUCTURE PREPARED

### TC-04: Gateway TLS Verification from WSL

**Objective:** Verify that a separate client can establish a TLS connection to the gateway while trusting the configured CA and checking the gateway hostname.

**Procedure:** Connect to the gateway's DoT endpoint using OpenSSL, with certificate-chain and hostname verification enabled.

**Observed result:** An independent WSL-based TLS test succeeded with CA trust and hostname verification.

**Status:** PASS

### TC-05: ESP8266 Gateway Hostname Resolution

**Objective:** Verify that the ESP8266 can resolve the configured gateway hostname.

**Procedure:**
1. Connect the ESP8266 to Wi-Fi.
2. Attempt to resolve `gateway.lab`.
3. Inspect the serial output.

**Observed result:** The current firmware test reported that `gateway.lab` could not be resolved successfully.

**Status:** IN PROGRESS

### TC-06: ESP8266 DNS-over-TLS Connection

**Objective:** Establish a TLS-authenticated connection from the ESP8266 to the gateway on TCP port `853`.

**Procedure:**
1. Confirm gateway hostname resolution and network reachability.
2. Confirm that the gateway certificate matches the configured hostname.
3. Configure the ESP8266 to trust the correct CA certificate.
4. Attempt the TLS connection.
5. Record the connection result and any BearSSL error details.

**Observed result:** Successful ESP8266-side certificate-verified TLS communication has not yet been demonstrated.

**Status:** IN PROGRESS

### TC-07: DNS Query over TLS

**Objective:** Send a DNS query through the established TLS connection and receive the corresponding DNS response.

**Procedure:**
1. Establish the verified TLS connection.
2. Encode the DNS query in wire format.
3. Prepend the two-byte message length required for DNS-over-TCP/DoT.
4. Send the query through the TLS connection.
5. Read and validate the response.
6. Compare the response with the resolver logs.

**Observed result:** DNS query transmission and response handling over the ESP8266 TLS connection remain to be implemented and verified.

**Status:** PENDING

## 4. DNSSEC Evaluation Matrix

| Test case | Expected behavior | Evidence to collect |
|---|---|---|
| Valid signed data | Resolution succeeds when the DNSSEC chain validates | DNS response and resolver validation evidence |
| Unsigned data | Resolution behavior depends on zone and resolver policy; data is not DNSSEC-authenticated | DNS response and validation status |
| DNSSEC-bogus data | Validation fails, typically with `SERVFAIL` | Resolver response and validation logs |

The expected behavior must be confirmed through actual queries to the configured validating resolver. Authoritative DNS responses alone do not demonstrate that DNSSEC validation succeeded or failed.

## 5. Evidence Collection

For each test run, record:

- Test case ID and date
- Query name and record type
- Test environment and network configuration
- Expected result
- Actual result
- ESP8266 serial output
- Relevant Unbound or NSD logs
- Final status: PASS, FAIL, or IN PROGRESS

Do not include Wi-Fi passwords, private keys, tokens, or other secrets in the evidence.

## 6. Current Summary

The ESP8266 Wi-Fi connection, Unbound configuration check, DNS test-zone preparation, and independent WSL TLS verification have been completed successfully.

The remaining integration work is to resolve the gateway hostname from the ESP8266, establish verified device-to-gateway DoT communication, and execute the DNSSEC scenarios through the complete system.

## 7. Next Steps

- Resolve gateway hostname and routing issues.
- Demonstrate certificate-verified DoT from the ESP8266.
- Implement DNS message transmission and response handling.
- Verify signed, unsigned, and DNSSEC-bogus behavior through Unbound.
- Collect repeatable logs and performance measurements.

---

*Update this document whenever a test produces new, reproducible evidence. Mark a test as PASS only when the observed result supports the expected behavior.*