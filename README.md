# Air-Gapped ESP32 Hardware Authenticator

## Overview
This project elevates a standard ESP32 development module into a secure, air-gapped Two-Factor Authentication (2FA) hardware token. By utilizing a physical rotary encoder for PIN entry and a TFT display for status rendering, the device securely manages secrets, dynamically computes Time-Based One-Time Passwords (TOTP), and automatically injects them into the host machine via BLE HID (Bluetooth Keyboard Emulation). The radio interface strictly enforces an air-gapped constraint, remaining completely unpowered until physical hardware verification succeeds.

## Enterprise-Grade Security Features
The firmware architecture is hardened against both logical and physical attack vectors, deploying industry-standard mitigation strategies.

### Constant-Time PIN Verification
To eliminate timing side-channel attacks, PIN verification logic guarantees an O(1) constant-time execution profile, ensuring that computation time remains invariant regardless of the digit accuracy or position within the provided sequence.

### Anti-Glitching Protections
To thwart Voltage Fault Injection (VFI) and clock-glitching attempts, critical control flow branches evaluate against `volatile` variables and utilize redundant logic constraints. The Finite State Machine (FSM) relies on high-Hamming-distance bitwise-inverted Magic Words (e.g., `UNLOCKED` and `UNLOCKED_INV`), guaranteeing that memory corruption or skipped instructions during branching cannot successfully evaluate to an authenticated state.

### Secure NVS Wipe (Data Remanence Protection)
Brute-force mitigation is strictly enforced. After 3 consecutive failed PIN attempts, the authentication loop triggers a secure memory wipe. To defeat Non-Volatile Storage (NVS) data remanence across flash memory sectors, the active cryptographic key material is forcibly overwritten with `0x00`-padded exact-length dummy buffers prior to calling the standard file system formatting logic.

### On-Device Cryptography
Zero secrets are hardcoded. Provisioned Base32 secrets are decoded internally and utilized by the onboard cryptographic engine via the `mbedtls` library. Live HMAC-SHA1 calculations correctly output RFC 6238-compliant TOTP generation entirely inside the secure boundaries of the ESP32.

## Software Architecture
- **Strict Finite State Machine (FSM)**: The entire input/output interface operates asynchronously through a rigorously modeled finite state machine.
- **Hardware Abstraction**: Dependencies are injected via pure virtual interfaces (e.g., `INvsManager`), completely decoupling the business logic from underlying hardware implementation.
- **Native Unit Testing**: The framework guarantees complete verification without hardware. A suite of **34 native tests** rigorously evaluates the FSM, security utils, and cryptography by mocking hardware interactions on the host OS natively.

## Hardware Requirements
- **Microcontroller**: ESP32 Dev Module
- **Input**: Standard Quadrature Rotary Encoder (Switch, CLK, DT)
- **Display**: SPI TFT Display (e.g., ST7789/ILI9341 via TFT_eSPI)

## Provisioning & Usage
The device operates in an `UNPROVISIONED` state upon initial flash. To deploy credentials, establish a Serial CLI connection (115200 baud) and utilize the provisioning endpoint:
```text
PROVISION <4-digit-pin> <base32_secret>
```
Upon successful provisioning, the module reboots into the `LOCKED` state. Rotate the encoder to navigate digits, and depress the switch to confirm. Upon full verification, the radio enables and injects the synthesized TOTP code natively into any paired Bluetooth host.

## Time Synchronization & Windows BLE Limitation
Because the ESP32 lacks a dedicated battery-backed RTC, it must synchronize its internal Unix timestamp upon boot. The primary mechanism retrieves time from the central host via the BLE Current Time Service (CTS, 0x1805).

**Windows Fallback Limitations**: Modern Windows desktop environments generally restrict application-level arbitrary reads of the native Current Time Service characteristic over GATT. To circumvent this without compromising workflow, utilize the companion Python script:
```bash
cd tools
pip install -r requirements.txt
python auto_sync.py
```
This utility auto-detects the ESP32 serial bridge, meticulously suppresses DTR/RTS lines to bypass hardware resets, and automatically injects the exact UTC Unix epoch via the Serial CLI.
