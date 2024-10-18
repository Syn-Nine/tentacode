#ifndef UTILITY_H
#define UTILITY_H

#include <sstream>
#include <iomanip>
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


std::string GenerateUUID()
{
    unsigned char bytes[16];

    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    for (unsigned i = 0; i < 16; i++)
    {
        bytes[i] = rand() % 256;
        if (i == 0) bytes[i] = bytes[i] & 0x0f;
        if (i == 6) bytes[i] = 0x40 | (bytes[i] & 0x0F);
        if (i == 8) bytes[i] = 0x80 | (bytes[i] & 0x3F);

        ss << std::setw(2) << static_cast<unsigned>(bytes[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9) ss << '-';
    }

    return ss.str();
}

#endif