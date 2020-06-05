#pragma once

template<class T>
struct ciLessLibC : public std::binary_function<std::string, T, bool> {
    bool operator()(const std::string &lhs, const std::string &rhs) const {
        return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
    }
};

template<class T>
class CaseInsensitiveMap : public std::map<std::string, T, ciLessLibC<T>> 
{
public:
    bool contains(const std::string& key)
    {
        return find(key) != end();
    }
    const T value(const Key &key, const T &defaultValue = T()) const
    {
        auto iter = find(key);
        if (iter != end())
            return iter->second;
        return defaultValue;
    }
};
