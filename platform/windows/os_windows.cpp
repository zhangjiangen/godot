/*************************************************************************/
/*  os_windows.cpp                                                       */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "os_windows.h"

#include "core/debugger/engine_debugger.h"
#include "core/debugger/script_debugger.h"
#include "core/io/marshalls.h"
#include "core/version_generated.gen.h"
#include "drivers/unix/net_socket_posix.h"
#include "drivers/windows/dir_access_windows.h"
#include "drivers/windows/file_access_windows.h"
#include "joypad_windows.h"
#include "lang_table.h"
#include "main/main.h"
#include "platform/windows/display_server_windows.h"
#include "servers/audio_server.h"
#include "servers/rendering/rendering_server_default.h"
#include "windows_terminal_logger.h"

#include <avrt.h>
#include <bcrypt.h>
#include <direct.h>
#include <knownfolders.h>
#include <process.h>
#include <regstr.h>
#include <shlobj.h>

extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

// Workaround mingw-w64 < 4.0 bug
#ifndef WM_TOUCH
#define WM_TOUCH 576
#endif

#ifndef WM_POINTERUPDATE
#define WM_POINTERUPDATE 0x0245
#endif

#if defined(__GNUC__)
// Workaround GCC warning from -Wcast-function-type.
#define GetProcAddress (void *)GetProcAddress
#endif

static String format_error_message(DWORD id) {
	LPWSTR messageBuffer = nullptr;
	size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, nullptr);

	String msg = "Error " + itos(id) + ": " + String::utf16((const char16_t *)messageBuffer, size);

	LocalFree(messageBuffer);

	return msg;
}

void RedirectStream(const char *p_file_name, const char *p_mode, FILE *p_cpp_stream, const DWORD p_std_handle) {
	const HANDLE h_existing = GetStdHandle(p_std_handle);
	if (h_existing != INVALID_HANDLE_VALUE) { // Redirect only if attached console have a valid handle.
		const HANDLE h_cpp = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(p_cpp_stream)));
		if (h_cpp == INVALID_HANDLE_VALUE) { // Redirect only if it's not already redirected to the pipe or file.
			FILE *fp = p_cpp_stream;
			freopen_s(&fp, p_file_name, p_mode, p_cpp_stream); // Redirect stream.
			setvbuf(p_cpp_stream, nullptr, _IONBF, 0); // Disable stream buffering.
		}
	}
}

void RedirectIOToConsole() {
	if (AttachConsole(ATTACH_PARENT_PROCESS)) {
		RedirectStream("CONIN$", "r", stdin, STD_INPUT_HANDLE);
		RedirectStream("CONOUT$", "w", stdout, STD_OUTPUT_HANDLE);
		RedirectStream("CONOUT$", "w", stderr, STD_ERROR_HANDLE);

		printf("\n"); // Make sure our output is starting from the new line.
	}
}

BOOL WINAPI HandlerRoutine(_In_ DWORD dwCtrlType) {
	if (!EngineDebugger::is_active()) {
		return FALSE;
	}

	switch (dwCtrlType) {
		case CTRL_C_EVENT:
			EngineDebugger::get_script_debugger()->set_depth(-1);
			EngineDebugger::get_script_debugger()->set_lines_left(1);
			return TRUE;
		default:
			return FALSE;
	}
}

