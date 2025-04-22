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
	internal class Structs
	{
		internal struct WADHeader
		{
			internal UInt32 magic;
			internal UInt32 timestamp;
			internal UInt32 numEntries;
			internal UInt32 ffotdVersion;
		}

		internal struct WADEntry
		{
			internal string name;
			internal UInt32 compressedSize;
			internal UInt32 size;
			internal UInt32 offset;
			internal byte[] compressedBuf;//missing in T5
		}

		internal struct WAD
		{
			internal WADHeader header;
			internal List<WADEntry> entries;
		}
	}
}
