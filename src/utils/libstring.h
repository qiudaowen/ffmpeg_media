#pragma once

#include <stdint.h>
#include <string>
#include <map>
#include <vector>

namespace libstring 
{
	std::wstring fromMultiByte(uint32_t codePage, const char *str, int size /*= -1*/);
	std::string toMultiByte(uint32_t codePage, const wchar_t *wstr, int size /*= -1*/);
	std::wstring fromUtf8(const char *str, int size /*= -1*/);
	std::wstring fromUtf8(const std::string &str);
	std::string toUtf8(const wchar_t *wstr, int size /*= -1*/);
	std::string toUtf8(const std::wstring &str);
	std::wstring fromLocal8Bit(const char *str, int size /*= -1*/);
	std::wstring fromLocal8Bit(const std::string &str);
	std::string toLocal8Bit(const wchar_t *wstr, int size /*= -1*/);
	std::string toLocal8Bit(const std::wstring &str);
	std::string utf8ToLocal8Bit(const std::string &str);
	std::string local8BitToUtf8(const std::string &str);

	bool contains(const std::wstring& str, const std::wstring& substr);
	bool contains(const std::string& str, const std::string& substr);
	bool startsWith(const std::wstring& str, const std::wstring& with);
	bool startsWith(const std::string& str, const std::string& with);
	bool endsWith(const std::wstring& str, const std::wstring& with);
	bool endsWith(const std::string& str, const std::string& with);
	std::wstring replace(const std::wstring& str, const std::wstring& search, const std::wstring& replace);
	std::string replace(const std::string& str, const std::string& search, const std::string& replace);
	std::string regex_replace(const std::string& str, const char* search, const char* replace);
	std::wstring regex_replace(const std::wstring& str, const wchar_t* search, const wchar_t* replace);

	std::vector<std::string> c_split(const char* in, const char* delim, bool includeEmpty = false);
	std::vector<std::wstring> c_split(const wchar_t* in, const wchar_t* delim, bool includeEmpty = false);

	std::wstring& trim(std::wstring &s);
	std::string& trim(std::string &s);
	std::map<std::string, std::string> parseKeyValue(const char* str, const char* part1 = ",", const char* part2 = "=");
	std::map<std::wstring, std::wstring> parseKeyValue(const wchar_t* str, const wchar_t* part1 = L",", const wchar_t* part2 = L"=");
}

