# OTA

This seems likely to be a little more involved than adding other code modules so the process is logged below.
Decisions:

* No factory slot - only two OTA slots.
* No signing of images.
* Alow HTTP (not HTTPS) for image download on a local presumably secure LAN.
* Use an MQTT message to tell the app to proceed with the OTA
* No App version.
* Use `python3 -m http.server` to serve the image.

## 2026-08-15 Add OTA support to the app.

* Fix flash size in the config (`idf.py menuconfig` → `Serial flasher config` → `Flash size`) - set to 4MB.
* Create `.../start/partitions_two_ota.csv` with contents:

```text
# Name,   Type, SubType, Offset,   Size,     Flags
nvs,      data, nvs,     0x9000,   0x4000,
otadata,  data, ota,     0xd000,   0x2000,
phy_init, data, phy,     0xf000,   0x1000,
ota_0,    app,  ota_0,   0x10000,  0x1F0000,
ota_1,    app,  ota_1,   ,         0x1F0000,
```

Note: This creates two OTA partitions without the "factory OTA" partition.

* Enable the partition CSV (`idf.py menuconfig` → `Partition Table` → `Partition Table` → `Custom partition table CSV`) using the file name above.
* Extend proj_mqtt for subscriptions (used to trigger OTA.) 
* Create the OTA component.

```text
mkdir -p components/proj_ota/include/
$EDITOR components/proj_ota/include/proj_ota.h
$EDITOR components/proj_ota/proj_ota.c
$EDITOR components/proj_ota/CMakeLists.txt
```

* Enable OTA configuration (`idf.py menuconfig` → `Component config` → `ESP HTTPS OTA`)
  * enable `Allow HTTP for OTA` (`CONFIG_ESP_HTTPS_OTA_ALLOW_HTTP`)
* Enable Rollback support `Bootloader config` → `Application rollback support` (`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`)
* add `"proj_ota"` to `.../start/main/CMakeLists.txt`
* Build, flash and test.

## 2026-08-15 Flash an update

Give the `.bin` its own directory path and run the server there. The path will include the project name and device name and will be rooted in an arbitrary location. Running the server will be rather ad-hoc but can be enshrined in Systemd if desired. The commands are tailored to be run on a diffrent host (which needs to be reachable by the ESP device.)

```text
cd /path/to/ESP32_create-project_start/start
export iot_host=spartan # substitute whatever hostname you serve from.
export target=esp32
export project=ESP32_create-project_start
ssh "$iot_host" "mkdir -p firmware/${project}/${target}"
scp build/start.bin "${iot_host}:firmware/${project}/${target}/"
ssh "$iot_host" "cd firmware/; python3 -m http.server 8080"
```

Sanity check that firmware can be served:

```text
export iot_host=spartan # substitute whatever hostname you serve from.
export target=esp32
export project=ESP32_create-project_start
cd /tmp
curl -I "http://{$iot_host}:8080/${project}/${target}/start.bin" 
```

Publish the trigger, substituting the publisher hostname and the ESP hostname.

```text
export iot_host=spartan # substitute whatever hostname you serve from.
export target=esp32
export project=ESP32_create-project_start
esp_host=esp32-b987b0
esp_host=esp32-486ab4
mosquitto_pub -h $iot_host -t CM/${esp_host}/system/ota -m "http://${iot_host}:8080/${project}/${target}/start.bin"
```
