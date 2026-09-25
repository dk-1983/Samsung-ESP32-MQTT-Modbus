# Attribution

Samsung Wi-Fi UART protocol reference: https://github.com/kumy/esphome_samsung_ac,
commit 2b7c14002c7fe0b2d8bb24f4c8d245fe6888bc5b, GPL-3.0.
The prototype is provided under GPL-3.0-or-later; LICENSE contains GPL v3.

`components/samsung_uart/modbus_core.h` originated in the user's
Haier-ESP32+Modbus project. Copyright (c) 2026 Krivolap Dmitriy Aleksandrovich.
MIT notice retained in LICENSE-4VRS. The adapted transport uses the samsung_modbus
namespace and a Samsung-specific backend function-code allowlist.

ESPHome and its build dependencies retain their respective upstream licenses.

Extended register reference: https://github.com/kumy/samsung-ac, commit 7d9df9d8010fe11a8171ebbab044a74d56ee441d, GPL-3.0, proto/consts.go. Register names do not establish support on the target model.

Management pages, updater, JSON parser, certificates and version comparison derive from
Dmitriy’s Haier-ESP32+Modbus fourvrs_portal (MIT; underlying4VRS attribution in LICENSE-4VRS).
Samsung portal adapters are GPL-3.0-or-later. Public trust key belongs to the same publisher;
no private key or private configuration was copied. Banner supplied by the user.
