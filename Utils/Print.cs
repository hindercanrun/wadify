using System;

namespace Utils
{
	internal class Print
	{
		//
		// Writes a message to the console.
		//
		// Usage:
		//  WriteMessage(<Message>);
		//
		// Example:
		//  WriteMessage("Hello, I'm a regular message!");
		//
		internal static void WriteMessage(string Message)
		{
			Console.ForegroundColor = ConsoleColor.White;
			Console.WriteLine(Message);
			Console.ResetColor();
		}

		//
		// Writes a miscellaneous message to the console.
		//
		// Usage:
		//  WriteMiscMessage(<Message>);
		//
		// Example:
		//  WriteMiscMessage("Hello, I'm a special message!");
		//
		internal static void WriteMiscMessage(string Message)
		{
			Console.ForegroundColor = ConsoleColor.Green;
			Console.WriteLine(Message);
			Console.ResetColor();
		}

#if DEBUG
		//
		// DEV ONLY!!
		//
		// Writes a debug message to the console.
		//
		// Usage:
		//  WriteDebugMessage(<Message>);
		//
		// Example:
		//  WriteDebugMessage("Hello, I'm a debug message!");
		//
		internal static void WriteDebugMessage(String Message)
		{
			Console.ForegroundColor = ConsoleColor.Blue;
			Console.WriteLine($"DEBUG :: {Message}");
			Console.ResetColor();
		}
#endif

		//
		// Writes a message to the console with a green colour.
		//
		// Usage:
		//  WriteGreenMessage(<Message>);
		//
		// Example:
		//  WriteGreenMessage("Hello, I'm a regular message, but I'm green!");
		//
		internal static void WriteGreenMessage(string Message)
		{
			Console.ForegroundColor = ConsoleColor.Green;
			Console.WriteLine(Message);
			Console.ResetColor();
		}

		//
		// Writes a warning message to the console.
		//
		// Usage:
		//  WriteWarningMessage(<Message>);
		//
		// Example:
		//  WriteWarningMessage("Hello, I'm a warning message!");
		//
		internal static void WriteWarning(string Message)
		{
			Console.ForegroundColor = ConsoleColor.Yellow;
			Console.WriteLine(Message);
			Console.ResetColor();
		}

		//
		// Writes an error message to the console.
		//
		// Usage:
		//  WriteErrorMessage(<Message>);
		//
		// Example:
		//  WriteErrorMessage("Hello, I'm an error message!");
		//
		internal static void WriteError(string Message)
		{
			Console.ForegroundColor = ConsoleColor.Red;
			Console.WriteLine($"ERROR :: {Message}");
			Console.ResetColor();
		}

		//
		// Writes an exception error message to the console.
		//
		// Usage:
		//  WriteExceptionError(
		//		<Message>,
		//		<Exception>);
		//
		// Example:
		//  WriteExceptionError(
		//		"Hello, I'm a warning message!",
		//		"Hello, I'm an exception message!");
		//
		internal static void WriteExceptionError(String Message, String Exception)
		{
			Console.ForegroundColor = ConsoleColor.Red;
			Console.WriteLine($"ERROR  :: {Message}");
			Console.WriteLine($"REASON :: {Exception}");
			Console.ResetColor();
		}

		//
		// Writes a usage warning message to the console.
		//
		// Usage:
		//  WriteUsageWarning(<Message>);
		//
		// Example:
		//  WriteUsageWarning("Hello, I'm a usage warning message!");
		//
		internal static void WriteUsageWarning(string Message)
		{
			Console.ForegroundColor = ConsoleColor.Yellow;
			Console.WriteLine($"USAGE :: {Message}");
			Console.ResetColor();
		}
	}
}