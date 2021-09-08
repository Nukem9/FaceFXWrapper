# FaceFXWrapper

A utility to generate native LIP files for TES/Fallout games without using or installing the [Creation Kit](https://www.creationkit.com/index.php).

## Basic Usage

| Argument | Possible value |
| -------- | -------------- |
Type | Skyrim, Fallout4
Lang | USEnglish
FonixDataPath | FonixData.cdf
WavPath | Source audio file
ResampledWavPath | Resampled source audio file
LipPath | Path for generated LIP file
Text | Dialogue in text form

\
LIP generation with automatic resampling of the source wav file data:

```
FaceFXWrapper [Type] [Lang] [FonixDataPath] [WavPath] [ResampledWavPath] [LipPath] [Text]
FaceFXWrapper Skyrim USEnglish FonixData.cdf c00jorrvaskrfight__000bd639_1.wav resampled.wav output_1.lip "My special sentence"

ResampledWavPath and LipPath are paths for output files. They may be relative or absolute. All other parameters are inputs.
```

LIP generation with manually supplied resampled wav file data (skipping resample stage):

```
FaceFXWrapper [Type] [Lang] [FonixDataPath] [ResampledWavPath] [LipPath] [Text]
FaceFXWrapper Fallout4 USEnglish FonixData.cdf my_precreated_resampled.wav output_2.lip "My special sentence"

LipPath is the path for the output file. It may be relative or absolute. All other parameters are inputs. FaceFXWrapper expects a 16khz 16 bit-per-sample single-channel voice file.
```

## Requirements

`FonixData.cdf` is not provided with this tool and must be obtained from the G.E.C.K. or Creation Kit. It's generally included under `\Data\Sound\Voice\Processing\` in the editor files. Alternatively, you can obtain it from [here](https://www.nexusmods.com/newvegas/mods/61248/) or [here](https://www.nexusmods.com/skyrimspecialedition/mods/40971).

## Supported games

- TES: Skyrim
- TES: Skyrim Special Edition
- Fallout 3
- Fallout NV
- Fallout 4

## Libraries

- [MemoryModule](https://github.com/fancycode/MemoryModule) ([License](https://github.com/fancycode/MemoryModule/blob/master/LICENSE.txt))
- [STB](https://github.com/nothings/stb) ([License](https://github.com/nothings/stb/blob/master/LICENSE))

## License

FaceFXWrapper uses code from the [Creation Kit](https://www.creationkit.com/index.php) and is subject to Bethesda's license agreement. Compressed [resource files](/resources) are property of Bethesda Softworks LLC. All other code is subject to the [license](LICENSE.md).
