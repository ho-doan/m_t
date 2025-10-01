#pragma once
#include <string>
#include <Windows.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <sstream>
#include <winrt/Windows.Storage.h>

#include <winrt/Windows.Data.Json.h>
#include <unordered_map>
#include <iostream>
#include <any>
#include <fstream>

#include <regex>

using namespace winrt;
using namespace Windows::Storage;

static inline std::wstring _get_path_app()
{
	wchar_t executablePath[MAX_PATH];

	// Get the full path of the executable to ensure it's quoted properly
	if (GetModuleFileNameW(NULL, executablePath, MAX_PATH) == 0)
	{
		std::wcerr << L"GetModuleFileName failed: " << GetLastError() << std::endl;
		return L"";
	}

	return std::wstring(executablePath);
}

static inline std::vector<std::wstring> _split(const std::wstring &str, const wchar_t delimiter)
{
	std::vector<std::wstring> tokens;
	std::wstring token;
	std::wistringstream tokenStream(str);

	while (std::getline(tokenStream, token, delimiter))
	{
		tokens.push_back(token);
	}

	return tokens;
}

static inline std::vector<std::wstring> _split(const std::wstring& str, const std::wstring& delimiter)
{
	std::vector<std::wstring> tokens;
	std::wistringstream tokenStream(str);

	size_t start = 0, end;

	while ((end = str.find(delimiter, start)) != std::wstring::npos)
	{
		tokens.push_back(str.substr(start, end - start));
		start = end + delimiter.length();
	}
	tokens.push_back(str.substr(start));

	return tokens;
}

static inline std::wstring get_current_path()
{
	auto path = _get_path_app();
	std::vector<std::wstring> segments = _split(path, L'\\');

	if (!segments.empty())
	{
		segments.pop_back();
	}

	std::wstring result;
	for (size_t i = 0; i < segments.size(); ++i)
	{
		result += segments[i];
		if (i < segments.size() - 1)
		{
			result += L'\\';
		}
	}

	return result;
}

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
										"abcdefghijklmnopqrstuvwxyz"
										"0123456789+/";

static inline std::string base64_encode(const std::string& in) {
	std::string out;
	int val = 0, valb = -6;
	for (unsigned char c : in) {
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0) {
			out.push_back(base64_chars[(val >> valb) & 0x3F]);
			valb -= 6;
		}
	}
	if (valb > -6)out.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
	while (out.size() % 4)out.push_back('=');
	return out;
}

static inline std::string base64_decode(const std::string& in) {
	std::string out;
	std::vector<int>T(256, -1);
	for (int i = 0; i < 64; i++) 
		T[base64_chars[i]] = i;
	int val = 0, valb = -8;
	for (unsigned char c : in) {
		if (T[c] == -1)break;
		val = (val << 6) + T[c];
		valb += 6;
		if (valb >= 0) {
			out.push_back(char((val >> valb) & 0xFF));
			valb -= 8;
		}
	}
	return out;
}

static inline std::wstring _cp_to_wide(const std::string &s, UINT codepage)
{
	int in_length = (int)s.length();
	int out_length = MultiByteToWideChar(codepage, 0, s.c_str(), in_length, 0, 0);
	std::wstring result(out_length, L'\0');
	if (out_length)
		MultiByteToWideChar(codepage, 0, s.c_str(), in_length, &result[0], out_length);
	return result;
}
static inline std::wstring utf8_to_wide(const std::string &s)
{
	return _cp_to_wide(s, CP_UTF8);
}

static inline std::wstring utf8_to_wide_2(const std::string *s)
{
	return std::wstring(s->begin(), s->end());
}

static inline std::string _wide_to_cp(const std::wstring &s, UINT codepage)
{
	int in_length = (int)s.length();
	int out_length = WideCharToMultiByte(codepage, 0, s.c_str(), in_length, 0, 0, 0, 0);
	std::string result(out_length, '\0');
	if (out_length)
		WideCharToMultiByte(codepage, 0, s.c_str(), in_length, &result[0], out_length, 0, 0);
	return result;
}
static inline std::string wide_to_utf8(const std::wstring &s)
{
	return _wide_to_cp(s, CP_UTF8);
}

static inline std::wstring ConvertMapToJSONString(
        const std::map<std::wstring, std::wstring>& data,
        int systemType)
{
    std::wstringstream jsonStream;
    jsonStream << L"{";

    bool firstEntry = true;
    for (const auto& pair : data)
    {
        if (!firstEntry)
        {
            jsonStream << L", ";
        }

        jsonStream << L"\"" << pair.first << L"\": ";

        if (pair.first == L"SystemType")
        {
            jsonStream << systemType;
        }
        else
        {
            jsonStream << L"\"" << pair.second << L"\"";
        }

        firstEntry = false;
    }

    jsonStream << L"}";
    return jsonStream.str();
}

