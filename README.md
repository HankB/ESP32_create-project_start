# ESP32_create-project_start

Start an ESP32 with the bare minimum using `idf.py create-project`.

Be warned that I will be asking an LLM (Claude web page - free) to assist. My link is <https://claude.ai/chat/091e01ff-3e3d-43cb-843b-d74f6b8fd040> but that probably won't work for you. I'm putting it here so I can tie it to this effort.

## Motivation

As of early 2026 I have several projects that use the 5.x version of the libraries. They suffer from issues like not operating continuously. Rather than continue to work with those, I'm starting with V6 (6.0.2 specifically) to rebuild the basis of my platform: GPIO, WiFi, SNTP, MQTT. To complete the project, I'll choose a sensor with which to capture and publish readings.

## Status

* 2026-08-04 Working SNTP
* 2026-08-04 Working WiFi
* 2026-08-03 Builds and flashes the on board LED for both ESP32 and ESP32-C3.

## Build

```text
source ~/.espressif/tools/activate_idf_v6.0.2.sh
cd ~/some/convenient/directory
git clone git@github.com:HankB/ESP32_create-project_start.git # For me or substitute your fork)
cd project
idf.py create-project start # I ran once, you don;t need to unless starting your own project.
cd start
idf.py set-target esp32c3
cp components/proj_wifi/secrets.h.example components/proj_wifi/secrets.h # fill in creds
idf.py build flash monitor # builds the project and flashes it. At present `main()` is empty.
```

## Errata

* The build system tends to put a lot of files in the top level directory so I created a `project` directory where the actual project will reside. That way the user won't have to scroll down the page to find the README contents. You're welcome. :D
* The output sense for the built in LED is reversed between the ESP32 and the ESP32-C3. That doesn't matter for simple on/off sequencing but for more involved signaling it will need to be addressed.
* The ESP32-C3 will occasionally have trouble associating with the AP and communicating over WiFi in general. Locating it where it has better "line of sight" to the AP seems to help.