void OS_Windows::alert(const String &p_alert, const String &p_title) {
	MessageBoxW(nullptr, (LPCWSTR)(p_alert.utf16().get_data()), (LPCWSTR)(p_title.utf16().get_data()), MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
}

void OS_Windows::initialize_debugging() {
	SetConsoleCtrlHandler(HandlerRoutine, TRUE);
}

void OS_Windows::initialize() {
	crash_handler.initialize();

#ifndef WINDOWS_SUBSYSTEM_CONSOLE
	RedirectIOToConsole();
#endif

	FileAccess::make_default<FileAccessWindows>(FileAccess::ACCESS_RESOURCES);
	FileAccess::make_default<FileAccessWindows>(FileAccess::ACCESS_USERDATA);
	FileAccess::make_default<FileAccessWindows>(FileAccess::ACCESS_FILESYSTEM);
	DirAccess::make_default<DirAccessWindows>(DirAccess::ACCESS_RESOURCES);
	DirAccess::make_default<DirAccessWindows>(DirAccess::ACCESS_USERDATA);
	DirAccess::make_default<DirAccessWindows>(DirAccess::ACCESS_FILESYSTEM);

	NetSocketPosix::make_default();

	// We need to know how often the clock is updated
	QueryPerformanceFrequency((LARGE_INTEGER *)&ticks_per_second);
	QueryPerformanceCounter((LARGE_INTEGER *)&ticks_start);

	// set minimum resolution for periodic timers, otherwise Sleep(n) may wait at least as
	//  long as the windows scheduler resolution (~16-30ms) even for calls like Sleep(1)
	timeBeginPeriod(1);

	process_map = memnew((Map<ProcessID, ProcessInfo>));

	// Add current Godot PID to the list of known PIDs
	ProcessInfo current_pi = {};
	PROCESS_INFORMATION current_pi_pi = {};
	current_pi.pi = current_pi_pi;
	current_pi.pi.hProcess = GetCurrentProcess();
	process_map->insert(GetCurrentProcessId(), current_pi);

	IPUnix::make_default();
	main_loop = nullptr;
}

void OS_Windows::delete_main_loop() {
	if (main_loop) {
		memdelete(main_loop);
	}
	main_loop = nullptr;
}

void OS_Windows::set_main_loop(MainLoop *p_main_loop) {
	main_loop = p_main_loop;
}

void OS_Windows::finalize() {
#ifdef WINMIDI_ENABLED
	driver_midi.close();
#endif

	if (main_loop) {
		memdelete(main_loop);
	}

	main_loop = nullptr;
}

void OS_Windows::finalize_core() {
	timeEndPeriod(1);

	memdelete(process_map);
	NetSocketPosix::cleanup();
}

Error OS_Windows::get_entropy(uint8_t *r_buffer, int p_bytes) {
	NTSTATUS status = BCryptGenRandom(nullptr, r_buffer, p_bytes, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
	ERR_FAIL_COND_V(status, FAILED);
	return OK;
}

Error OS_Windows::open_dynamic_library(const String p_path, void *&p_library_handle, bool p_also_set_library_path) {
	String path = p_path.replace("/", "\\");

	if (!FileAccess::exists(path)) {
		//this code exists so gdnative can load .dll files from within the executable path
		path = get_executable_path().get_base_dir().plus_file(p_path.get_file());
	}

	typedef DLL_DIRECTORY_COOKIE(WINAPI * PAddDllDirectory)(PCWSTR);
	typedef BOOL(WINAPI * PRemoveDllDirectory)(DLL_DIRECTORY_COOKIE);

	PAddDllDirectory add_dll_directory = (PAddDllDirectory)GetProcAddress(GetModuleHandle("kernel32.dll"), "AddDllDirectory");
	PRemoveDllDirectory remove_dll_directory = (PRemoveDllDirectory)GetProcAddress(GetModuleHandle("kernel32.dll"), "RemoveDllDirectory");

	bool has_dll_directory_api = ((add_dll_directory != nullptr) && (remove_dll_directory != nullptr));
	DLL_DIRECTORY_COOKIE cookie = nullptr;

	if (p_also_set_library_path && has_dll_directory_api) {
		cookie = add_dll_directory((LPCWSTR)(path.get_base_dir().utf16().get_data()));
	}

	p_library_handle = (void *)LoadLibraryExW((LPCWSTR)(path.utf16().get_data()), nullptr, (p_also_set_library_path && has_dll_directory_api) ? LOAD_LIBRARY_SEARCH_DEFAULT_DIRS : 0);
	ERR_FAIL_COND_V_MSG(!p_library_handle, ERR_CANT_OPEN, "Can't open dynamic library: " + p_path + ", error: " + format_error_message(GetLastError()) + ".");

	if (cookie) {
		remove_dll_directory(cookie);
	}

	return OK;
}

Error OS_Windows::close_dynamic_library(void *p_library_handle) {
	if (!FreeLibrary((HMODULE)p_library_handle)) {
		return FAILED;
	}
	return OK;
}

Error OS_Windows::get_dynamic_library_symbol_handle(void *p_library_handle, const String p_name, void *&p_symbol_handle, bool p_optional) {
	p_symbol_handle = (void *)GetProcAddress((HMODULE)p_library_handle, p_name.utf8().get_data());
	if (!p_symbol_handle) {
		if (!p_optional) {
			ERR_FAIL_V_MSG(ERR_CANT_RESOLVE, "Can't resolve symbol " + p_name + ", error: " + String::num(GetLastError()) + ".");
		} else {
			return ERR_CANT_RESOLVE;
		}
	}
	return OK;
}

String OS_Windows::get_name() const {
	return "Windows";
}

OS::Date OS_Windows::get_date(bool p_utc) const {
	SYSTEMTIME systemtime;
	if (p_utc) {
		GetSystemTime(&systemtime);
	} else {
		GetLocalTime(&systemtime);
	}

	//Get DST information from Windows, but only if p_utc is false.
	TIME_ZONE_INFORMATION info;
	bool daylight = false;
	if (!p_utc && GetTimeZoneInformation(&info) == TIME_ZONE_ID_DAYLIGHT) {
		daylight = true;
	}

	Date date;
	date.day = systemtime.wDay;
	date.month = Month(systemtime.wMonth);
	date.weekday = Weekday(systemtime.wDayOfWeek);
	date.year = systemtime.wYear;
	date.dst = daylight;
	return date;
}

OS::Time OS_Windows::get_time(bool p_utc) const {
	SYSTEMTIME systemtime;
	if (p_utc) {
		GetSystemTime(&systemtime);
	} else {
		GetLocalTime(&systemtime);
	}

	Time time;
	time.hour = systemtime.wHour;
	time.minute = systemtime.wMinute;
	time.second = systemtime.wSecond;
	return time;
}

OS::TimeZoneInfo OS_Windows::get_time_zone_info() const {
	TIME_ZONE_INFORMATION info;
	bool daylight = false;
	if (GetTimeZoneInformation(&info) == TIME_ZONE_ID_DAYLIGHT) {
		daylight = true;
	}

	// Daylight Bias needs to be added to the bias if DST is in effect, or else it will not properly update.
	TimeZoneInfo ret;
	if (daylight) {
		ret.name = info.DaylightName;
		ret.bias = info.Bias + info.DaylightBias;
	} else {
		ret.name = info.StandardName;
		ret.bias = info.Bias + info.StandardBias;
	}

	// Bias value returned by GetTimeZoneInformation is inverted of what we expect
	// For example, on GMT-3 GetTimeZoneInformation return a Bias of 180, so invert the value to get -180
	ret.bias = -ret.bias;
	return ret;
}

double OS_Windows::get_unix_time() const {
	// 1 Windows tick is 100ns
	const uint64_t WINDOWS_TICKS_PER_SECOND = 10000000;
	const uint64_t TICKS_TO_UNIX_EPOCH = 116444736000000000LL;

	SYSTEMTIME st;
	GetSystemTime(&st);
	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);
	uint64_t ticks_time;
	ticks_time = ft.dwHighDateTime;
	ticks_time <<= 32;
	ticks_time |= ft.dwLowDateTime;

	return (double)(ticks_time - TICKS_TO_UNIX_EPOCH) / WINDOWS_TICKS_PER_SECOND;
}

void OS_Windows::delay_usec(uint32_t p_usec) const {
	if (p_usec < 1000) {
		Sleep(1);
	} else {
		Sleep(p_usec / 1000);
	}
}

uint64_t OS_Windows::get_ticks_usec() const {
	uint64_t ticks;

	// This is the number of clock ticks since start
	QueryPerformanceCounter((LARGE_INTEGER *)&ticks);
	// Subtract the ticks at game start to get
	// the ticks since the game started
	ticks -= ticks_start;

	// Divide by frequency to get the time in seconds
	// original calculation shown below is subject to overflow
	// with high ticks_per_second and a number of days since the last reboot.
	// time = ticks * 1000000L / ticks_per_second;

	// we can prevent this by either using 128 bit math
	// or separating into a calculation for seconds, and the fraction
	uint64_t seconds = ticks / ticks_per_second;

	// compiler will optimize these two into one divide
	uint64_t leftover = ticks % ticks_per_second;

	// remainder
	uint64_t time = (leftover * 1000000L) / ticks_per_second;

	// seconds
	time += seconds * 1000000L;

	return time;
}

String OS_Windows::_quote_command_line_argument(const String &p_text) const {
	for (int i = 0; i < p_text.size(); i++) {
		char32_t c = p_text[i];
		if (c == ' ' || c == '&' || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == '^' || c == '=' || c == ';' || c == '!' || c == '\'' || c == '+' || c == ',' || c == '`' || c == '~') {
			return "\"" + p_text + "\"";
		}
	}
	return p_text;
}

Error OS_Windows::execute(const String &p_path, const List<String> &p_arguments, String *r_pipe, int *r_exitcode, bool read_stderr, Mutex *p_pipe_mutex, bool p_open_console) {
	String path = p_path.replace("/", "\\");
	String command = _quote_command_line_argument(path);
	for (const String &E : p_arguments) {
		command += " " + _quote_command_line_argument(E);
	}

	ProcessInfo pi;
	ZeroMemory(&pi.si, sizeof(pi.si));
	pi.si.cb = sizeof(pi.si);
	ZeroMemory(&pi.pi, sizeof(pi.pi));
	LPSTARTUPINFOW si_w = (LPSTARTUPINFOW)&pi.si;

	bool inherit_handles = false;
	HANDLE pipe[2] = { nullptr, nullptr };
	if (r_pipe) {
		// Create pipe for StdOut and StdErr.
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle = true;
		sa.lpSecurityDescriptor = nullptr;

		ERR_FAIL_COND_V(!CreatePipe(&pipe[0], &pipe[1], &sa, 0), ERR_CANT_FORK);
		ERR_FAIL_COND_V(!SetHandleInformation(pipe[0], HANDLE_FLAG_INHERIT, 0), ERR_CANT_FORK); // Read handle is for host process only and should not be inherited.

		pi.si.dwFlags |= STARTF_USESTDHANDLES;
		pi.si.hStdOutput = pipe[1];
		if (read_stderr) {
			pi.si.hStdError = pipe[1];
		}
		inherit_handles = true;
	}
	DWORD creation_flags = NORMAL_PRIORITY_CLASS;
	if (p_open_console) {
		creation_flags |= CREATE_NEW_CONSOLE;
	} else {
		creation_flags |= CREATE_NO_WINDOW;
	}

	int ret = CreateProcessW(nullptr, (LPWSTR)(command.utf16().ptrw()), nullptr, nullptr, inherit_handles, creation_flags, nullptr, nullptr, si_w, &pi.pi);
	if (!ret && r_pipe) {
		CloseHandle(pipe[0]); // Cleanup pipe handles.
		CloseHandle(pipe[1]);
	}
	ERR_FAIL_COND_V_MSG(ret == 0, ERR_CANT_FORK, "Could not create child process: " + command);

	if (r_pipe) {
		CloseHandle(pipe[1]); // Close pipe write handle (only child process is writing).
		char buf[4096];
		DWORD read = 0;
		for (;;) { // Read StdOut and StdErr from pipe.
			bool success = ReadFile(pipe[0], buf, 4096, &read, NULL);
			if (!success || read == 0) {
				break;
			}
			if (p_pipe_mutex) {
				p_pipe_mutex->lock();
			}
			(*r_pipe) += String::utf8(buf, read);
			if (p_pipe_mutex) {
				p_pipe_mutex->unlock();
			}
		}
		CloseHandle(pipe[0]); // Close pipe read handle.
	} else {
		WaitForSingleObject(pi.pi.hProcess, INFINITE);
	}

	if (r_exitcode) {
		DWORD ret2;
		GetExitCodeProcess(pi.pi.hProcess, &ret2);
		*r_exitcode = ret2;
	}

	CloseHandle(pi.pi.hProcess);
	CloseHandle(pi.pi.hThread);

	return OK;
}

Error OS_Windows::create_process(const String &p_path, const List<String> &p_arguments, ProcessID *r_child_id, bool p_open_console) {
	String path = p_path.replace("/", "\\");
	String command = _quote_command_line_argument(path);
	for (const String &E : p_arguments) {
		command += " " + _quote_command_line_argument(E);
	}

	ProcessInfo pi;
	ZeroMemory(&pi.si, sizeof(pi.si));
	pi.si.cb = sizeof(pi.si);
	ZeroMemory(&pi.pi, sizeof(pi.pi));
	LPSTARTUPINFOW si_w = (LPSTARTUPINFOW)&pi.si;

	DWORD creation_flags = NORMAL_PRIORITY_CLASS;
	if (p_open_console) {
		creation_flags |= CREATE_NEW_CONSOLE;
	} else {
		creation_flags |= CREATE_NO_WINDOW;
	}

	int ret = CreateProcessW(nullptr, (LPWSTR)(command.utf16().ptrw()), nullptr, nullptr, false, creation_flags, nullptr, nullptr, si_w, &pi.pi);
	ERR_FAIL_COND_V_MSG(ret == 0, ERR_CANT_FORK, "Could not create child process: " + command);

	ProcessID pid = pi.pi.dwProcessId;
	if (r_child_id) {
		*r_child_id = pid;
	}
	process_map->insert(pid, pi);

	return OK;
}

Error OS_Windows::kill(const ProcessID &p_pid) {
	ERR_FAIL_COND_V(!process_map->has(p_pid), FAILED);

	const PROCESS_INFORMATION pi = (*process_map)[p_pid].pi;
	process_map->erase(p_pid);

	const int ret = TerminateProcess(pi.hProcess, 0);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return ret != 0 ? OK : FAILED;
}

int OS_Windows::get_process_id() const {
	return _getpid();
}

Error OS_Windows::set_cwd(const String &p_cwd) {
	if (_wchdir((LPCWSTR)(p_cwd.utf16().get_data())) != 0) {
		return ERR_CANT_OPEN;
	}

	return OK;
}

String OS_Windows::get_executable_path() const {
	WCHAR bufname[4096];
	GetModuleFileNameW(nullptr, bufname, 4096);
	String s = String::utf16((const char16_t *)bufname).replace("\\", "/");
	return s;
}

bool OS_Windows::has_environment(const String &p_var) const {
#ifdef MINGW_ENABLED
	return _wgetenv((LPCWSTR)(p_var.utf16().get_data())) != nullptr;
#else
	WCHAR *env;
	size_t len;
	_wdupenv_s(&env, &len, (LPCWSTR)(p_var.utf16().get_data()));
	const bool has_env = env != nullptr;
	free(env);
	return has_env;
#endif
}

String OS_Windows::get_environment(const String &p_var) const {
	WCHAR wval[0x7fff]; // MSDN says 32767 char is the maximum
	int wlen = GetEnvironmentVariableW((LPCWSTR)(p_var.utf16().get_data()), wval, 0x7fff);
	if (wlen > 0) {
		return String::utf16((const char16_t *)wval);
	}
	return "";
}

bool OS_Windows::set_environment(const String &p_var, const String &p_value) const {
	return (bool)SetEnvironmentVariableW((LPCWSTR)(p_var.utf16().get_data()), (LPCWSTR)(p_value.utf16().get_data()));
}

String OS_Windows::get_stdin_string(bool p_block) {
	if (p_block) {
		char buff[1024];
		return fgets(buff, 1024, stdin);
	}

	return String();
}

Error OS_Windows::shell_open(String p_uri) {
	INT_PTR ret = (INT_PTR)ShellExecuteW(nullptr, nullptr, (LPCWSTR)(p_uri.utf16().get_data()), nullptr, nullptr, SW_SHOWNORMAL);
	if (ret > 32) {
		return OK;
	} else {
		switch (ret) {
			case ERROR_FILE_NOT_FOUND:
			case SE_ERR_DLLNOTFOUND:
				return ERR_FILE_NOT_FOUND;
			case ERROR_PATH_NOT_FOUND:
				return ERR_FILE_BAD_PATH;
			case ERROR_BAD_FORMAT:
				return ERR_FILE_CORRUPT;
			case SE_ERR_ACCESSDENIED:
				return ERR_UNAUTHORIZED;
			case 0:
			case SE_ERR_OOM:
				return ERR_OUT_OF_MEMORY;
			default:
				return FAILED;
		}
	}
}

String OS_Windows::get_locale() const {
	const _WinLocale *wl = &_win_locales[0];

	LANGID langid = GetUserDefaultUILanguage();
	String neutral;
	int lang = PRIMARYLANGID(langid);
	int sublang = SUBLANGID(langid);

	while (wl->locale) {
		if (wl->main_lang == lang && wl->sublang == SUBLANG_NEUTRAL) {
			neutral = wl->locale;
		}

		if (lang == wl->main_lang && sublang == wl->sublang) {
			return String(wl->locale).replace("-", "_");
		}

		wl++;
	}

	if (!neutral.is_empty()) {
		return String(neutral).replace("-", "_");
	}

	return "en";
}

// We need this because GetSystemInfo() is unreliable on WOW64
// see https://msdn.microsoft.com/en-us/library/windows/desktop/ms724381(v=vs.85).aspx
// Taken from MSDN
typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;

BOOL is_wow64() {
	BOOL wow64 = FALSE;

	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

	if (fnIsWow64Process) {
		if (!fnIsWow64Process(GetCurrentProcess(), &wow64)) {
			wow64 = FALSE;
		}
	}

	return wow64;
}

int OS_Windows::get_processor_count() const {
	SYSTEM_INFO sysinfo;
	if (is_wow64()) {
		GetNativeSystemInfo(&sysinfo);
	} else {
		GetSystemInfo(&sysinfo);
	}

	return sysinfo.dwNumberOfProcessors;
}

String OS_Windows::get_processor_name() const {
	const String id = "Hardware\\Description\\System\\CentralProcessor\\0";

	HKEY hkey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, (LPCWSTR)(id.utf16().get_data()), 0, KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS) {
		ERR_FAIL_V_MSG("", String("Couldn't get the CPU model name. Returning an empty string."));
	}

	WCHAR buffer[256];
	DWORD buffer_len = 256;
	DWORD vtype = REG_SZ;
	if (RegQueryValueExW(hkey, L"ProcessorNameString", NULL, &vtype, (LPBYTE)buffer, &buffer_len) == ERROR_SUCCESS) {
		RegCloseKey(hkey);
		return String::utf16((const char16_t *)buffer, buffer_len).strip_edges();
	} else {
		RegCloseKey(hkey);
		ERR_FAIL_V_MSG("", String("Couldn't get the CPU model name. Returning an empty string."));
	}
}

