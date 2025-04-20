using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using ICSharpCode.SharpZipLib.Zip.Compression;
using ICSharpCode.SharpZipLib.Zip.Compression.Streams;

using static Utils.Structs;
using static Utils.EndiannessReader;

namespace Utils
{
	class Writer
	{
		//
		// unlinks the entries inside a .wad file
		//
		public static void UnlinkEntries(List<WADEntry> Entries, byte[] Bytes, string OutputDirectory)
		{
			foreach (WADEntry Entry in Entries)
			{
				// tell the user what we are extracting..
				Print.WriteMessage($"Extracting: {Entry.name}..");

				try
				{
					byte[] CompressedData = new byte[Entry.compressedSize];
					Array.Copy(Bytes, Entry.offset, CompressedData, 0, Entry.compressedSize);

					byte[] DecompressedData = DecompressFile(CompressedData);
					string outputPath = Path.Combine(OutputDirectory, Entry.name);
					File.WriteAllBytes(outputPath, DecompressedData);
				}
				catch (Exception Message)
				{
					Print.WriteExceptionError(
						$"Failed to unlink: {Entry.name}!",
						Message.Message);
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
				ffotdVersion = 0 // it's like always 0 or 1
								// pre-release: 0
								// post-release: 1
			};

			List<WADEntry> Entries = new List<WADEntry>();
			uint Offset = 16 + (Header.numEntries * 44);

			foreach (string Name in FileNames)
			{
				string FileName = Path.GetFileName(Name);
				Print.WriteMessage($"Compressing {FileName}..");

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
		public static WADEntry CreateWADEntry(string FilePath, string FileNameOnly, uint Offset)
		{
			byte[] DecompressedBuf = File.ReadAllBytes(FilePath);
			byte[] CompressedBuf = CompressFile(DecompressedBuf);

			return new WADEntry
			{
				name = FileNameOnly,
				compressedBuf = CompressedBuf,
				compressedSize = (uint)CompressedBuf.Length,
				offset = Offset,
				size = (uint)DecompressedBuf.Length
			};
		}

		//
		// writes the full header
		//
		public static void WriteWADHeader(BinaryWriter Writer, WADHeader Header)
		{
			Writer.Write(ReverseEndianUInt32(Header.magic));
			Writer.Write(ReverseEndianUInt32(Header.timestamp));
			Writer.Write(ReverseEndianUInt32(Header.numEntries));
			Writer.Write(ReverseEndianUInt32(Header.ffotdVersion));
		}

		//
		// writes the entries
		//
		public static void WriteWADEntries(BinaryWriter Writer, List<WADEntry> Entries)
		{
			foreach (WADEntry Entry in Entries)
			{
				//write the name
				Writer.Write(Encoding.ASCII.GetBytes(Entry.name));
				WritePadding(Writer, Entry.name.Length);

				//write the compressed size, entry size and offset
				Writer.Write(ReverseEndianUInt32(Entry.compressedSize));
				Writer.Write(ReverseEndianUInt32(Entry.size));
				Writer.Write(ReverseEndianUInt32(Entry.offset));
			}
		}

		//
		// writes compressed data
		//
		public static void WriteCompressedData(BinaryWriter Writer, List<WADEntry> Entries)
		{
			foreach (WADEntry Entry in Entries)
				Writer.Write(Entry.compressedBuf);
		}

		//
		// appends a padding containing 00
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

		//
		// compresses the inputted file
		//
		public static byte[] CompressFile(byte[] FileName)
		{
			using (var OutputStream = new MemoryStream())
			{
				using (
					var Deflate = new DeflaterOutputStream(
						OutputStream, new Deflater(Deflater.DEFAULT_COMPRESSION, false)))
				{
					Deflate.Write(FileName, 0, FileName.Length);
					Deflate.Finish();
				}

				return OutputStream.ToArray();
			}
		}

		//
		// decompresses the inputted file
		//
		public static byte[] DecompressFile(byte[] FileName)
		{
			using (var Stream = new MemoryStream(FileName))
			using (var Inflate = new InflaterInputStream(Stream, new Inflater(false)))
			using (var OutputStream = new MemoryStream())
			{
				Inflate.CopyTo(OutputStream);
				return OutputStream.ToArray();
			}
		}
	}
}

