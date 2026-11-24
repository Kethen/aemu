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
1. Go to Settings -> "Network" -> "Wi-Fi Settings" and connect to your Wi-Fi hotspot, use Settings -> "Network" -> "Internet Connection Test" to make sure you have working internet connection on the PSVita
2. Go to Settings -> "Power Save Settings" and disable "Use Wi-Fi in Power Save Mode"
3. (Adrenaline) In XMB -> "Settings" -> "Power Save Settings", set "WLAN Power Save" to "off"
4. (Adrenaline) Press select in XMB -> RECOVERY MENU, and change these settings

| Name | Set to |
| -- | -- |
| CPU Speed in UMD/ISO | 333/166 |
| Advanced -> Force High Memory Layout | Disabled |

4. (ARK Standalone) Press triangle in Custom Launcher -> CFW Settings, and change these settings

| Name | Set to |
| -- | -- |
| CPU Clock in Game | OverClock |
| Use Extra Memory | Auto |
| Memory Stick Speedup | Off |
| Inferno Cache | Off |

5. Copy folder `kd` and `seplugins` inside `dist_660_release` to the root of your PSP memory stick, assuming you did not change that in adrenaline, that would be `ux0:/pspemu`.
6. (Adrenaline) add the following line to the text file `ux0:/pspemu/seplugins/game.txt` **if it does not already exist**
```
ms0:/seplugins/atpro.prx 1
```
6. (ARK Standalone) add the following line to the text file `ux0:/pspemu/seplugins/plugins.txt` **if it does not already exist**
```
game, ms0:/seplugins/atpro.prx, on
```
7. Remove `ux0:/pspemu/kd/pspnet_miniupnc.prx` if exists

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
| psp.mgn.pub | Madness Gaming Network https://discord.com/invite/kaPScVrPes | Alaska USA | For players looking to play any games, has a good amount of Monster Hunter players, also provides VPN |
| eahub.eu | EA Nation Hub https://discord.com/invite/fwrQHHxrQQ | France | Mostly for Medal of Honor Heros 2 players, but can be used for other games |
| psi-hate.com | PSP Online https://discord.com/invite/wxeGVkM | Minnesota USA | For players looking to play any games |