void OS_Windows::run() {
	if (!main_loop) {
		return;
	}

	main_loop->initialize();

	while (!force_quit) {
		DisplayServer::get_singleton()->process_events(); // get rid of pending events
		if (Main::iteration()) {
			break;
		}
	}

	main_loop->finalize();
}

MainLoop *OS_Windows::get_main_loop() const {
	return main_loop;
}

String OS_Windows::get_config_path() const {
	// The XDG Base Directory specification technically only applies on Linux/*BSD, but it doesn't hurt to support it on Windows as well.
	if (has_environment("XDG_CONFIG_HOME")) {
		if (get_environment("XDG_CONFIG_HOME").is_absolute_path()) {
			return get_environment("XDG_CONFIG_HOME").replace("\\", "/");
		} else {
			WARN_PRINT_ONCE("`XDG_CONFIG_HOME` is a relative path. Ignoring its value and falling back to `%APPDATA%` or `.` per the XDG Base Directory specification.");
		}
	}
	if (has_environment("APPDATA")) {
		return get_environment("APPDATA").replace("\\", "/");
	}
	return ".";
}

String OS_Windows::get_data_path() const {
	// The XDG Base Directory specification technically only applies on Linux/*BSD, but it doesn't hurt to support it on Windows as well.
	if (has_environment("XDG_DATA_HOME")) {
		if (get_environment("XDG_DATA_HOME").is_absolute_path()) {
			return get_environment("XDG_DATA_HOME").replace("\\", "/");
		} else {
			WARN_PRINT_ONCE("`XDG_DATA_HOME` is a relative path. Ignoring its value and falling back to `get_config_path()` per the XDG Base Directory specification.");
		}
	}
	return get_config_path();
}

