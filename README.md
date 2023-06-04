<h4 align="center">Several open source real-time pitchshifting platform independent apps and plugins.</h4>


Applications 
============
- MenguPitchy - Pitchshifted audio file player and saver
- MenguStretchy - Timestretched audio file player and saver

(Why not combine them into one app? Because I converted them from unit tests into apps and I'm too lazy to refactor the underlying code to stake them together)

Plugins (and formats)
===========
- Mengubah (lv2) - Real time pitch shifting plugin



Dependencies
============
- Miniaudio (included in repo) - Platform independent audio capture, playback and file backend. In theory supports .mp3, .wav and .ogg, but it struggles with certain files seemingly randomly. If file doesn't seem to want to load, try converting it to another one of these formats.
- Nanogui (included in repo) - Platform independent GUI. A bit barebones and ugly, but makes smaller binaries than feature rich libraries like qt and gtk


Other
=====
This project is released under GNU GPLv3 because I might want to add a VST3 plugin
