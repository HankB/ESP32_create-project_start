# ESP32_create-project_start

Start an ESP32 with the bare minimum using `idf.py create-project`.

Be warned that I will be asking an LLM (Claude web page - free) to assist. My link is <https://claude.ai/chat/091e01ff-3e3d-43cb-843b-d74f6b8fd040> but that probably won't work for anyone else. I'm putting it here so I can tie it to this effort. At this point I have found that letting Claude write the code under my direction has been very productive. The model used is the default at this time:Sonnet 5. The only down side is that the free version runs out of tokens and I have to find something else to do until it recharges (usually at 1400 or 1430.) That also gives me time to give the code a second review and make any planned changes (that I didn't need Claude to do.) Benefits to using Claude:

* It knows what the overall structure of an ESP-IDF project should be and provides guidance WRT where to put things.
* It knows or can research the APIs so I don;t have to find them.
* It produces decent code, though I often see opportunities for improvement and suggestions to Claude to make the changes are met with reasoned discussion and (sometimes) fawning agreement.

## Motivation

As of early 2026 I have several projects that use the 5.x version of the libraries. They suffer from issues like not operating continuously. Rather than continue to work with those, I'm starting with V6 (6.0.2 specifically) to rebuild the basis of my platform: GPIO, WiFi, SNTP, MQTT. To complete the project, I'll choose a sensor with which to capture and publish readings.

## Status

* 2026-08-15 Add OTA update ([OTA notes](./OTA.md))
* 2026-08-06 Add sensor ID string to topic
* 2026-08-06 Add DS18B20 sensor(s)
* Original goal is complete. I still need to add a sensor. At present this code publishes a timestamp every 10s.
* 2026-08-04 Working MQTT
* 2026-08-04 Working SNTP
* 2026-08-04 Working WiFi
* 2026-08-03 Builds and flashes the on board LED for both ESP32 and ESP32-C3.

## Plans

* Additional diagnostics which can be usedul for further testing.

## Build

```text
source ~/.espressif/tools/activate_idf_v6.0.2.sh
cd ~/some/convenient/directory
git clone git@github.com:HankB/ESP32_create-project_start.git # For me or substitute your fork)
idf.py create-project start # I ran once, you don't need to unless starting your own project.
cd start
idf.py set-target esp32c3 # or idf.py set-target esp32
cp components/proj_wifi/secrets.h.example components/proj_wifi/secrets.h # fill in creds
idf.py build flash monitor # builds the project and flashes it. At present `main()` is empty.
```

## Errata

* The build system tends to put a lot of files in the top level directory so I created a `project` directory where the actual project will reside. That way the user won't have to scroll down the page to find the README contents. You're welcome. ... Never mind! the `start/` directory created by `idf.py create-project start` serves this function without the extra directory level.
* The output sense for the built in LED is reversed between the ESP32 and the ESP32-C3. That doesn't matter for simple on/off sequencing but for more involved signaling it will need to be addressed.
* The ESP32-C3 will occasionally have trouble associating with the AP and communicating over WiFi in general. Locating it where it has better "line of sight" to the AP seems to help.
* I have been working with an ESP32-C3 which connects to Linux as `/dev/ttyACM0` and an ESP32 WROOM which connects as `/dev/ttyUSB0`
* Whenever the target is changed (`idf.py set-target [esp32|esp32c3]`) the MQTT broker URI reverts to the default.
* Apparently the way that the one wire bus (RMT) driver works, it produces the following harmless warning:

```text
W (5798) rmt: GPIO 4 is not usable, maybe conflict with others
```

* When switching to a different target (e.g. esp32c3) The following paramters return to their default and must be reset.
  * Flash size 2MB -> 4MB
  * MQTT URI
  * Default SNTP server and number of servers.
