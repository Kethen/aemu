## Installation

### Index

- [PSP 2000/3000](#psp-2000-3000)
- [PSVita](#psvita)
- [PSP Go](#psp-go)
- [Advanced configurations](#advanced-configurations)
- [Troubleshooting](#troubleshooting)
- [Crossplaying with PPSSPP](#crossplaying-with-ppsspp)

### <a id="psp-2000-3000"/>PSP 2000/3000

0. Make sure you have installed the newest CFW, 6.61 pro-c is still somewhat supported, but future releases might require 6.61 ARK-4
1. In XMB -> "Settings" -> "Network Settings", configure your "Infrastructure Mode" connection, make sure to note down the "SSID" of your hotspot, and test the connection with the "Test Connection" feature under the triangle button context menu to make sure that it is working. Avoid using special characters in the SSID of your hotspot.
2. In XMB -> "Settings" -> "Power Save Settings", set "WLAN Power Save" to "off"
3. In XMB -> "Settings" -> "System Settings",  set "UMD Cache" to "off"
4. (PRO cfw) Press select in XMB ->RECOVERY MENU, and change these settings

| Name | Set to |
| -- | -- |
| Advanced -> Memory Stick Speedup | None |
| Advanced -> Inferno & NP9660 Use ISO Cache | Disabled |
| Advanced -> Force High Memory Layout | Disabled |
| CPU Speed -> Game CPU/BUS | 333/166 |

4. (ARK-4 cfw) Press select in XMB -> RECOVERY MENU -> Custom Firmware Settings, and change these settings

| Name | Set to |
| -- | -- |
| CPU Clock in Game | OverClock |
| Use Extra Memory | Auto |
| Memory Stick Speedup | Off |
| Inferno Cache | Off |

5. Copy folder `kd` and `seplugins` inside `dist_660_release` to the root of your memory stick, aka `ms0:/`
6. (PRO cfw) add the following line to the text file `ms0:/seplugins/game.txt` **if it does not already exist**
```
ms0:/seplugins/atpro.prx 1
```
6. (ARK cfw) add the following line to the text file `ms0:/seplugins/plugins.txt` **if it does not already exist**
```
game, ms0:/seplugins/atpro.prx, on
```
7. Open `ms0:/seplugins/hotspot.txt`, and write the "SSID" of your hotspot into the file. This will make the plugin use the infrastructure mode profile you have created earlier.
8. Remove `ms0:/kd/pspnet_miniupnc.prx` if exists

### <a id="psvita"/>PSVita

0. Make sure you are on 6.61 Adrenaline or ARK standalone CFW
1. Go to Settings -> "Network" -> "Wi-Fi Settings" and connect to your Wi-Fi hotspot, use Go to Settings -> "Network" -> "Internet Connection Test" to make sure you have working internet connection on the PSVita
2. Go to Settings -> "Power Save Settings" and disable "Use Wi-Fi in Power Save Mode"
3. (Adrenaline) In XMB -> "Settings" -> "Power Save Settings", set "WLAN Power Save" to "off"
4. (Adrenaline) Press select in XMB ->RECOVERY MENU, and change these settings

| Name | Set to |
| -- | -- |
| CPU Speed in UMD/ISO | 333/166 |
| Advanced -> Force High Memory Layout | Disabled |

5. (ARK Standalone) Press triangle -> CFW Settings, and change these settings

| Name | Set to |
| -- | -- |
| CPU Clock in Game | OverClock |
| Use Extra Memory | Auto |
| Memory Stick Speedup | Off |
| Inferno Cache | Off |

6. Copy folder `kd` and `seplugins` inside `dist_660_release` to the root of your PSP memory stick, assuming you did not change that in adrenaline, that would be `ux0:/pspemu`.
7. (PRO cfw) add the following line to the text file `ux0:/pspemu/seplugins/game.txt` **if it does not already exist**
```
ms0:/seplugins/atpro.prx 1
```
7. (ARK Standalone) add the following line to the text file `ux0:/pspemu/seplugins/plugins.txt` **if it does not already exist**
```
game, ms0:/seplugins/atpro.prx, on
```
8. Remove `ux0:/pspemu/kd/pspnet_miniupnc.prx` if exists


### <a id="psp-go"/>PSP Go

0. Make sure you have installed the newest CFW, 6.61 pro-c is still somewhat supported, but future releases might require 6.61 ARK-4
1. In XMB -> "Settings" -> "Network Settings", configure your "Infrastructure Mode" connection, make sure to note down the "SSID" of your hotspot, and test the connection with the "Test Connection" feature under the triangle button context menu to make sure that it is working. Avoid using special characters in the SSID of your hotspot.
2. In XMB -> "Settings" -> "Power Save Settings", set "WLAN Power Save" to "off"
3. (PRO cfw) Press select in XMB ->RECOVERY MENU, and change these settings

| Name | Set to |
| -- | -- |
| Advanced -> Memory Stick Speedup | None |
| Advanced -> Inferno & NP9660 Use ISO Cache | Disabled |
| Advanced -> Force High Memory Layout | Disabled |
| CPU Speed -> Game CPU/BUS | 333/166 |

3. (ARK-4 cfw) Press select in XMB -> RECOVERY MENU -> Custom Firmware Settings, and change these settings

| Name | Set to |
| -- | -- |
| CPU Clock in Game | OverClock |
| Use Extra Memory | Auto |
| Memory Stick Speedup | Off |
| Inferno Cache | Off |

4. Copy folder `kd` and `seplugins` inside `dist_660_release` to the root of your memory stick, aka `ms0:/`, **AND** the root of your internal storage, aka `ef0:/`
5. (PRO cfw) add the following line to the text file `ms0:/seplugins/game.txt` **if it does not already exist**
```
ms0:/seplugins/atpro.prx 1
```
5. (ARK cfw) add the following line to the text file `ms0:/seplugins/plugins.txt` **if it does not already exist**
```
game, ms0:/seplugins/atpro.prx, on
```
6. (PRO cfw) add the following line to the text file `ef0:/seplugins/game.txt` **if it does not already exist**
```
ef0:/seplugins/atpro.prx 1
```
6. (ARK cfw) add the following line to the text file `ef0:/seplugins/plugins.txt` **if it does not already exist**
```
game, ef0:/seplugins/atpro.prx, on
```
7. Open `ms0:/seplugins/hotspot.txt`, and write the "SSID" of your hotspot into the file. This will make the plugin use the infrastructure mode profile you have created earlier.
8. Open `ef0:/seplugins/hotspot.txt`, and write the "SSID" of your hotspot into the file. This will make the plugin use the infrastructure mode profile you have created earlier.
9. Remove `ms0:/kd/pspnet_miniupnc.prx` and `ef0:/kd/pspnet_miniupnc.prx` if exists

### <a id="advanced-configurations"/>Advanced configurations

- `ms0:/seplugins/server.txt` / `ef0:/seplugins/server.txt` / `ux0:/pspemu/seplugins/server.txt` can be changed if you chose to use an adhoc server other than `socom.cc`. Here is a list of currently available servers

| Server Address | Community chat for organizing play sessions | Location | Note |
| -- | -- | -- | -- |
| socom.cc | Socom Adhoc Server https://discord.com/invite/XtVYDr7 | France | For players looking to play any games |
| 64.110.29.52 | Madness Gaming Network https://discord.com/invite/kaPScVrPes | Alaska USA | For players looking to play any games, have a good amount of Monster Hunter players, domain name for relay server pending, also provides VPN |
| eahub.eu | EA Nation Hub https://discord.com/invite/fwrQHHxrQQ | France | Mostly for Medal of Honor Heros 2 players, but can be used for other games |
| psi-hate.com | PSP Online https://discord.com/invite/wxeGVkM | Minnesota USA | For players looking to play any games |

- `ms0:/seplugins/port_offset.txt` / `ef0:/seplugins/port_offset.txt` / `ux0:/pspemu/seplugins/port_offset.txt` can be changed if you wish to change your port offset, mirrors the port offset option in PPSSPP
- On PSVita, https://github.com/Kethen/pspemu_inet_multithread can be used on top to make some games run faster during adhoc multiplayer
- **DO NOT DO THIS IF YOU DON'T KNOW WHAT CLASSIC P2P MODE MEANS**, classic P2P mode can be enabled by renaming `seplugins/aemu_postoffice.prx` to `seplugins/aemu_postoffice.prx.bak`

### <a id="Troubleshooting" />Troubleshooting

- Note that not all games currently work. Try a different game to see if it's an installation issue or game specific issue.
- Install `kd` and `seplugins` from `dist_660_debug` instead, then run your game again. `ms0:/atpro.log` / `ef0:/atpro.log` / `ux0:/pspemu/atpro.log` will contain debug information that might help solving your issues.

### <a id="crossplaying-with-ppsspp" /> Crossplaying with PPSSPP

1. Download latest PPSSPP from https://www.ppsspp.org/devbuilds/ , or 1.20+ when released
2. Follow the ["Multiplayer With A Public Server on the Internet (aemu_postoffice relay on version 1.20+)" section here](https://github.com/hrydgard/ppsspp/wiki/How-to-play-multiplayer-games-with-PPSSPP#multiplayer-with-a-public-server-on-the-internet-aemu-postoffice) to configure your PPSSPP
