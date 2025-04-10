using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

using static Utils.Structs;
using static Utils.EndianessReader;

namespace Utils
{
    class EndianessReader : BinaryReader
    {
        public enum Endianness
        {
            Little,
            Big,
        }

        private Endianness _Endianness = Endianness.Little;

        public EndianessReader(
            Stream Input)
            : base(Input)
        {
        }

        public EndianessReader(
            Stream Input, Encoding Encoding)
            : base(Input, Encoding)
        {
        }

        public EndianessReader(
            Stream Input, Encoding Encoding,
            bool LeaveOpen)
            : base(Input, Encoding, LeaveOpen)
        {
        }

        public EndianessReader(
            Stream Input,
            Endianness Endianness)
            : base(Input)
        {
            _Endianness = Endianness;
        }

        public EndianessReader(
            Stream Input, Encoding Encoding,
            Endianness Endianness)
            : base(Input, Encoding)
        {
            _Endianness = Endianness;
        }

        public EndianessReader(
            Stream Input, Encoding Encoding, bool LeaveOpen,
            Endianness Endianness)
            : base(Input, Encoding, LeaveOpen)
        {
            _Endianness = Endianness;
        }

        public void SetEndianness(Endianness Endianness)
        {
            _Endianness = Endianness;
        }

        public override short ReadInt16()
            => ReadInt16(_Endianness);

        public override int ReadInt32()
            => ReadInt32(_Endianness);

        public override long ReadInt64()
            => ReadInt64(_Endianness);

        public override ushort ReadUInt16()
            => ReadUInt16(_Endianness);

        public override uint ReadUInt32()
            => ReadUInt32(_Endianness);

        public override ulong ReadUInt64()
            => ReadUInt64(_Endianness);

        public override bool ReadBoolean()
            => ReadBoolean(_Endianness);

        public override byte ReadByte()
            => base.ReadByte();

        public override string ReadString()
            => ReadString(0, _Endianness);//uhh idk

        public override char ReadChar()
            => ReadChar(_Endianness);

        public float ReadFloat()
            => ReadFloat(_Endianness);

        public override double ReadDouble()
            => ReadDouble(_Endianness);

        public short ReadInt16(Endianness Endianness)
            => BitConverter.ToInt16(
                ReadForEndianness(sizeof(short),
                Endianness), 0);

        public int ReadInt32(Endianness Endianness)
            => BitConverter.ToInt32(
                ReadForEndianness(sizeof(int),
                Endianness), 0);

        public long ReadInt64(Endianness Endianness)
            => BitConverter.ToInt64(
                ReadForEndianness(sizeof(long),
                Endianness), 0);

        public ushort ReadUInt16(Endianness Endianness)
            => BitConverter.ToUInt16(
                ReadForEndianness(sizeof(ushort),
                Endianness), 0);

        public uint ReadUInt32(Endianness Endianness)
            => BitConverter.ToUInt32(
                ReadForEndianness(sizeof(uint),
                Endianness), 0);

        public ulong ReadUInt64(Endianness Endianness)
            => BitConverter.ToUInt64(
                ReadForEndianness(sizeof(ulong),
                Endianness), 0);

        public bool ReadBoolean(Endianness Endianness)
            => BitConverter.ToBoolean(
                ReadForEndianness(sizeof(bool),
                Endianness), 0);

        public string ReadString(int Length, Endianness Endianness)
            => BitConverter.ToString(
                ReadForEndianness(Length,
                Endianness));

        public char ReadChar(Endianness Endianness)
            => BitConverter.ToChar(
                ReadForEndianness(sizeof(char),
                Endianness), 0);

        public float ReadFloat(Endianness Endianness)
            => BitConverter.ToSingle(
                ReadForEndianness(sizeof(float),
                Endianness), 0);

        public double ReadDouble(Endianness Endianness)
            => BitConverter.ToDouble(
                ReadForEndianness(sizeof(double),
                Endianness), 0);

        private byte[] ReadForEndianness(int BytesToRead, Endianness Endianness)
        {
            var BytesRead = ReadBytes(BytesToRead);
            if ((Endianness == Endianness.Little && !BitConverter.IsLittleEndian)
                || (Endianness == Endianness.Big && BitConverter.IsLittleEndian))
            {
                Array.Reverse(BytesRead);
            }

            return BytesRead;
        }

        public static UInt32 ReverseEndianUInt32(UInt32 Value)
        {
            return ((Value & 0x000000FF) << 24) |
                   ((Value & 0x0000FF00) << 8) |
                   ((Value & 0x00FF0000) >> 8) |
                   ((Value & 0xFF000000) >> 24);
        }
    }

