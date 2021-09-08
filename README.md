# FaceFXWrapper (32-bit)

A utility for taking dialogue (.wav + text) meant for TES/Fallout games and generating .lip files as the native Creation Kit would do.

## Basic Usage

---

LIP generation with automatic resampling of the source wav file data:

```
FaceFXWrapper [Lang] [FonixDataPath] [WavPath] [ResampledWavPath] [LipPath] [Text]
FaceFXWrapper USEnglish FonixData.cdf c00jorrvaskrfight__000bd639_1.wav resampled.wav output_1.lip "My special sentence"

ResampledWavPath and LipPath are paths for output files. They may be relative or absolute. All other parameters are inputs.
```

LIP generation with manually supplied resampled wav file data (skipping resample stage):

```
FaceFXWrapper [Lang] [FonixDataPath] [ResampledWavPath] [LipPath] [Text]
FaceFXWrapper USEnglish FonixData.cdf my_precreated_resampled.wav output_2.lip "My special sentence"

LipPath is the path for the output file. It may be relative or absolute. All other parameters are inputs. FaceFXWrapper expects a 16khz 16 bit-per-sample single-channel voice file.
```

## Requirements

---

`FonixData.cdf` is not provided with this tool and must be obtained from the G.E.C.K. or Creation Kit. It's generally included under `\Data\Sound\Voice\Processing\` in the editor files. Alternatively, you can obtain it from [here](https://www.nexusmods.com/newvegas/mods/61248/) or [here](https://www.nexusmods.com/skyrimspecialedition/mods/40971).

## Supported games

---

- TES: Skyrim
- TES: Skyrim Special Edition
- Fallout 3
- Fallout NV

## Libraries

---

- [MemoryModule](https://github.com/fancycode/MemoryModule) ([License](https://github.com/fancycode/MemoryModule/blob/master/LICENSE.txt))
- [STB](https://github.com/nothings/stb) ([License](https://github.com/nothings/stb/blob/master/LICENSE))

## License

---

FaceFXWrapper uses code from the [Creation Kit](https://www.creationkit.com/index.php) and is subject to Bethesda's license agreement. Resource files are property of Bethesda Softworks LLC.