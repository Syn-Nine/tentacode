#ifndef UTILITY_H
#define UTILITY_H

#include <vector>
#include <string>

static std::string StrJoin(std::vector<std::string> s, std::string delimiter) {
    std::string ret;
    for (size_t i = 0; i < s.size(); ++i)
    {
        ret.append(s[i]);
        if (i != s.size()-1) { ret.append(delimiter); }
    }
    return ret;
}

// source: https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
static std::vector<std::string> StrSplit(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}


// based on https://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
static std::string StrReplace(std::string str, const std::string& from, const std::string& to) {
	if(from.empty()) return str;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
	return str;
}

#endif