<h4 align="center">Several open source real-time pitchshifting platform independent apps and plugins.</h4>


Applications 
============
- MenguPitchy - Pitchshifted audio file player
- MenguStretchy - Timestretched audio file player
- MenguVoice - Voice changer with modular pitch shifter effects

(Why not combine Pitchy and Stretchy into one app? Because I converted them from unit tests into apps and I'm too lazy to refactor the underlying code to stake them together)

Plugins (and formats)
===========
- Mengubah (lv2) - Real time pitch shifting plugin



Dependencies
===========
- Miniaudio (included in repo) - Platform independent audio capture, playback and file backend. In theory supports .mp3, .wav and .ogg, but it struggles with certain files seemingly randomly. If file doesn't seem to want to load, try converting it to another one of these formats.
- Nanogui (included in repo) - Platform independent GUI. A bit barebones and ugly, but makes smaller binaries than feature rich libraries like Qt and GTK

- The LV2 SDK - Required only for building the LV2 plugin. On by default, disable with -DMENGU_LV2=OFF when creating the build tree

On Arch Linux:
`pacman -S lv2`

On other systems, use your appropriate package manager.

For MSVC and other build chains that don't have a package manager that can install lv2, you only need to include the lv2 header files. You can do so by cloning the lv2 repo at `https://github.com/lv2/lv2.git` and then setting the cmake variable `LV2_INCLUDE_PATH`. That is type `cmake .. -DLV2_INCLUDE_PATH=PATH/TO/LV2/REPO/include` when creating the build tree (See [Building](#building)). 

Building
==========
Like all CMake Projects, it's recommended to create a build directory to store the resulting build tree.
```
mkdir build
cd build
```

Create the build tree
```
cmake ..
```

Build it
```
cmake --build .
```

The results should now be in /build/bin

Tested on Linux gcc, MinGW, MSVC

Binaries
=====
For those who do not want to build it yourself [here is a link to some prebuilt binaries](https://drive.google.com/drive/folders/1WAx0_zWLrofNzsw-H7JzYDPCZMCYWLFI?usp=sharing)

Other
=====
This project is released under GNU GPLv3 because I might want to add a VST3 plugin
