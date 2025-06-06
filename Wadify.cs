/*/
 *
 * this tool was made for T6_greenlight_mp
 * it also supports all versions of T6
 * it *should* support all versions of T5 too, however it's very untested
 *  keyword: should
 *
 * this tool also only support's xbox 360 (ps3, wii u and pc are untested but should work for them)
 * 
 * for ps3, wii u and pc:
 *  i will need to get my hands on a wad for these platforms to properly confirm...
 *  however this likely won't happen
 *  interestingly the ones in the dedicated server files are little endian
 *  however this is probably because those server's are run on PC
 *
/*/

using System;
using System.IO;
using System.Collections.Generic;

using static Utils.Structs;
using static Utils.Reader;
using static Utils.Writer;

namespace Wadify
{
	class Wadify
	{
		static void UnlinkWAD(string FileName)
		{
			// tell the user what we are unlinking
			Utils.Print.WriteMessage($"\nUnlinking: {FileName}..\n");

			try
			{
				List<WADEntry> Entries = ProcessOnlineWAD(File.ReadAllBytes(FileName));
				if (Entries == null)
				{
					Utils.Print.WriteError($"{FileName}'s entry's were NULL!");
					return;
				}

				string OutputDirectory = Path.GetFullPath(
					Path.GetFileNameWithoutExtension(GetFilename(FileName)));

				CreateOutputDirectory(OutputDirectory);
				UnlinkEntries(Entries, File.ReadAllBytes(FileName), OutputDirectory);

				Utils.Print.WriteGreenMessage("\nDone !");
			}
			catch (Exception Message)
			{
				Utils.Print.WriteExceptionError(
					$"Failed to unlink: {FileName}!",
					Message.Message);
				return;
			}
		}

		static void LinkWAD(string FolderName)
		{
			// tell the user what we are linking
			Utils.Print.WriteMessage($"\nLinking: {FolderName}..\n");

			try
			{
				using (var Stream = new MemoryStream())
				using (var Writer = new BinaryWriter(Stream))
				{
					WAD WADFile = WriteOnlineWAD(Directory.GetFiles(FolderName));
					WriteWADHeader(Writer, WADFile.header);
					WriteWADEntries(Writer, WADFile.entries);
					WriteCompressedData(Writer, WADFile.entries);

					// okay let's save the file now
					File.WriteAllBytes(
						Path.GetFileName(FolderName) + ".wad",
						Stream.ToArray());

					Utils.Print.WriteGreenMessage("\nDone !");
				}
			}
			catch (Exception Message)
			{
				Utils.Print.WriteExceptionError(
					$"Failed to link: {FolderName}!",
					Message.Message);
				return;
			}
		}

		static void Unlink(string[] Parameters)
		{
			// first check if there are any parameters
			if (Parameters.Length < 2)
			{
				Utils.Print.WriteUsageWarning($"{Parameters[0]} <input.wad>");
				return;
			}

			// small check to see if it's actually a .wad file
			if (!Parameters[1].EndsWith(".wad", StringComparison.OrdinalIgnoreCase))
			{
				Utils.Print.WriteWarning(
					"WARNING :: tried to unlink a non .wad file!");
				Utils.Print.WriteWarning(
					"        :: add .wad extension to your command or check your file name.");
				return;
			}

			// okay all good, unlink it now
			UnlinkWAD(Parameters[1]);
		}

		static void Link(string[] Parameters)
		{
			// first check if there are any parameters
			if (Parameters.Length < 2)
			{
				Utils.Print.WriteUsageWarning($"{Parameters[0]} <input folder>");
				return;
			}

			if (Parameters[1].EndsWith(".wad", StringComparison.OrdinalIgnoreCase))
			{
				Utils.Print.WriteWarning(
					"WARNING :: trying to link an already linked .wad file!");
				Utils.Print.WriteWarning(
					"        :: remove the .wad extension from your command or check your file name.");
				return;
			}

			// okay all good, link it now
			LinkWAD(Parameters[1]);
		}

		static void Help()
		{
			// just general help for the tool

			Utils.Print.WriteMessage(
				"command usages:");
			Utils.Print.WriteMessage(
				"--unlink   <input .wad>   ::  unlinks the inputted .wad file.");
			Utils.Print.WriteMessage(
				"  shortcut                :: -u");
			Utils.Print.WriteMessage(
				"--link     <input folder> ::  links the inputted folder into a .wad file.");
			Utils.Print.WriteMessage(
				"  shortcut                :: -l");
			Utils.Print.WriteMessage(
				"--help                    ::  displays help for various commands.");
			Utils.Print.WriteMessage(
				"  shortcut                :: -h");
			Utils.Print.WriteMessage(
				"--about                   ::  displays information about this tool.");
			Utils.Print.WriteMessage(
				"  shortcut                :: -a");
		}

		static void About()
		{
			Utils.Print.WriteMessage("tool information:");
			Utils.Print.WriteMessage("wadify.exe :: a linker / unlinker tool for 3arc's .wad file type");
			Utils.Print.WriteMessage("           :: made by hindercanrun");
		}

		static void Main(string[] Parameters)
		{
			// first check if there are any parameters
			if (Parameters.Length < 1)
			{
				Utils.Print.WriteUsageWarning("wadify.exe <command>");
				return;
			}

			// now check what the user wants to do

			switch (Parameters[0])
			{
				case "-u":
				case "--unlink":
					Unlink(Parameters);
					break;
				case "-l":
				case "--link":
					Link(Parameters);
					break;
				case "-?":
				case "-h":
				case "--help":
					Help();
					break;
				case "-a":
				case "--about":
					About();
					break;
				default:
					Utils.Print.WriteError($"unknown command: {Parameters[0]}!");
					break;
			}
		}
	}
}