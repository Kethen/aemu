## Known issues and tips

### General issues
- PSN eboots are currently not supported
- More games now work on the PSP1000, but keep in mind that games that do work on slims have a not small chance to not work on 1000s
  - Games that uses memory partition 5 will likely not work properly eg. GTA LCS, GTA VCS
  - System save data dialog will probably stop working during/after multiplayer round. Games that saves without system dialog or provide their own save UI are not affected.
- Not all games work properly, pspnet_adhoc_matching implementation here still needs work to match PPSSPP's implementation
- Some users report that Adrenaline Bubble Manager on PSVita does not work with the plugin, I have no idea why

### Game issues
- In Star Wars The Force Unleashed, console should host when cross playing with PPSSPP, due to pspnet_adhoc_matching timing difference.
- [Games affected by cpu emulation inaccuracy desync on PPSSPP](https://github.com/hrydgard/ppsspp/issues/14361#issuecomment-814940104) will likely not work when cross playing with PPSSPP
- Dissidia 012 on PSVita has timing issues when qutting from a match and going back to lobby. It'll look normal but it won't be able to find other players most of the time. When that happens, go back to main menu then back to lobby.