String OS_Windows::get_cache_path() const {
	static String cache_path_cache;
	if (cache_path_cache.is_empty()) {
		// The XDG Base Directory specification technically only applies on Linux/*BSD, but it doesn't hurt to support it on Windows as well.
		if (has_environment("XDG_CACHE_HOME")) {
			if (get_environment("XDG_CACHE_HOME").is_absolute_path()) {
				cache_path_cache = get_environment("XDG_CACHE_HOME").replace("\\", "/");
			} else {
				WARN_PRINT_ONCE("`XDG_CACHE_HOME` is a relative path. Ignoring its value and falling back to `%LOCALAPPDATA%\\cache`, `%TEMP%` or `get_config_path()` per the XDG Base Directory specification.");
			}
		}
		if (cache_path_cache.is_empty() && has_environment("LOCALAPPDATA")) {
			cache_path_cache = get_environment("LOCALAPPDATA").replace("\\", "/");
		}
		if (cache_path_cache.is_empty() && has_environment("TEMP")) {
			cache_path_cache = get_environment("TEMP").replace("\\", "/");
		}
		if (cache_path_cache.is_empty()) {
			cache_path_cache = get_config_path();
		}
	}
	return cache_path_cache;
}

// Get properly capitalized engine name for system paths
String OS_Windows::get_godot_dir_name() const {
	return String(VERSION_SHORT_NAME).capitalize();
}

