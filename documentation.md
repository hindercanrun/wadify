# wad documentation
just some basic information that I learnt while reversing this type.

name: 'Where's All the Data' - this isn't the offical name, nobody knows what it is so it's just an assumption but knowing 3arc, this is probably what it's called.

description/usage: a wad is a file container, the point of this file is to hold several online things like contracts, codtv information, playlists, etc. in short: no wad found? no online play for you.

now, let's dive into the actual file structure. wad's don't really have a file system like directories, it's all in the "raw/main".

I have used `online_tu0_mp_english.wad` from `T6_greenlight_mp` build for this document.

```
magic
string: T3w« hex: 54 33 77 AB (big endian)
string: «w3T hex: AB 77 33 54 (little endian)
```
![image](https://github.com/user-attachments/assets/e6ba742c-3890-4cb9-8331-43519243a32c)

now, we have the timestamp.

![image](https://github.com/user-attachments/assets/c002ad3d-1dd4-472e-b762-21f864a24568)

in my instance, it was this.

![image](https://github.com/user-attachments/assets/0d0a247d-8a8a-4bc5-b3bd-0c05610d7f21)

now we have the number of entries.

![image](https://github.com/user-attachments/assets/a1cb9920-208b-4d0e-87f1-dd0a2627dbb9)

which was three from this wad. remember each wad can have a different timestamp and number of entries

![image](https://github.com/user-attachments/assets/aa8256ad-afe5-46f6-b266-19c327952d4c)

after the number of entries is the ffotd version. this is always 0 for pre-release wad's but set to 1 for post-release wad's.

This screenshot is from `rfazio/t6/wads/online_tu15_mp_english.wad`, as you can see it has ffotd version set to 1. `rfazio build` version is from `2013`, so post release. `T6_greenlight_mp` has ffotd version of `0`. this ffotd version is completely seperate from the one the game uses.

![image](https://github.com/user-attachments/assets/04da48b9-1700-4b67-b338-b76457bc729d)

now, we have the entries themselves. remember this can be different depending on your game mode's and game version's wad.

![image](https://github.com/user-attachments/assets/f87f19de-fead-439b-b3d0-cdfe7e34e34a)

after the entry name, there is size and then `00` padding.

after this, you have reached the "end" of the readable by human's section.

![image](https://github.com/user-attachments/assets/1de3b25b-9b53-436d-af67-f3e202bb6a72)

the rest is pointless to go through as it's just compressed with basic zlib, nothing interesting.

![image](https://github.com/user-attachments/assets/6cf6cdc9-fe8e-43a1-bc29-eca317c6d20a)

and just like that, you've learnt pretty much this entire format, atleast the basics of it.

things to note:
- endianness impacts the file
- 3arc used a linker that pulled from a zone_source type folder to link together the assets
- if your wad is at all invalid, then you cannot play online at all

the end!
