# Settings and GitHub OTA — Samsung-ESP32 1.0.2

[Русский](MANAGEMENT_RU.md)

## Portal language (1.0.1)

The port 80 portal defaults to English. Choose English / Русский in the navigation;
the browser stores a `samsung_ui_lang` cookie for one year (`Path=/`, `SameSite=Lax`).
A missing or unsupported value selects English. The preference applies to every
portal page, including Wi-Fi reset. Switching reloads the page and discards unsaved
form edits. It does not change controller settings or NVS and requires no device
restart. Clearing cookies or changing browsers or controller addresses restores
English. This selector does not control the advanced ESPHome UI on port 8080.

### Localization checks

Russian page templates remain the source. `components/samsung_portal/i18n/en.tsv`
maps their UI text to English. Run `python tools/build_ui_translations.py` after
editing a template or translation and include the generated `UiEnglish.h`.
`./Build.ps1 -TestOnly` checks catalog completeness, form contracts, all nine
language routes and the actual C++ page renderer, as well as protocol/bridge tests.

Browser checks use pinned Playwright from `tests/ui/package.json` and simulated
API responses, never a controller. After host tests, run:

```text
npm install --prefix tests/ui --ignore-scripts --no-package-lock
node tests/ui/node_modules/playwright/cli.js install chromium
node tests/ui/browser.cjs
```

The public build workflow runs these checks before compiling the OTA image.
Browser checks cover English/Russian, desktop/mobile layouts, cookie persistence,
forms, stale/offline feedback and confirmation dialogs. They do not certify a
firmware upload or device operation. Publish and verify the OTA asset before
updating the signed stable feed; never manually edit its signed payload.


## Local settings

### Wi-Fi (0.4.5)

`/wifi` shows SSID, IP, station MAC, RSSI in dBm, channel and setup AP status.
`/wifi/reset` separately confirms removal of only the saved network. Know the
Samsung-Setup password before resetting. Confirmation erases the network record
and restarts after two seconds; MQTT, Modbus, web/OTA/AP credentials and updater
settings are preserved. Join Samsung-Setup and open `http://192.168.4.1:8080/`
to provision a new 2.4 GHz network. Explicit reset does not restore the old network.
Ordinary connection loss starts the setup AP after roughly ten seconds without
erasing the saved network.

Authenticated GET `/wifi/status` never returns passwords. GET `/wifi/reset` is
read-only; POST requires authentication, CSRF `token` and `confirm=RESET_WIFI`.
It is blocked during OTA, pending restart or an AC command. Only the Wi-Fi NVS
record is erased, never all preferences. Configuration validation requires
captive provisioning without compiled station networks; the key layout must be
rechecked when upgrading the pinned ESPHome version.

Read-only operation and request guards are checked on the live bench. Actual
network deletion and reprovisioning require a separate test with setup AP access.

### Passwords (0.4.4)

`/settings` changes web authentication (`admin`, ports 80 and 8080), local OTA
and Samsung-Setup access point passwords independently. Each new password needs
confirmation; blank pairs preserve existing values. New web/OTA passwords accept
16–64 printable ASCII characters without spaces; AP passwords accept 8–63.
Existing web passwords of at least eight characters remain valid.

All fields are validated before a single NVS blob is written. Errors leave all
credentials unchanged. Actual changes restart the board after two seconds;
unchanged values cause neither a write nor a restart. Credentials survive OTA
and are never returned by the API. Sign in with the new web password after reboot.
Future local OTA uploads require the new OTA password; the computer's `secrets.yaml`
is not updated automatically.

Home Wi-Fi, MQTT and GitHub signature verification are unaffected. Changes are
blocked during OTA, scheduled restart or pending AC commands. Forgotten-password
recovery is not implemented; this page requires existing access.

Authenticated `GET /settings/config` returns only `credentials_ready`.
POST to the same endpoint requires CSRF `token` and matching pairs:
`web_password`/`web_confirm`, `ota_password`/`ota_confirm`,
`setup_password`/`setup_confirm`. Empty or omitted pairs preserve credentials.
Unknown/duplicate fields are rejected. Responses contain only `changed` and `restarting`.

### System information and restart (0.4.3)