- `ms0:/seplugins/port_offset.txt` / `ef0:/seplugins/port_offset.txt` / `ux0:/pspemu/seplugins/port_offset.txt` can be changed if you wish to change your port offset, mirrors the port offset option in PPSSPP
- `ms0:/seplugins/nickname.txt` / `ef0:/seplugins/nickname.txt` / `ux0:/pspemu/seplugins/nickname.txt` can be added to override your system nickname, mostly useful for PSVita ARK Standalone if games don't work with your nickname generated from PSVita PSN
- `hotspot.txt` lines can also contain the name of an infrastructure mode profile instead of its SSID. This is useful when several profiles use the same SSID, e.g. one with automatic and one with manual IP settings: a unique SSID match is used first, otherwise the line is looked up as a profile name
- All of the above can also be set in one file, `ms0:/seplugins/atpro.ini` / `ef0:/seplugins/atpro.ini` / `ux0:/pspemu/seplugins/atpro.ini` (PPSSPP: `ms0:/PSP/PLUGINS/atpro/atpro.ini`). Settings found there take precedence, anything missing falls back to the separate files above. One `key = value` per line, lines starting with `#` or `;` are comments, `hotspot` can be repeated to list several profiles in order:
  ```
  server = socom.cc
  port_offset = 10000
  hotspot = MyProfileName
  # hotspot = MyOtherProfile
  nickname = Hunter
  hud_on_home = 1
  hud_text_background = 0
  allow_sleep = 0
  ```
  `hud_on_home = 0` makes the plugin leave the HOME button alone: HOME then neither toggles the chat HUD nor gets the system home menu blocked, so custom home menu plugins keep working. Use L+R+SELECT to open the HUD instead.
  `hud_text_background = 1` draws a black box behind each text of the HUD, so it stays readable on bright game screens. The rest of the screen and the "incoming message" notifications are not affected.
  `allow_sleep = 1` keeps sleep mode available while the WLAN switch is on. By default the plugin blocks sleep for the whole game session, because sleeping drops the network connection; sleeping while connected will most likely end the session.
  `postoffice = 1` / `postoffice = 0` turns postoffice (relay) mode on or off, see [Classic P2P mode](#classic-p2p-mode). Without this key, postoffice mode is used whenever `kd/aemu_postoffice.prx` exists
- `atpro.ini` can hold several **profiles**, e.g. one per adhoc server, and you can switch between them in the HUD. A `[name]` line starts a profile; keys before the first profile apply to all of them. A key set in the active profile replaces the global one (for `hotspot`, the profile's whole list replaces the global list):
  ```
  nickname = Hunter
  hud_on_home = 0

  [socom]
  server = socom.cc
  port_offset = 10000
  hotspot = HomeWifi
  postoffice = 1

  [other]
  server = adhoc.example.com
  port_offset = 0
  hotspot = OtherWifi
  postoffice = 0
  ```
  - To switch, open the HUD (L+R+SELECT, or HOME) while not connected, e.g. in the village or the game menu, and press LEFT/RIGHT on the `PROFILE: < NAME >` line. While connected the HUD only shows the active profile, the chat keeps all buttons
  - The new profile is used from the next connection (`sceNetAdhocInit` / `sceNetAdhocctlInit`): leave the online area and enter it again. No game restart is needed. Server address, port offset and postoffice mode are only re-read when the profile actually changed; the hotspot list is read on every connection
  - The choice is saved in `kd/atpro_profile.txt` on the same storage as the `atpro.ini` that was used (`ms0:/kd/`, `ef0:/kd/`, PPSSPP: `ms0:/PSP/PLUGINS/atpro/`), so it is kept across game starts. Without that file, or if it names a profile that doesn't exist anymore, the first profile is used
  - `nickname`, `hud_on_home`, `hud_text_background` and `allow_sleep` are only read when the game starts. Put them before the first profile; set inside a profile they only take effect after restarting the game with that profile active
  - Up to 8 profiles, names up to 31 characters, case insensitive. Without any `[name]` line `atpro.ini` works as a single configuration like before
- On PSVita, https://github.com/Kethen/pspemu_inet_multithread can be used on top to make some games run faster during adhoc multiplayer

- <details>
	<summary>Classic P2P mode

	**DO NOT DO THIS IF YOU DON'T KNOW WHAT CLASSIC P2P MODE MEANS**

	</summary>

	<a id="classic-p2p-mode"/>classic P2P mode can be enabled by renaming `kd/aemu_postoffice.prx` to `kd/aemu_postoffice.prx.bak`, or by setting `postoffice = 0` in `atpro.ini`. With [profiles](#advanced-configurations), `postoffice` can be set per profile, so `kd/aemu_postoffice.prx` can stay installed for servers that use postoffice mode while classic P2P is used for others

</details>

### <a id="Troubleshooting" />Troubleshooting

- Note that not all games currently work. Try a different game to see if it's an installation issue or game specific issue.
- Some plugins might conflict with aemu, disable all other plugins and try again.
- If you have a lot of random lag/disconnects, try to improve your networking environment.
  - Choose a relay server you have stable connection to, usually means the server you are physically the closest to.
  - Create a wireless hotspot with a clean 2.4GHz channel.
    - On PSP, 802.11b absolutely requires this, find a dedicated channel for your PSP, if you have multiple PSPs, it might be a good idea to have a dedicated channel for each PSP.
    - On PSVita, it's less bad with 802.11n, but it's still a good idea to have a less busy channel. It's usually okay to share a dedicated channel among a few PSVitas.
    - Apps like https://github.com/VREMSoftwareDevelopment/WifiAnalyzer might help finding a clean/less busy 2.4GHz channel.
- Install `kd` and `seplugins` from `dist_660_debug` instead, then run your game again. `ms0:/atpro.log` / `ef0:/atpro.log` / `ux0:/pspemu/atpro.log` will contain debug information that might help solving your issues.

### <a id="crossplaying-with-ppsspp" /> Crossplaying with PPSSPP

1. Download latest PPSSPP from https://www.ppsspp.org/devbuilds/ , or 1.20+ when released
2. Follow the ["Multiplayer With A Public Server on the Internet (aemu_postoffice relay on version 1.20+)" section here](https://github.com/hrydgard/ppsspp/wiki/How-to-play-multiplayer-games-with-PPSSPP#multiplayer-with-a-public-server-on-the-internet-aemu-postoffice) to configure your PPSSPP
