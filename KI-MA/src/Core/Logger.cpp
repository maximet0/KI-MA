#include "Logger.h"

#include <Windows.h>
#include <iostream>
#include <chrono>

namespace Core {
	void Core::Logger::Init()
	{
#ifdef DEBUG
		// Erstellt eine Konsole für die Anwendung und leitet die Standardausgabe und Standardfehlerausgabe an diese Konsole weiter.
		AllocConsole();
		FILE* newStdOUT;
		freopen_s(&newStdOUT, "CONOUT$", "w", stdout);
		freopen_s(&newStdOUT, "CONOUT$", "w", stderr);
		DWORD dwMode = 0;
		GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &dwMode);
		SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
		SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_EXTENDED_FLAGS);
#endif
	}

	void Core::Logger::Shutdown()
	{
#ifdef DEBUG
		FreeConsole();
#endif
	}

	void Logger::WriteToConsole(char* buffer, size_t size)
	{
		WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buffer, (DWORD)size, NULL, NULL);
	}

	void Logger::msgBox(const char* msg)
	{
		wchar_t winMsg[2048] = { 0 };
		int strLen = static_cast<int>(strlen(msg));
		MultiByteToWideChar(0, 0, msg, strLen, winMsg, strLen);
		MessageBox(NULL, winMsg, L"Application", MB_OK | MB_ICONERROR);
	}

	void Logger::getTime(char* buf, size_t bufSize)
	{
		// Die aktuelle Zeit im Format [HH:MM:SS] in den Puffer buf schreiben.
		auto localTime = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
		auto timePoint = std::chrono::duration_cast<std::chrono::seconds>(localTime.time_since_epoch() % std::chrono::days{ 1 });
		auto HMS = std::chrono::hh_mm_ss(timePoint);

		auto result = std::format_to_n(buf, bufSize - 1, "[{:02}:{:02}:{:02}]", HMS.hours().count(), HMS.minutes().count(), HMS.seconds().count());
		buf[result.out - buf] = '\0';
	}

}