`/about` shows firmware, hostname, IP, Wi-Fi station MAC, uptime, internal RAM,
PSRAM and Flash. Wi-Fi, MQTT, UART, AC freshness and enabled Modbus transports
are reported separately. The station MAC can be used for a DHCP reservation.

Restart requires confirmation and preserves settings. The page waits for a new
boot and reloads after reconnection. Restart is rejected during OTA or a pending
AC command. UART starts enabled. If the IP changes, reopen the page at its new address.

API: authenticated `GET /system/status` and `POST /system/restart`. POST also
requires the current CSRF `token` and `confirm=RESTART`; GET never restarts.
Diagnostics contain no passwords. `boot_id` changes on boot, `uptime_s` is uptime;
`storage_ok` reports storage initialization, not a continuous memory test.

Port80 hosts `/mqtt`, `/modbus`, `/updates`. Existing ESPHome controls/REST move to8080;
local ESPHome OTA remains3232. Modbus uses the0.3.0 map. Both web servers use the same
admin password. Initial private0.4.0 boot stores web/OTA/AP credentials from secrets.yaml
in NVS; later OTA images restore these values instead of their compiled defaults.

MQTT, RTU and TCP default off and have independent persistent checkboxes. Blank MQTT
password keeps the saved password; a separate checkbox clears it. Since 0.4.2 every MQTT settings save restarts the controller after two seconds
to recreate the MQTT client with the saved broker and credentials;
UART starts enabled. MQTT topics and discovery use ESPHome. Previously retained discovery messages are not removed when disabled.

Modbus changes apply immediately and close existing TCP clients. RTU uses8E1 with
9600/19200/38400/57600/115200 baud; unit1–247. Disabled RTU discards requests and disabled
TCP stops listening on502. Word2479 reports the transport flags. None enables AC UART.
Initial Wi-Fi configuration remains on the ESPHome captive portal at
`http://192.168.4.1:8080/` after joining Samsung-Setup; the port80 portal includes an AP link.

## Updates

Feed: `https://raw.githubusercontent.com/dk-1983/Samsung-ESP32-MQTT-Modbus/main/releases/stable.json`.
Profile: `samsung-s3-n16r8-v1`; assets must belong to Releases of that repository.
Automatic installation defaults on. Disabling it blocks installation but
allows checking. Checks start about one minute after boot and recur every6hours with
jitter. Network connectivity and synchronized clock are required.

The updater provides HTTPS verification, ECDSA P-256 manifest verification,
profile/size/SHA256/ESP32-S3 checks, separate app slot, trial boot confirmation and
rollback. A trial needs45seconds of healthy local services/network; failed restart or
timeout can select the previous slot. Samsung rollback has not yet been exercised.
The existing4vrs publisher public key is retained; no private key is copied. Distinct
Samsung profile/asset URLs reject manifests for other devices.

The worker waits for the main-loop flash handoff and pauses local OTA while writing.
Transport setting changes are rejected during updates, but disabling installation
remains available during download. GitHub OTA 0.4.0 → 0.4.1 passed on the device:
signed download, installation, preserved settings/credentials and trial boot confirmation.

## Release

1. For a blank board, install a local build of the current stable source with personal
   `secrets.yaml` and `public_release: "false"`; first boot saves credentials in NVS.
   Inline bridge support starts with 0.5.0; 0.4.x uses the earlier non-bridge
   configuration. Verify the GPIO wiring against the current schematic before
   updating hardware wired for a pre-0.5.0 firmware. On the current hardware, configured
   1.0.0/1.0.1 controllers can update directly. No old base firmware is required.
   Public OTA requires saved credentials and is not a first-install image.
2. Run `Prepare public OTA artifact` in GitHub Actions. Clean CI uses only
   `release.defaults.yaml`; packaging checks prevent local private binary publication.
3. Download and bench-test the artifact; install `requirements-release.txt`.
4. Sign with `python tools/sign_release.py --build <artifact-directory> --key <private-P256.pem> --out releases/stable.json`.
   The tool requires the key matching the firmware trust anchor.
5. Publish `v<version>` and its OTA asset, then the signed stable.json to main.

Stable release 1.0.2 provides a ready-made OTA binary and a signed stable update manifest. Public binaries require an already provisioned controller.

[Home Assistant MQTT Discovery setup and validation status](HOME_ASSISTANT.md).
