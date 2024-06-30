<p align="center">
  <img src="https://www.project64-legacy.com/data/uploads/PJ64Plus_Clear.png" alt="logo" width="433" />
</p>

# Project64 1.6 Plus

Project64 1.6 Plus is a free and open-source emulator for the Nintendo 64 and It is written in C/C++ currently only for Windows. The project is a feature & vulnerability update fix of the original Project 64 1.6 source. It is targeted at the communities that still desire to use 1.6's ability to play Rom Hacks and a-like because of an incorect core that lets a lot of things slide when it comes to compatibility. 

This is not intended for development use or is it a modernised 1.6 core, it is just a "Safety Fix" For its loyal users and some added features that they may enjoy.

## Enhancements special mentions (amongst many)

- Arbitrary code vulnerability fix.

    TLB miss in write opcodes are now generating exceptions as expected. This solves a vulnerability that can allow roms
    to run arbitrary code in previous releases.  
	
- TLB: extra checks for overflow buffer mapping for user TLB entries

	Adds an extra check for user mapping TLB further than allowed buffer size.

- Retain 1.6 original ROM Hack compatibility

- Advance Mode enabled by default (allowing to name a few)

  - File/Rom Information
  - System/Screenshot Capture
  - Optiuons/Configure RSP Plugin
  - Help/About INIFiles 

- Save/load states (10 extra than original release)
- Game Information in File & Rom Browser (Popup Menu)
- Good Name replaced by Game Name in RDB & Browser Tab.
- Internal Name replaced by File Name in Title Bar when emulating.
- New Entry uses File name not Internal Name to add to Game Name in RDB
- Rom Browser uses File name to display a game not currently in RDB with the status of unknown,
  but once added to the RDB it will use the Game Name= instead.
- Always remember cheats as default so user don't have to re-enable after every close
- Max 10 Recent Roms as default
- Max 10 Recent Rom Dir as default
- Rom Dir Recursion as default, this allows sub-directories to be included in rom browser
- Jabo specific pemrcheats read from the jabo.ini so they do not effect other plugins as no longer enabled in the rdb.
- Display build date and time in About dialog title bar for easy build version recognition
- Help/Uninstall Application Settings
  - Uninstall application registry settings from windows registry and deletes Project64.cache 
- Help/Uninstall Jabo plugin Settings
  - Uninstall Jabo plugin settings from windows registry 

## Screenshot

<p align="center">
  <img src="https://www.project64-legacy.com/data/uploads/Docs/pj64plus_screen_about_26_06_2024.png" alt="screenshot" width="400" />
</p>

## Installation

Project64 1.6 Plus release can be found [here](https://github.com/pj64team/Project64-1.6-Plus/releases).

## Minimum requirements

* Operating system (No support for Windows 2000 and below)?
  *  Microsoft Windows XP and up?
* CPU
  * Intel or AMD processor with at least SSE2 support
* RAM
  * 512MB or more
* Graphics card
  * DirectX 8 capable (Jabo's Direct3D8/Icepir8sLegacyLLE)
  * OpenGL 3.3 capable (Icepir8sLegacyLLE/GLideN64)
  
<sub>Intel integrated graphics can have issues that are not present with Nvidia and AMD GPU's even when the requirements are met.</sub>

## Support

Please join our [Discord server](https://discord.gg/ha7HWAFE8uc) for support, questions, etc.

## Contributing

Contributors are always welcome!

## Maintainers and contributors

- [@Project64-1.6-Plus](https://github.com/pj64team/Project64-1.6-Plus)

- Zilmar (Tooie) - Project Founder / contributor
- Jabo - Contributor
- Smiff - Contributor
- Witten - Contributor
- RadeonUser - Contributor
- Gent - Contributor
- Azimer - Contributor
- Icepir8 - Contributor
- Nekokabu - Contributor
- Hacktarux - Contributor


Also see the list of [community contributors](https://github.com/pj64team/Project64-1.6-Plus/graphs/contributors).

## 🔗 Links
- [Website](https://github.com/pj64team/Project64-1.6-Plus)
- [Discord](https://discord.gg/TnFmnW6WQE)

