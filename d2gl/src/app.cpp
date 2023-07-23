/*
	D2GL: Diablo 2 LoD Glide/DDraw to OpenGL Wrapper.
	Copyright (C) 2023  Bayaraa

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "pch.h"
#include "d2/common.h"
#include "helpers.h"
#include "option/ini.h"
#include "win32.h"

namespace d2gl {

D2GLApp App;

void checkCompatibilityMode()
{
	char buffer[1024] = { 0 };
	if (GetEnvironmentVariableA("__COMPAT_LAYER", buffer, sizeof(buffer))) {
		std::string compat_str(buffer);
		helpers::strToLower(compat_str);
		if (compat_str.find("win95") != std::string::npos || compat_str.find("win98") != std::string::npos || compat_str.find("nt4sp5") != std::string::npos) {
			MessageBoxA(NULL, "Please disable compatibility mode for game executable and then try to start the game again.", "Compatibility mode detected!", MB_OK | MB_ICONWARNING);
			error_log("Compatibility mode '%s' detected!", buffer);
			exit(1);
		}
	}
}

DWORD RegQueryValueDWORD(HKEY hKey, LPCSTR szSubKey, LPCSTR szValueName, int nDefault)
{
	BYTE Data[4] = {0};
	DWORD cbData = sizeof(DWORD);
	HKEY phkResult = 0;

	if (RegOpenKeyExA(hKey, szSubKey, 0, KEY_QUERY_VALUE, &phkResult)) {
		return nDefault;
	}

	if (!RegQueryValueExA(phkResult, szValueName, 0, 0, Data, &cbData)) {
		return *(DWORD*)Data;
	}

	RegCloseKey(phkResult);
	return nDefault;
}

void dllAttach(HMODULE hmodule)
{
	std::string command_line = GetCommandLineA();
	helpers::strToLower(command_line);

	if (command_line.find("d2vidtst") != std::string::npos) {
		App.video_test = true;
		return;
	}

	//bool flag_3dfx = command_line.find("-3dfx") != std::string::npos;
	//flag_3dfx = !flag_3dfx ? *d2::video_mode == 4 : flag_3dfx;

	//if ((App.api == Api::Glide && !flag_3dfx) || (App.api == Api::DDraw && flag_3dfx))

	static char acPath[MAX_PATH] = { 0 };
	char* pcTemp = 0;
	GetModuleFileNameA(hmodule, acPath, sizeof(acPath));
	pcTemp = strrchr(acPath, '\\');
	if (pcTemp != 0) {
		*(char*)(pcTemp) = 0;
		if (RegQueryValueDWORD(HKEY_CURRENT_USER, "Software\\Blizzard Entertainment\\Diablo II\\VideoConfig", "Render", 3) == 3) {
			App.api = Api::Glide;
			// if called from ddraw.dll, return
			if (!_stricmp((char*)(pcTemp + 1), "ddraw.dll")) 
				return;
		} else if (RegQueryValueDWORD(HKEY_CURRENT_USER, "Software\\Blizzard Entertainment\\Diablo II\\VideoConfig", "Render", 0) == 0) {
			App.api = Api::DDraw;
			// if called from glide3x.dll, return
			if (!_stricmp((char*)(pcTemp + 1), "glide3x.dll")) 
		return;
		} else if (RegQueryValueDWORD(HKEY_CURRENT_USER, "Software\\Blizzard Entertainment\\Diablo II\\VideoConfig", "Render", 1) == 1) {
			return; // if direct3d
		}
	}

	if (command_line.find("-w ") != std::string::npos || command_line.find("-w") == command_line.length() - 2) {
		if (App.api == Api::Glide) {
			MessageBoxA(NULL, "D2GL Glide wrapper is not compatible with \"-w\" flag.\nRemove \"-w\" flag and run game again.", "Unsupported argument detected!", MB_OK | MB_ICONWARNING);
			exit(1);
		}
		return;
	}

	App.log = command_line.find("-log") != std::string::npos;
	App.direct = command_line.find("-direct") != std::string::npos;

	logInit();
	trace_log("Renderer Api: %s", App.api == Api::Glide ? "Glide" : "DDraw");

	auto ini_pos = command_line.find("-config ");
	if (ini_pos != std::string::npos) {
		std::string custom_ini = "";
		for (size_t i = ini_pos + 8; i < command_line.length(); i++) {
			if (command_line.at(i) == ' ')
				break;
			custom_ini += command_line.at(i);
		}
		custom_ini.erase(std::remove(custom_ini.begin(), custom_ini.end(), ' '), custom_ini.end());
		if (custom_ini.length() > 0) {
			App.ini_file = "d2gl_" + custom_ini + ".ini";
			trace_log("Custom config file: %s", App.ini_file.c_str());
		}
	}

	if (helpers::getVersion() == Version::Unknown) {
		MessageBoxA(NULL, "Game version is not supported!", "Unsupported version!", MB_OK | MB_ICONERROR);
		error_log("Game version is not supported!");
		exit(1);
	}
	trace_log("Diablo 2 LoD (%s) version %s detected.", helpers::getLangString().c_str(), helpers::getVersionString().c_str());

	checkCompatibilityMode();
	timeBeginPeriod(1);
	win32::setDPIAwareness();
	App.hmodule = hmodule;

	option::loadIni();
	helpers::loadDlls(App.dlls_early);

	d2::initHooks();
	win32::initHooks();
}

void dllDetach()
{
	if (App.hmodule) {
		win32::destroyHooks();
		d2::destroyHooks();
		timeEndPeriod(1);
		exit(EXIT_SUCCESS);
	}
}

}