String OS_Windows::get_system_dir(SystemDir p_dir, bool p_shared_storage) const {
	KNOWNFOLDERID id;

	switch (p_dir) {
		case SYSTEM_DIR_DESKTOP: {
			id = FOLDERID_Desktop;
		} break;
		case SYSTEM_DIR_DCIM: {
			id = FOLDERID_Pictures;
		} break;
		case SYSTEM_DIR_DOCUMENTS: {
			id = FOLDERID_Documents;
		} break;
		case SYSTEM_DIR_DOWNLOADS: {
			id = FOLDERID_Downloads;
		} break;
		case SYSTEM_DIR_MOVIES: {
			id = FOLDERID_Videos;
		} break;
		case SYSTEM_DIR_MUSIC: {
			id = FOLDERID_Music;
		} break;
		case SYSTEM_DIR_PICTURES: {
			id = FOLDERID_Pictures;
		} break;
		case SYSTEM_DIR_RINGTONES: {
			id = FOLDERID_Music;
		} break;
	}

	PWSTR szPath;
	HRESULT res = SHGetKnownFolderPath(id, 0, nullptr, &szPath);
	ERR_FAIL_COND_V(res != S_OK, String());
	String path = String::utf16((const char16_t *)szPath).replace("\\", "/");
	CoTaskMemFree(szPath);
	return path;
}

