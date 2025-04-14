/*/
 *
 * this tool was made for T6_greenlight_mp
 * it also supports all versions of T6
 * it *should* support all versions of T5 too, however it's very untested
 * --keyword: should
 *
 * it also only support's xbox 360 (ps3 and wii u are untested but should work for them)
 * pc is unsupported however you *can* unlink pc wad's, you just have to swap around the magic's bytes
 * you cannot link wad's for pc, you could maybe switch around the bytes but that probably won't work
 * i am not entirely sure if the game can load big endian wad's, maybe it can but i'm not sure
 * 
 * for wii u and ps3:
 *  i will need to get my hands on a ps3 and wii u wad to properly confirm...
 *  however this likely won't happen
 *  the ones in the dedicated server are little endian/pc one's
 *
/*/

using System;
using System.IO;
using System.Collections.Generic;

using static Utils.Structs;
using static Utils.Reader;
using static Utils.Writer;

namespace Wad
{
    class Wad
    {
        static void UnlinkWAD(string FileName)
        {
            // tell the user what .wad we are unlinking
            Console.WriteLine($"Unlinking: {FileName}..\n");

            try
            {
                List<WADEntry> Entries = ProcessOnlineWAD(File.ReadAllBytes(FileName));
                if (Entries != null)
                {
                    string OutputDirectory = Path.Combine(
                        ".", Path.GetFileNameWithoutExtension(GetFilename(FileName)));
                    // check if the output directory exists
                    CreateOutputDirectory(OutputDirectory);

                    UnlinkEntries(Entries, File.ReadAllBytes(FileName), OutputDirectory);

                    Console.WriteLine("\nDone!");
                }
            }
            catch (Exception MSG)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"ERROR  :: Failed to unlink: {FileName}!");
                Console.WriteLine($"REASON :: {MSG.Message}");
                Console.ResetColor();
                return;
            }
        }

        static void LinkWAD(string FolderName)
        {
            // tell the user what .wad we are linking
            Console.WriteLine($"Linking: {FolderName}..\n");

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
                        Path.GetFileName(FolderName) + ".wad", Stream.ToArray());

                    Console.WriteLine("\nDone!");
                }
            }
            catch (Exception MSG)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"ERROR  :: Failed to link: {FolderName}!");
                Console.WriteLine($"REASON :: {MSG.Message}");
                Console.ResetColor();
                return;
            }
        }

        static void Unlink(string[] Parameters)
        {
            // first check if there are any parameters
            if (Parameters.Length < 2)
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine($"USAGE :: {Parameters[0]} <input.wad>");
                Console.ResetColor();
                return;
            }

            // small check to see if it's actually a .wad file
            if (!Parameters[1].EndsWith(".wad", StringComparison.OrdinalIgnoreCase))
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine(
                    "WARNING :: tried to unlink a non .wad file!");
                Console.WriteLine(
                    "        :: you might be trying to unlink an already unlinked .wad.");
                Console.WriteLine(
                    "        :: if not, add .wad extension to your command or check your file name.");
                Console.ResetColor();
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
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine($"USAGE :: {Parameters[0]} <input folder>");
                Console.ResetColor();
                return;
            }

            if (Parameters[1].EndsWith(".wad", StringComparison.OrdinalIgnoreCase))
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine(
                    "WARNING :: trying to link an already linked .wad file!");
                Console.WriteLine(
                    "        :: if not, remove the .wad extension from your command or check your file name.");
                Console.ResetColor();
                return;
            }

            // okay all good, link it now
            LinkWAD(Parameters[1]);
        }

        static void Help()
        {
            // just general help for the tool

            Console.WriteLine("command usages:");
            Console.WriteLine("--unlink   <input .wad>   ::  unlinks the inputted .wad file.");
            Console.WriteLine("  shortcut                :: -u");
            Console.WriteLine("--link     <input folder> ::  links the inputted folder into a .wad file.");
            Console.WriteLine("  shortcut                :: -l");
            Console.WriteLine("--help                    ::  displays help for various commands.");
            Console.WriteLine("  shortcut                :: -h");
            Console.WriteLine("--about                   ::  displays information about this tool.");
            Console.WriteLine("  shortcut                :: -a");
        }

        static void About()
        {
            Console.WriteLine("tool information:");
            Console.WriteLine("wad.exe :: a linker / unlinker tool for 3arc's .wad file type");
            Console.WriteLine("        :: made by ymes_zzz");
        }

        static void Main(string[] Parameters)
        {
            // first check if there are any parameters
            if (Parameters.Length < 1)
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine($"USAGE :: wad.exe <command>");
                Console.ResetColor();
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
                case "-h":
                case "--help":
                    Help();
                    break;
                case "-a":
                case "--about":
                    About();
                    break;
                default:
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine($"ERROR :: unknown command: {Parameters[0]}!");
                    Console.ResetColor();
                    return;
            }
        }
    }
}
