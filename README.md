# sand_stretch
Remake of dblue_stretch in JUCE framework with some stuff added in.

[Download it from here.](https://github.com/s4n7r0/sand_stretch/releases)

![image](https://github.com/s4n7r0/sand_stretch/assets/116836670/267690ba-e731-4682-a088-e9d52e7f38f7)

## Important notes

This plugin was not tested in every daw under every condition. Issues may occur
such as audio getting desynced, crashes or lag.

The provided .vst3 was built on a windows machine and may or may not work on a mac.

## Info

I made this plugin because:

- I wanted to learn plugin development.

- 64-bit version of dblue_stretch doesn't exist (rip ableton :<)

- i wanted to add some features to the original plugin.

Didn't do it in an object oriented style cause im a lazy bum and rather
do it procedurally, so excuse my big processBlock lmao.

## Parameters

| Name | Description |
| ----------- | ----------- |
| Trigger               | Self-explanatory. |
| Hold                  | Holds current frame in place. | 
| Reverse               | Outputs samples in reverse. |
| Reduce DC Offset      | Attempts to remove dc offset but often fails lol, use an eq after to be safe. |
| Ratio                 | Stretches input by that amount, e.g. ratio = 2.5, audio gets stretched out by 2.5x. |
| Samples               | How many samples to keep in a frame. |
| Skip Samples          | Instead of outputting samples one by one, it skips some by that amount. (**WARNING**: it's dangerous because it can lead to weird issues with other features.) |
| Crossfade             | Crossfades beginning and the end of a frame to smooth it out. |
| Hold Offset           | Offsets currently held frame by that amount. |
| zCross Window Size    | Holds a frame of samples that crossed 0. Only works when Hold is on. |
| zCross Window Offset  | Offsets the window of zcrossing samples. |
| bufferSize            | How many seconds to store in the buffer. `(sampleRate * bufferSize)` amount of samples.|

## Credits

- Illformed - creator of dblue_stretch - https://illformed.org/
- JUCE Framework - https://juce.com/

## Support

[Soundcloud](https://www.soundcloud.com/s4n7r0)
[Twitter](https://www.twitter.com/s4n7r0)
[Bandcamp](https://s4n7r0.bandcamp.com/)
