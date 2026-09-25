# Settings and GitHub OTA — Samsung-ESP32 0.4.0

[Русский](MANAGEMENT_RU.md)

## Local settings

Port80 hosts `/mqtt`, `/modbus`, `/updates`. Existing ESPHome controls/REST move to8080;
local ESPHome OTA remains3232. Modbus uses the0.3.0 map. Both web servers use the same
admin password. Initial private0.4.0 boot stores web/OTA/AP credentials from secrets.yaml
in NVS; later OTA images restore these values instead of their compiled defaults.

MQTT, RTU and TCP default off and have independent persistent checkboxes. Blank MQTT
password keeps the saved password; a separate checkbox clears it. Broker and discovery
changes apply without reboot. Changing topic prefix restarts to rebuild subscriptions;
UART then returns to monitor-only. MQTT topics/discovery remain ESPHome, not Haier
`/set/<field>`. Previously retained discovery messages are not removed when disabled.

Modbus changes apply immediately and close existing TCP clients. RTU uses8E1 with
9600/19200/38400/57600/115200 baud; unit1–247. Disabled RTU discards requests and disabled
TCP stops listening on502. Word2479 reports the transport flags. None enables AC UART.
Initial Wi-Fi configuration remains on the ESPHome captive portal at
`http://192.168.4.1:8080/` after joining Samsung-Setup; the port80 portal includes an AP link.

## Updates

Feed: `https://raw.githubusercontent.com/dk-1983/Samsung-ESP32-MQTT-Modbus/main/releases/stable.json`.
Profile: `samsung-s3-n16r8-v1`; assets must belong to Releases of that repository.
Automatic installation defaults on, as in Haier. Disabling it blocks installation but
allows checking. Checks start about one minute after boot and recur every6hours with
jitter. Network connectivity and synchronized clock are required.

The Haier worker supplies HTTPS verification, ECDSA P-256 manifest verification,
profile/size/SHA256/ESP32-S3 checks, separate app slot, trial boot confirmation and
rollback. A trial needs45seconds of healthy local services/network; failed restart or
timeout can select the previous slot. Samsung rollback has not yet been exercised.
The existing4vrs publisher public key is retained; no private key is copied. Distinct
Samsung profile/asset URLs reject Haier manifests.

The worker waits for the main-loop flash handoff and pauses local OTA while writing.
Transport setting changes are rejected during updates, but disabling installation
remains available during download. No live end-to-end updater test is claimed.

## Release

1. Install a private0.4.0 build first to migrate personal credentials to NVS.
   Public OTA is for provisioned devices, not first installation on a blank board.
2. Run `Prepare public OTA artifact` in GitHub Actions. Clean CI uses only
   `release.defaults.yaml`; packaging checks prevent local private binary publication.
3. Download and bench-test the artifact; install `requirements-release.txt`.
4. Sign with `python tools/sign_release.py --build <artifact-directory> --key <private-P256.pem> --out releases/stable.json`.
   The tool requires the key matching the firmware trust anchor.
5. Publish `v<version>` and its OTA asset, then the signed stable.json to main.

No release is published or device flashed by this change. Until stable.json exists,
the empty repository produces a manifest-fetch error. The supplied README banner is
stored unchanged at `docs/assets/banner_SRCC.png`.
