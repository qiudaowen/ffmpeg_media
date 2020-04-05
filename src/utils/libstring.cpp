#include "libstring.h"
#include <Windows.h>
#include <string>
#include <regex>
#include <map>


namespace libstring 
{
    std::wstring fromMultiByte(uint32_t codePage, const char *str, int size /*= -1*/)
    {
        std::wstring wstr;
        if (size < 0)
        {
            size = (int)strlen(str);
        }
        int bytesNeed = MultiByteToWideChar(codePage, 0, str, size, 0, 0);
        wstr.resize(bytesNeed);
        MultiByteToWideChar(codePage, 0, str, size, const_cast<wchar_t *>(wstr.c_str()), bytesNeed);
        return wstr;
    }

    std::string toMultiByte(uint32_t codePage, const wchar_t *wstr, int size /*= -1*/)
    {
        std::string str;
        if (size < 0)
        {
            size = (int)wcslen(wstr);
        }
        int bytesNeed = WideCharToMultiByte(codePage, NULL, wstr, size, NULL, 0, NULL, FALSE);
        str.resize(bytesNeed);
        WideCharToMultiByte(codePage, NULL, wstr, size, const_cast<char *>(str.c_str()), bytesNeed, NULL, FALSE);
        return str;
    }

    std::wstring fromUtf8(const char *str, int size /*= -1*/)
    {
        return fromMultiByte(CP_UTF8, str, size);
    }

    std::wstring fromUtf8(const std::string &str)
    {
        return fromUtf8(str.c_str(), (int)str.length());
    }

    std::wstring fromLocal8Bit(const char *str, int size /*= -1*/)
    {
        return fromMultiByte(CP_ACP, str, size);
    }

    std::wstring fromLocal8Bit(const std::string &str)
    {
        return fromLocal8Bit(str.c_str(), (int)str.length());
    }

    std::string toLocal8Bit(const wchar_t *wstr, int size /*= -1*/)
    {
        return toMultiByte(CP_ACP, wstr, size);
    }

    std::string toLocal8Bit(const std::wstring &str)
    {
        return toLocal8Bit(str.c_str(), (int)str.length());
    }

    std::string toUtf8(const wchar_t *wstr, int size /*= -1*/)
    {
        return toMultiByte(CP_UTF8, wstr, size);
    }

    std::string toUtf8(const std::wstring &str)
    {
        return toUtf8(str.c_str(), (int)str.length());
    }

    std::string utf8ToLocal8Bit(const std::string &str)
    {
        return toLocal8Bit(fromUtf8(str));
    }

    std::string local8BitToUtf8(const std::string &str)
    {
        return toUtf8(fromLocal8Bit(str));
    }

    bool contains(const std::wstring& str, const std::wstring& substr)
    {
        std::size_t found = str.find(substr);
        if (found == std::string::npos)
            return false;
        else
            return true;
    }
    bool contains(const std::string& str, const std::string& substr)
    {
        auto found = str.find(substr);
        if (found == std::string::npos)
            return false;
        else
            return true;
    }
    bool startsWith(const std::wstring& str, const std::wstring& with)
    {
        return str.compare(0, with.size(), with) == 0 ? true : false;
    }
    bool startsWith(const std::string& str, const std::string& with)
    {
        return str.compare(0, with.size(), with) == 0 ? true : false;
    }
    bool endsWith(const std::wstring& str, const std::wstring& with)
    {
        if (str.size() < with.size())
            return false;
        return str.compare(str.size() - with.size(), with.size(), with) == 0 ? true : false; ;
    }
    bool endsWith(const std::string& str, const std::string& with)
    {
        if (str.size() < with.size())
            return false;
        return str.compare(str.size() - with.size(), with.size(), with) == 0 ? true : false; ;
    }
    std::wstring replace(const std::wstring& str, const std::wstring& search, const std::wstring& replace)
    {
        std::wstring subject = str;
        size_t pos = 0;
        while ((pos = subject.find(search, pos)) != std::wstring::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
        return subject;
    }
    std::string replace(const std::string& str, const std::string& search, const std::string& replace)
    {
        auto subject = str;
        size_t pos = 0;
        while ((pos = subject.find(search, pos)) != std::string::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
        return subject;
    }
    std::string regex_replace(const std::string& str, const char* search, const char* replace)
    {
        return std::regex_replace(str, std::regex(search), replace);
    }
    std::wstring regex_replace(const std::wstring& str, const wchar_t* search, const wchar_t* replace)
    {
        return std::regex_replace(str, std::wregex(search), replace);
    }

    std::vector<std::string> c_split(const char* in, const char* delim) {
        std::regex re{ delim };
        return std::vector<std::string> {
            std::cregex_token_iterator(in, in + strlen(in), re, -1),
                std::cregex_token_iterator()
        };
    }
    std::vector<std::wstring> c_split(const wchar_t* in, const wchar_t* delim) {
        std::wregex re{ delim };
        return std::vector<std::wstring> {
            std::wcregex_token_iterator(in, in + wcslen(in), re, -1),
                std::wcregex_token_iterator()
        };
    }

	std::wstring& trim(std::wstring &s) {
		if (s.empty()) {
			return s;
		}
		std::wstring character = L"";
		for (int i = 0; i < 33; i++) {
			character += wchar_t(i);
		}
		character += wchar_t(127);
		s.erase(0, s.find_first_not_of(character));
		s.erase(s.find_last_not_of(character) + 1);
		return s;
	}
	std::string& trim(std::string &s) {
		if (s.empty()) {
			return s;
		}
		std::string character = "";
		for (int i = 0; i < 33; i++) {
			character += char(i);
		}
		character += char(127);
		s.erase(0, s.find_first_not_of(character));
		s.erase(s.find_last_not_of(character) + 1);
		return s;
	}

    std::map<std::string, std::string> parseKeyValue(const char* str, const char* part1, const char* part2)
    {
        std::vector<std::string> keyValueList = c_split(str, part1);
        std::map<std::string, std::string> keyValueMap;
        for (const auto& item : keyValueList)
        {
            std::vector<std::string> keyValue = c_split(item.c_str(), part2);
            if (keyValue.size() == 0)
                continue;
            else if (keyValue.size() == 1)
                keyValueMap[trim(keyValue[0])] = std::string();
            else if (keyValue.size() == 2)
                keyValueMap[trim(keyValue[0])] = keyValue[1];
        }
        return keyValueMap;
    }
    std::map<std::wstring, std::wstring> parseKeyValue(const wchar_t* str, const wchar_t* part1, const wchar_t* part2)
    {
        std::vector<std::wstring> keyValueList = c_split(str, part1);
        std::map<std::wstring, std::wstring> keyValueMap;
        for (const auto& item : keyValueList)
        {
            std::vector<std::wstring> keyValue = c_split(item.c_str(), part2);
            if (keyValue.size() == 0)
                continue;
            else if (keyValue.size() == 1)
                keyValueMap[trim(keyValue[0])] = std::wstring();
            else if (keyValue.size() == 2)
                keyValueMap[trim(keyValue[0])] = keyValue[1];
        }
        return keyValueMap;
    }
}

