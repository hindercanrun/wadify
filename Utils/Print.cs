using System;

namespace Utils
{
    class Print
    {
        //
        // writes a regular message to the console
        //
        public static void WriteMessage(string Message)
        {
            Console.ForegroundColor = ConsoleColor.White;
            Console.WriteLine(Message);
            Console.ResetColor();
        }

        //
        // writes a misc message to the console
        //
        public static void WriteMiscMessage(string Message)
        {
            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine(Message);
            Console.ResetColor();
        }

#if DEBUG
        //
        // DEV ONLY :: writes a debug message to the console
        //
        public static void WriteDebugMessage(String Message)
        {
            Console.ForegroundColor = ConsoleColor.Blue;
            Console.WriteLine($"DEBUG :: {Message}");
            Console.ResetColor();
        }
#endif

        //
        // writes a regular message with the colour green to the console
        //
        public static void WriteGreenMessage(string Message)
        {
            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine(Message);
            Console.ResetColor();
        }

        //
        // writes a warning message to the console
        //
        public static void WriteWarning(string Message)
        {
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine(Message);
            Console.ResetColor();
        }

        //
        // writes an error message to the console
        //
        public static void WriteError(string Message)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"ERROR :: {Message}");
            Console.ResetColor();
        }

        //
        // writes an exception error message to the console
        //
        public static void WriteExceptionError(String Message, String Exception)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"ERROR  :: {Message}");
            Console.WriteLine($"REASON :: {Exception}");
            Console.ResetColor();
        }

        //
        // the exact same as WriteWarning
        // but only used for usage command
        //
        public static void WriteUsageWarning(string Message)
        {
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine($"USAGE :: {Message}");
            Console.ResetColor();
        }
    }
}
