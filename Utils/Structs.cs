using System;
using System.Collections.Generic;

/*/
 *
 * just a notice:
 *  these structs were taken from retail T6
 *  due to there being no public CoDMP.pdb in T6_greenlight_mp
 *  and i can't be bothered to fully reverse them
 *  it doesn't matter anyways, because the structs are 1:1
 *  the only new addition is compressedBuf in WADEntry
 *
/*/

namespace Utils
{
	class Structs
	{
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
			public byte[] compressedBuf;//missing in T5
		}

		public struct WAD
		{
			public WADHeader header;
			public List<WADEntry> entries;
		}
	}
}
