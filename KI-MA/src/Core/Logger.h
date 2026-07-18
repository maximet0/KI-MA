#pragma once
#include <cstdlib>
#include <string>
#include <format>

namespace Core {

	/// <summary>
	/// Logger-Klasse für die Konsolenausgabe von Nachrichten mit verschiedenen Log-Leveln.
	/// </summary>
	class Logger {
	public:
		/// <summary>
		/// Initialisiert den Logger.
		/// </summary>
		static void Init();

		/// <summary>
		/// Schliesst den Logger.
		/// </summary>
		static void Shutdown();
	
		/// <summary>
		/// Schreibt eine Nachricht in die Konsole mit dem Level "Error".
		/// </summary>
		/// <typeparam name="...Args"></typeparam>
		/// <param name="fmt"></param>
		/// <param name="...args"></param>
		template<typename... Args>
		static void Error(std::format_string<Args...> fmt, Args&&... args) {
#ifdef DEBUG
			char msg[4096];
			auto result = std::format_to_n(msg, sizeof(msg) - 1, fmt, std::forward<Args>(args)...);
			msg[result.out - msg] = '\0';
			char time[16];
			getTime(time, sizeof(time));
			char buf[4096];
			result = std::format_to_n(buf, sizeof(buf) - 1, "\033[91m{} [ERROR] {}\n", time, msg);
			WriteToConsole(buf, result.out - buf);
#endif
		};

		/// <summary>
		/// Schreibt eine Nachricht in die Konsole mit dem Level "Warn".
		/// </summary>
		/// <typeparam name="...Args"></typeparam>
		/// <param name="fmt"></param>
		/// <param name="...args"></param>
		template<typename... Args>
		static void Warn(std::format_string<Args...> fmt, Args&&... args) {
#ifdef DEBUG
			char msg[4096];
			auto result = std::format_to_n(msg, sizeof(msg) - 1, fmt, std::forward<Args>(args)...);
			msg[result.out - msg] = '\0';
			char time[16];
			getTime(time, sizeof(time));
			char buf[4096];
			result = std::format_to_n(buf, sizeof(buf) - 1, "\033[93m{} [WARNING] {}\n", time, msg);
			WriteToConsole(buf, result.out - buf);
#endif
		};

		/// <summary>
		/// Schreibt eine Nachricht in die Konsole mit dem Level "Success".
		/// </summary>
		/// <typeparam name="...Args"></typeparam>
		/// <param name="fmt"></param>
		/// <param name="...args"></param>
		template<typename... Args>
		static void Success(std::format_string<Args...> fmt, Args&&... args) {
#ifdef DEBUG
			char msg[4096];
			auto result = std::format_to_n(msg, sizeof(msg) - 1, fmt, std::forward<Args>(args)...);
			msg[result.out - msg] = '\0';
			char time[16];
			getTime(time, sizeof(time));
			char buf[4096];
			result = std::format_to_n(buf, sizeof(buf) - 1, "\033[32m{} [SUCCESS] {}\n", time, msg);
			WriteToConsole(buf, result.out - buf);
#endif
		};

		/// <summary>
		/// Schreibt eine Nachricht in die Konsole mit dem Level "Info".
		/// </summary>
		/// <typeparam name="...Args"></typeparam>
		/// <param name="fmt"></param>
		/// <param name="...args"></param>
		template<typename... Args>
		static void Info(std::format_string<Args...> fmt, Args&&... args) {
#ifdef DEBUG
			char msg[4096];
			auto result = std::format_to_n(msg, sizeof(msg) - 1, fmt, std::forward<Args>(args)...);
			msg[result.out - msg] = '\0';
			char time[16];
			getTime(time, sizeof(time));
			char buf[4096];
			result = std::format_to_n(buf, sizeof(buf) - 1, "\033[37m{} [INFO] {}\n", time, msg);
			WriteToConsole(buf, result.out - buf);
#endif
		};

		/// <summary>
		/// Schreibt eine Nachricht in die Konsole mit dem Level "Debug".
		/// </summary>
		/// <typeparam name="...Args"></typeparam>
		/// <param name="fmt"></param>
		/// <param name="...args"></param>
		template<typename... Args>
		static void Debug(std::format_string<Args...> fmt, Args&&... args) {
#ifdef DEBUG
			char msg[4096];
			auto result = std::format_to_n(msg, sizeof(msg) - 1, fmt, std::forward<Args>(args)...);
			msg[result.out - msg] = '\0';
			char time[16];
			getTime(time, sizeof(time));
			char buf[4096];
			result = std::format_to_n(buf, sizeof(buf) - 1, "\033[90m{} [DEBUG] {}\n", time, msg);
			WriteToConsole(buf, result.out - buf);
#endif
		};

		/// <summary>
		/// Schreibt eine Nachricht in die Konsole mit dem Level "Fatal", zeigt eine Nachricht an und beendet das Programm.
		/// </summary>
		/// <typeparam name="...Args"></typeparam>
		/// <param name="fmt"></param>
		/// <param name="...args"></param>
		template<typename... Args>
		static void Fatal(std::format_string<Args...> fmt, Args&&... args) {
			char msg[4096];
			auto result = std::format_to_n(msg, sizeof(msg) - 1, fmt, std::forward<Args>(args)...);
			msg[result.out - msg] = '\0';
#ifdef DEBUG
			char time[16];
			getTime(time, sizeof(time));
			char buf[4096];
			result = std::format_to_n(buf, sizeof(buf) - 1, "\033[31m{} [FATAL] {}\n", time, msg);
			WriteToConsole(buf, result.out - buf);
#endif
			msgBox(msg);
			exit(EXIT_FAILURE);
		};

	private:
		static void WriteToConsole(char* buffer, size_t size);
		static void msgBox(const char* msg);
		static void getTime(char* buf, size_t bufSize);
	};
}