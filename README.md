<div align="center">

# a decompresser/compresser for Treyarch's .wad container files.

![latest release](https://img.shields.io/github/v/tag/hindercanrun/wadify?filter=!v*-pre&style=flat-square&label=Latest%20Release&labelColor=F3F8FF&color=F88379)
![release date](https://img.shields.io/github/release-date-pre/hindercanrun/wadify?style=flat-square&label=Release%20Date&labelColor=F3F8FF&color=F88379)
![size](https://img.shields.io/github/languages/code-size/hindercanrun/wadify?style=flat-square&label=Code%20Size&labelColor=F3F8FF&color=F88379)
![downloads](https://img.shields.io/github/downloads/hindercanrun/wadify/total?style=flat-square&label=Total%20Downloads&labelColor=F3F8FF&color=F88379)
</div>

---

## Installation & Usage
1. Download the [latest version](https://github.com/hindercanrun/wadify/releases/latest/download/wadify.exe).
2. Drag and drop it to your folder of choice.
3. Open up Command Prompt and type in `cd <your folder of choice>`.
4. Type `wadify` and now your command of choice.
   - `--decompress <wad>` if you want to decompress a wad.
   - `--compress <folder>` if you want to compress a folder.
   - If you need any help, type `--help` for all the commands.
5. Done.

---

## Command line arguments

- ```--decompress```, ```-d```
  - Decompresses the input .wad.
- ```--compress```, ```-c```
  - Compresses the input folder into a .wad.
- ```--help```, ```-h```, ```-?```
  - Displays help for various commands.
- ```--about```, ```-a```
  - Displays about information.

##### Example
```cmd
wadify.exe --decompress online_tu0_mp_english.wad
```

---

## Documentation:

For anyone interested in learning about the file type, I have put some documentation [here](https://github.com/hindercanrun/wad/blob/main/Docs/WadFile.md) about it.

---

> [!IMPORTANT]
> **This tool has been created purely for the purposes of research.**
