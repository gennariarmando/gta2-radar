# GTA2 Radar From GTA3
This is a backport of GTA3's radar mini map to GTA2, made thanks to re3 project.
Some changes have been made in order to make it work correctly in GTA2 but the base code is still the same.

## Features:
 - Full Map for each districts and bonus levels.
 - Shows mission blips from gangs, churches, garages.
 - Shows current objectives and markers height levels.
 - Rectangular radar.
 - Low quality assets.
 - Ability to add new radar tiles for custom levels.
 
## Screenshots:
<p align="center">
<img src="https://i.imgur.com/TSIEHVP.png" width="320" height="180">
<img src="https://i.imgur.com/I1kLIPR.png" width="320" height="180"> <br/>
<img src="https://i.imgur.com/EremRPB.png" width="320" height="180"> 
</p>

## Compiling:
Requirements:
 - Visual Studio 2022
 - [Plugin SDK](https://github.com/DK22Pac/plugin-sdk)

## Download:
Download the latest archive from the [releases](https://github.com/gennariarmando/gta2-frontend-fix/releases) page.

# Installation:
#### Installing an ASI Loader:
An ASI Loader is required in order to inject the plugin into the game, if you already have one skip to the next step.\
Recommended: [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)

Place the DLL file (renamed into "dinput.dll") into your GTA2 directory.

Requires [Widescreen Fix](https://thirteenag.github.io/wfp#gta2)

#### Installing GTA2 Radar:
Archive content: 
- GTA2Radar.asi
- data\hud folder
- data\radar folder

Paste the content of the "data" folder into your GTA2 "data" directory.\
Create a folder called "scripts" inside your GTA2 directory and paste GTA2Radar.asi in it.

## Links:
- [plugin-sdk](https://github.com/DK22Pac/plugin-sdk)
- [re3](https://github.com/GTAModding/re3)
