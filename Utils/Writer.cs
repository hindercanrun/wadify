using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Ionic.Zlib;

using static Utils.Structs;
using static Utils.EndiannessReader;

namespace Utils
{
    class Writer
    {
        //
        // unlinks the entries inside a .wad file
        //
        public static void UnlinkEntries(List<WADEntry> WADEntries, byte[] Bytes, string OutputDirectory)
        {
            foreach (WADEntry WADEntry in WADEntries)
            {
                // tell the user what we are extracting..
                Console.WriteLine($"Extracting: {WADEntry.name}..");

                try
                {
                    //extract the compressed data
                    byte[] CompressedData = new byte[WADEntry.compressedSize];

                    //okay copy it then write it
                    Array.Copy(Bytes, WADEntry.offset, CompressedData, 0, WADEntry.compressedSize);
                    File.WriteAllBytes(Path.Combine(OutputDirectory, WADEntry.name),
                        ZlibStream.UncompressBuffer(CompressedData));
                }
                catch (Exception MSG)
                {
                    //bad!
                    Console.WriteLine($"{WADEntry.name} failed to extract!");
                    Console.WriteLine($"Reason: {MSG.Message}");
                    return;
                }
            }
        }

        //
        // Writes a WAD file
        //
        public static WAD WriteOnlineWAD(string[] FileNames)
        {
            WADHeader Header = new WADHeader
            {
                magic = 0x543377AB,
                timestamp = (uint)DateTimeOffset.UtcNow.ToUnixTimeSeconds(),
                numEntries = (uint)FileNames.Length,
                ffotdVersion = 0 // it's like always 0 (pre-release atleast, only goes to 1 in post-release)
            };

            List<WADEntry> Entries = new List<WADEntry>();
            uint Offset = 16 + (Header.numEntries * 44);

            foreach (string Name in FileNames)
            {
                string FileName = Path.GetFileName(Name);
                Console.WriteLine($"Compressing {FileName}..");

                WADEntry Entry = CreateWADEntry(Name, FileName, Offset);
                Entries.Add(Entry);

                Offset += Entry.compressedSize;
            }

            return new WAD
            {
                header = Header,
                entries = Entries
            };
        }

        //
        // creates a WAD entry
        //
        public static WADEntry CreateWADEntry(string Path, string fileNameOnly, uint offset)
        {
            byte[] DecompressedBuf = File.ReadAllBytes(Path);
            byte[] CompressedBuf = ZlibStream.CompressBuffer(DecompressedBuf);

            return new WADEntry
            {
                name = fileNameOnly,
                compressedBuf = CompressedBuf,
                compressedSize = (uint)CompressedBuf.Length,
                offset = offset,
                size = (uint)DecompressedBuf.Length
            };
        }

        //
        // writes the header to the file
        //
        public static void WriteWADHeader(BinaryWriter Writer, WADHeader Header)
        {
            Writer.Write(ReverseEndianUInt32(Header.magic));
            Writer.Write(ReverseEndianUInt32(Header.timestamp));
            Writer.Write(ReverseEndianUInt32(Header.numEntries));
            Writer.Write(ReverseEndianUInt32(Header.ffotdVersion));
        }

        //
        // writes the entries to the file
        //
        public static void WriteWADEntries(BinaryWriter Writer, List<WADEntry> Entries)
        {
            foreach (WADEntry Entry in Entries)
            {
                //write the name
                Writer.Write(Encoding.ASCII.GetBytes(Entry.name));
                WritePadding(Writer, Entry.name.Length);

                //write the compressed size, size and offset
                Writer.Write(ReverseEndianUInt32(Entry.compressedSize));
                Writer.Write(ReverseEndianUInt32(Entry.size));
                Writer.Write(ReverseEndianUInt32(Entry.offset));
            }
        }

        //
        // writes compressed data to the file
        //
        public static void WriteCompressedData(BinaryWriter Writer, List<WADEntry> WADEntries)
        {
            foreach (WADEntry Entry in WADEntries)
                Writer.Write(Entry.compressedBuf);
        }

        //
        // writes a padding to the file
        //
        public static void WritePadding(BinaryWriter Writer, int NameLength)
        {
            int PaddingLength = 32 - NameLength;
            for (int Index = 0; Index < PaddingLength; Index++)
                Writer.Write((byte)0);
        }

        //
        // checks if a directory exists, if not it creates it
        //
        public static void CreateOutputDirectory(string FolderName)
        {
            if (!Directory.Exists(FolderName))
                Directory.CreateDirectory(FolderName);
        }
    }
}
