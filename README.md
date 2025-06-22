<div align="center">

# a compresser/decompresser for Treyarch's .wad type.

![downloads](https://img.shields.io/github/downloads/hindercanrun/wadify/total?style=flat-square&label=Total%20Downloads&labelColor=F3F8FF&color=F88379)
![latest release](https://img.shields.io/github/v/tag/hindercanrun/wadify?filter=!v*-pre&style=flat-square&label=Latest%20Release&labelColor=F3F8FF&color=F88379)
![release date](https://img.shields.io/github/release-date-pre/hindercanrun/wadify?style=flat-square&label=Release%20Date&labelColor=F3F8FF&color=F88379)
![code size](https://img.shields.io/github/languages/code-size/hindercanrun/wadify?style=flat-square&label=Code%20Size&labelColor=F3F8FF&color=F88379)
</div>

![title_image](https://github.com/user-attachments/assets/2e975015-e4c3-47b2-ab86-994ba699aaab)

---

## Installation & Usage
1. Download the [latest version](https://github.com/hindercanrun/wadify/releases/latest/download/wadify.exe).
2. Drag and drop it to your folder of choice.
3. Open up Command Prompt and type in `cd <your folder of choice>`.
4. Type `wadify` and now your command of choice. Here are a few examples.
   - `--decompress <wad>` if you want to decompress a wad.
   - `--compress <folder>` if you want to compress a folder.
   - If you need any help, type `--help` for all the commands.
5. Done.

## Building From Source
1. **Clone** the Git repository or **download** as .ZIP and extract it to your desired location.
2. **Open** up `wadify.sln` in **Visual Studio** (I use Visual Studio 2022).
   - Your build platform doesn't matter, so either `x64` or `x86` will work.
   - Your build config doesn't matter too, so either `Debug` or `Release` will work.
3. **Build** the solution and it should build `wadify.exe`.
4. Done.

## Command Line Arguments

- ```--decompress```, ```-d```
  - Decompresses the input .wad.
- ```--compress```, ```-c```
  - Compresses the input folder into a .wad.
- ```--output-folder```, ```-o```
  - Specifies the output folder.
- ```--help```, ```-h```, ```?```
  - Displays help for various commands.
- ```--about```, ```-a```
  - Displays about information.

##### Example Usage
```cmd
wadify.exe --decompress online_tu0_mp_english.wad
```

## Documentation:

For anyone interested in learning about this file type, I have put some documentation [here](https://github.com/hindercanrun/wad/blob/main/Docs/WadFile.md) about it.

---

> [!IMPORTANT]
> **This tool has been created purely for the purposes of research.**