String OS_Windows::get_user_data_dir() const {
	String appname = get_safe_dir_name(ProjectSettings::get_singleton()->get("application/config/name"));
	if (!appname.is_empty()) {
		bool use_custom_dir = ProjectSettings::get_singleton()->get("application/config/use_custom_user_dir");
		if (use_custom_dir) {
			String custom_dir = get_safe_dir_name(ProjectSettings::get_singleton()->get("application/config/custom_user_dir_name"), true);
			if (custom_dir.is_empty()) {
				custom_dir = appname;
			}
			return get_data_path().plus_file(custom_dir).replace("\\", "/");
		} else {
			return get_data_path().plus_file(get_godot_dir_name()).plus_file("app_userdata").plus_file(appname).replace("\\", "/");
		}
	}

	return get_data_path().plus_file(get_godot_dir_name()).plus_file("app_userdata").plus_file("[unnamed project]");
}

String OS_Windows::get_unique_id() const {
	HW_PROFILE_INFO HwProfInfo;
	ERR_FAIL_COND_V(!GetCurrentHwProfile(&HwProfInfo), "");
	return String::utf16((const char16_t *)(HwProfInfo.szHwProfileGuid), HW_PROFILE_GUIDLEN);
}

bool OS_Windows::_check_internal_feature_support(const String &p_feature) {
	return p_feature == "pc";
}

