# Chrony_GSR

![Current Chrony GSR Face](https://github.com/GuruSR/Chrony_GSR/blob/main/Images/chrony-face.png)

## Current tweaks

- [Pxl999 port](https://github.com/dezign999/pxl999) for Watchy_GSR (plus battery voltage)
- Show current temperature in watch face (for now, local sensor only)
- Menu overrides:
    - Back: shortcut to auto-start Elapsed Timer
    - Down: Send HTTP GET to predefined URL. In my case, I use it to send a Tasker AutoRemote action (to turn on my lights) :)
- Required library tweaks are automated by PlatformIO's build process
- And probably more to come

## Setup

0. Instal Platform IO Core
1. Clone the repo
2. 
```bash
cd Chrony_GSR
pio run -t upload
```

Done! Library patches will be executed by the build process

Please see [Watchy_GSR's repo](https://github.com/GuruSR/Watchy_GSR/blob/main/Usage.md) for Usage and general information.