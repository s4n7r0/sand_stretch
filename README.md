# sand_stretch2
Remake of my remake of dblue_stretch, with some stuff added in.

[Download it from here.](https://github.com/s4n7r0/sand_stretch/releases)

<img src="https://i.imgur.com/G4KyO4w.png" width = 400>

# What does it do?
Stretches out audio, similar to how dblue_stretch does it. <br>
But since everyone I know doesn't use it in that way (me included), <br>
it also works as a sort of "glitch" machine. <br>
It collects the input into the buffer and then outputs grains <Br>
with the given size, delayed by the given ratio. <br>
Grains can be held in place, played in reverse, synced to a note duration, crossfaded, <br> 
or only grains where samples at the beginning and the end of it crossed 0 (zcrossed samples) <br>

## Parameters

| Name          | Description                                               |
| ------------- | --------------------------------------------------------- |
| trigger       | start collecting samples and simultaneously output grains | 
| hold          | hold current grain |
| hold offset   | offset currently held grain by amount of samples |
| tempo         | use size in note lengths instead of samples  |
| reverse       | play in reverse  |
| declick       | smooth out the edges between the end of the current grain <br> and start of the next one <br> 4 * (2 ^ declick) samples will be declicked | 
| grain         | size of a grain in samples |
| note          | size of a grain in note duration |
| ratio         | how much should the input be stretched out by |
| subdivision   | adjusts note duration |
| zcross size   | if set, grain gets offset to the closest zcrossed sample <br> then it's size is the distance to the next zcrossed sample <br> determined by the amount |
| zcross offset | offsets grain to the next window of zcrossed samples |
| crossfade     | crossfades between grains |


## How to build from source

### Prerequesities

- JUCE
- Your system C++ build toolchain (Visual Studio on Windows, XCode on Mac, GCC/Clang on Linux, etc.

## Building (the Projucer way)

- ```git clone https://github.com/s4n7r0/sand_stretch.git```
- Open Projucer
- Open `sand_stretch_remake.jucer` in Projucer
- Select your system build configuration, save the project
- Build the plugin using the generated project in the `Build` folder. Which means:
  + Run Visual Studio/Xcode and open the appropriate project, either the generated `.sln`/`xcodeproject` or thru Projucer (Win/Mac)
  # MAC
  + If wanna build the plugin in AU format, you need to add AU plugin format in Project Settings's in Projucer
  + Product > Scheme > Edit Scheme... > Run > Build Configuration > Select "Release"
  + Build
  + Built plugin in any format should be copied to "~/Library/Audio/Plug-Ins/" to their respective folders, but if not:
  + Copy the compiled sand_stretch2.vst3 from "Builds/MaxOSX/build/Release/sand_stretch2.vst3" to "~/Library/Audio/Plug-Ins/VST3"
  + Copy the compiled sand_stretch2.component from "Builds/MaxOSX/build/Release/sand_stretch2.component" to "~/Library/Audio/Plug-Ins/Components" ( if building as an AU )
  # WINDOWS
  + Select "Release" configuration (Win)
  + Build
  + Copy the compiled sand_stretch2.vst3 from "Builds\VisualStudio2022\x64\Release\VST3\sand_stretch2.vst3\Contents\x86_64-win\" to "C:\Program Files\Common Files\VST3\" (Win)
  # LINUX
  + `cd Builds/LinuxMakefile && make CONFIG=Release`
  + Built VST3 should be copied to "~/.vst3/" but if not:
  + `mkdir ~/.vst3`
  + `mv "Builds/LinuxMakefile/build/sand_stretch2.vst3/Contents/x86_64-linux/sand_stretch2.so" "~/.vst3/sand_stretch2.vst3"`

## Credits

[Illformed](https://illformed.com/)

[JUCE](https://juce.com/)

## Support

[Gumroad](https://s4n7r0.gumroad.com) <br>
[Soundcloud](https://www.soundcloud.com/s4n7r0) <br>
[Twitter](https://www.twitter.com/s4n7r0) <br>
[Bandcamp](https://s4n7r0.bandcamp.com/) <br>
[Bluesky](https://bsky.app/profile/sandr0.bsky.social) <br>

