# Samsung UART prototype

Target: ESP32-S3-WROOM-1 N16R8.
Keep changes within this Samsung project. The Modbus map follows MIM-B19N/B19NT conventions plus the extensions in docs/MODBUS.md.
Read README_RU.md, docs/PROTOCOL.md and docs/MODBUS.md before hardware changes.

Actual target AC: Samsung AR24BSFCMWKNER. Direct Wi-Fi UART D0 profile is experimental; core ventilation controls have been
verified on this model. Do not infer connector voltage/pins from another model or assume NASA == Wi-Fi UART.
User explicitly requested UART enabled on every boot starting with 0.4.2. Keep the local disable switch and gate all transmissions, including polls and ACKs, when disabled.
Never publish requested state as received state; keep freshness and command confirmation tests.
No hardware flashing or eFuse changes as part of routine builds. Do not publish secrets.yaml or binaries
containing private credentials to a public repository. Local binary delivery to the user is expected.

Build: PowerShell ./Build.ps1. Test only: ./Build.ps1 -TestOnly.
Tools are pinned in requirements.txt. New capabilities should have a documented protocol source and tests.
Keep English and Russian README descriptions of limitations consistent. Firmware has persistent MQTT/Modbus settings and a signed GitHub updater.
See docs/MANAGEMENT.md. Control REST uses port8080; management uses80. Public OTA requires provisioned NVS credentials.