void OS_Windows::disable_crash_handler() {
	crash_handler.disable();
}

bool OS_Windows::is_disable_crash_handler() const {
	return crash_handler.is_disabled();
}

Error OS_Windows::move_to_trash(const String &p_path) {
	SHFILEOPSTRUCTW sf;

	Char16String utf16 = p_path.utf16();
	WCHAR *from = new WCHAR[utf16.length() + 2];
	wcscpy_s(from, utf16.length() + 1, (LPCWSTR)(utf16.get_data()));
	from[utf16.length() + 1] = 0;

	sf.hwnd = main_window;
	sf.wFunc = FO_DELETE;
	sf.pFrom = from;
	sf.pTo = nullptr;
	sf.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION;
	sf.fAnyOperationsAborted = FALSE;
	sf.hNameMappings = nullptr;
	sf.lpszProgressTitle = nullptr;

	int ret = SHFileOperationW(&sf);
	delete[] from;

	if (ret) {
		ERR_PRINT("SHFileOperation error: " + itos(ret));
		return FAILED;
	}

	return OK;
}

OS_Windows::OS_Windows(HINSTANCE _hInstance) {
	ticks_per_second = 0;
	ticks_start = 0;
	main_loop = nullptr;
	process_map = nullptr;

	force_quit = false;

	hInstance = _hInstance;
#ifdef STDOUT_FILE
	stdo = fopen("stdout.txt", "wb");
#endif

#ifdef WASAPI_ENABLED
	AudioDriverManager::add_driver(&driver_wasapi);
#endif
#ifdef XAUDIO2_ENABLED
	AudioDriverManager::add_driver(&driver_xaudio2);
#endif

	DisplayServerWindows::register_windows_driver();

	Vector<Logger *> loggers;
	loggers.push_back(memnew(WindowsTerminalLogger));
	_set_logger(memnew(CompositeLogger(loggers)));
}

OS_Windows::~OS_Windows() {
#ifdef STDOUT_FILE
	fclose(stdo);
#endif
}