static inline std::unordered_map<std::wstring, std::any> json_to_map(const winrt::Windows::Data::Json::JsonObject &jsonObject)
{
	std::unordered_map<std::wstring, std::any> map;

	for (const auto &keyValue : jsonObject)
	{
		const auto key = utf8_to_wide(winrt::to_string(keyValue.Key()));
		const auto &value = keyValue.Value();

		// Assuming you want to convert all values to string for the map
		if (value.ValueType() == winrt::Windows::Data::Json::JsonValueType::String)
		{
			map[key] = value.GetString();
		}
		else if (value.ValueType() == winrt::Windows::Data::Json::JsonValueType::Number)
		{
			map[key] = value.GetNumber();
		}
	}

	return map;
}

static inline BOOL write_pid(HWND pid)
{
	std::wstringstream ss;
	ss << L"0x" << std::hex << reinterpret_cast<uintptr_t>(pid);
	std::wstring buf = ss.str();
	std::wstring path = get_current_path() + L"\\app_system.ini";
	std::ifstream fileCheck(path);
	if (!fileCheck.good())
	{
		std::ofstream outFile(path);
		if (outFile)
		{
			outFile.close();
		}
	}
	return WritePrivateProfileString(L"process", L"pid", buf.c_str(), path.c_str());
}

static inline void write_error()
{
	std::wstring s = L"Error: " + std::to_wstring(GetLastError()) + L"\n================ end ======== \n";
	std::wstring path = get_current_path() + L"\\crash.txt";
	std::wofstream file(path, std::ios::app);
	file << s;
	file.close();
}
static inline void write_error(const std::exception& ex, int line)
{
	std::wstring path = get_current_path() + L"\\crash.txt";
	std::wofstream file(path, std::ios::app);
	file << L"Error: " << line << ex.what() << L"\n================ end ======== \n";
	file.close();
}
static inline void write_log(const int s)
{
	std::wstring path = get_current_path() + L"\\crash.txt";
	std::wofstream file(path, std::ios::app);
	file << L"Log: " << s << L"\n================ end ======== \n";
	file.close();
}
static inline void write_log(const std::string& s)
{
	std::wstring path = get_current_path() + L"\\crash.txt";
	std::wofstream file(path, std::ios::app);
	file << L"Log: " << utf8_to_wide(s) << L"\n================ end ======== \n";
	file.close();
}
static inline void write_log(const std::wstring& s)
{
	std::wcout << "Log: " << s << "\n";
	OutputDebugStringW(s.c_str());
	std::wstring path = get_current_path() + L"\\crash.txt";
	std::wofstream file(path, std::ios::app);
	file << L"Log: " << s << L"\n================ end ======== \n";
	file.close();
}
static inline void write_log(const std::wstring& s,hstring ss)
{
	std::wstringstream sss;
	sss << s << ":" << ss << "\n";
	std::wcout << "Log: " << sss.str() << "\n";
	OutputDebugStringW(sss.str().c_str());
	std::wstring path = get_current_path() + L"\\crash.txt";
	std::wofstream file(path, std::ios::app);
	file << L"Log: " << s << ss << L"\n================ end ======== \n";
	file.close();
}
static inline void write_log(const std::wstring& s,std::wstring& ss)
{
	std::wstringstream sss;
	sss << s << ":" << ss << "\n";
	std::wcout << "Log: " << sss.str() << "\n";
	OutputDebugStringW(sss.str().c_str());
	std::wstring path = get_current_path() + L"\\crash.txt";
	std::wofstream file(path, std::ios::app);
	file << L"Log: " << s << ss << L"\n================ end ======== \n";
	file.close();
}

static inline HWND read_pid()
{
	std::wstring path = get_current_path() + L"\\app_system.ini";
	wchar_t buffer[256] = { 0 };

	GetPrivateProfileString(L"process", L"pid", L"", buffer, 256, path.c_str());

	if (wcslen(buffer) == 0) return NULL;
	uintptr_t handleValue = 0;
	swscanf_s(buffer, L"%Ix", &handleValue);
	return reinterpret_cast<HWND>(handleValue);
}

static inline std::wstring get_sys_device_id() {
	HKEY hKey;
	const wchar_t* subkey = L"SOFTWARE\\Microsoft\\SQMClient"; //SOFTWARE\\Microsoft\\Cryptography
	const wchar_t* valueName = L"MachineId";//MachineGuid
	wchar_t machineGuid[64];
	DWORD size = sizeof(machineGuid);

	// Open the registry key
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subkey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		// Query the MachineGuid value from the registry
		if (RegQueryValueExW(hKey, valueName, nullptr, nullptr, (BYTE*)machineGuid, &size) == ERROR_SUCCESS) {
			RegCloseKey(hKey);
			std::string str = wide_to_utf8(machineGuid);

			str = std::regex_replace(str, std::regex("\\{|\\}"), "");
			return utf8_to_wide(str);
		}
		RegCloseKey(hKey);
	}
	return L"Error";
}