#include <iostream>
#include <filesystem>

#include "Core/Application.h"
#include "Core/Logger.h"

// Setzt das Arbeitsverzeichnis der Anwendung auf den Ordner, in dem die ausführbare Datei liegt.
// So kann die Anwendung in jeder Umgebung mit relativen Pfaden arbeiten.
void setWorkingDir() {
	wchar_t buffer[MAX_PATH];
	GetModuleFileNameW(NULL, buffer, MAX_PATH);
	std::filesystem::path exePath(buffer);
	std::filesystem::current_path(exePath.parent_path());
}

// Hauptfunktion der Anwendung
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	Core::Logger::Init();
	Core::Logger::Debug("Starting Application");
	setWorkingDir();

	Core::Application app;
	app.onStart();
	while (app.isRunning()) {
		app.onUpdate();
	}

	return 0;
}