    class Reader
    {
        private const int WADMagic = 0x543377AB; // T3w«

        private static EndianessReader _Reader;

        //
        // processes the inputted .wad file
        //
        public static List<WADEntry> ProcessOnlineWAD(byte[] WADBytes)
        {
            using (var Stream = new MemoryStream(WADBytes))
            using (_Reader = new EndianessReader(Stream, Endianness.Big))
            {
                WADHeader Header = ReadWADHeader();
                // check the magic
                if (Header.magic != WADMagic)
                {
                    Console.WriteLine(
                        $"WAD has incorrect magic! Expecting: 0x{WADMagic:X8}, got: 0x{Header.magic:X8}!");
                    return null;
                }

                // your .wad is good if you get here

                //convert the timestamp to a readable format
                DateTimeOffset TimeOffset = DateTimeOffset.FromUnixTimeSeconds(Header.timestamp);
                DateTime Time = TimeOffset.UtcDateTime;

                //some misc .wad information
                Console.WriteLine($"WAD Information:");
                Console.WriteLine($"Magic: 0x{Header.magic:X8}");
                Console.WriteLine($"Timestamp: {Time:HH:mm:ss, dd/MM/yyyy} ({Header.timestamp:X8})");
                Console.WriteLine($"Number of Entries: {Header.numEntries}");
                Console.WriteLine($"FFOTD Version: {Header.ffotdVersion}");

                Console.WriteLine($"\nExtracting files..\n");

                //time to read the entries
                List<WADEntry> Entries = new List<WADEntry>();
                for (int Index = 0; Index < Header.numEntries; Index++)
                    Entries.Add(ReadWADEntry(WADBytes, Index));

                return Entries;
            }
        }

        //
        // reads the .wad header
        //
        public static WADHeader ReadWADHeader()
        {
            WADHeader Header = new WADHeader
            {
                magic = _Reader.ReadUInt32(),
                timestamp = _Reader.ReadUInt32(),
                numEntries = _Reader.ReadUInt32(),
                ffotdVersion = _Reader.ReadUInt32()
            };

            return Header;
        }

        //
        // reads the .wad entries
        //
        public static WADEntry ReadWADEntry(byte[] WADBytes, int Index)
        {
            const int EntryDataSize = 44;// WADEntry struct size

            byte[] Data = new byte[EntryDataSize];
            Array.Copy(WADBytes, 16 + (EntryDataSize * Index), Data, 0, Data.Length);

            using (MemoryStream _Stream = new MemoryStream(Data))
            using (BinaryReader _Reader = new BinaryReader(_Stream))
            {
                WADEntry Entry = new WADEntry
                {
                    name = ReadEntryName(_Reader),
                    compressedSize = ReverseEndianUInt32(_Reader.ReadUInt32()),
                    size = ReverseEndianUInt32(_Reader.ReadUInt32()),
                    offset = ReverseEndianUInt32(_Reader.ReadUInt32())
                };

                Entry.compressedBuf = new byte[Entry.compressedSize];
                return Entry;
            }
        }

        //
        // reads a 32-bit integer from the reader
        //
        public static string ReadEntryName(BinaryReader _Reader)
        {
            return Encoding.ASCII.GetString(_Reader.ReadBytes(32)).Trim('\0');
        }

        //
        // gets the filename from the specified path
        //
        public static string GetFilename(string FileName)
        {
            return Path.GetFileName(FileName);
        }

        //
        // swaps the endian of a byte array
        //
        public static void SwapEndian(byte[] FileData)
        {
            byte[] TargetSequence = new byte[] // no little endian for now, sorry!
            {
                0x54, 0x33, 0x77, 0xAB // T3w«
            };

            for (int Index = 0; Index <= FileData.Length - TargetSequence.Length; Index++)
            {
                //check if it's a match
                if (IsMatching(FileData, TargetSequence, Index))
                {
                    //now swap the endian
                    Array.Reverse(FileData, Index, TargetSequence.Length);
                    Index += TargetSequence.Length - 1; //skip past this sequence
                }
            }
        }

        //
        // checks if a sequence of bytes matches another sequence
        //
        public static bool IsMatching(byte[] FileData, byte[] TargetSequence, int StartIndex)
        {
            for (int Index = 0; Index < TargetSequence.Length; Index++)
                if (FileData[StartIndex + Index] != TargetSequence[Index])
                    return false;
            return true;
        }
    }
}
