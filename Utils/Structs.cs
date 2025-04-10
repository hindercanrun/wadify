using System;
using System.Collections.Generic;

namespace Utils
{
    class Structs
    {
        // these structs were taken from retail T6 due to no structs in T6_greenlight_mp
        //

        public struct WADHeader
        {
            public UInt32 magic;
            public UInt32 timestamp;
            public UInt32 numEntries;
            public UInt32 ffotdVersion;
        }

        public struct WADEntry
        {
            public string name;
            public UInt32 compressedSize;
            public UInt32 size;
            public UInt32 offset;
            public byte[] compressedBuf;//nonexistant in T5 - it may also not exist in pre-release T6
        }

        public struct WAD
        {
            public WADHeader header;
            public List<WADEntry> entries;
        }
    }
}
