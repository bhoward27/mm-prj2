#include "lzw.h"
#include "wav_compression.h"
#include <iostream>

using std::string;
using std::vector;
using std::unordered_map;
using std::unique_ptr;
using std::memcpy;
using std::cout;
using std::endl;

unordered_map<string, code_t> new_dic(vector<string>& ordered_dic) {
    unordered_map<string, code_t> dic;
    for (quint16 i = 0; i <= 255; i++) {
        char ch = (char) i;
        string symbol = "";
        symbol += ch;
        code_t code = i;
        dic.insert({symbol, code});
        ordered_dic.push_back(symbol);
    }
    return dic;
}

void string_to_bytes(vector<char>& bytes, string s) {
    for (char byte : s) {
        bytes.push_back(byte);
    }
    bytes.push_back('\0');
}

vector<char> pickle(const vector<string>& ordered_dic, const vector<code_t>& codes, quint64& dic_num_bytes) {
    vector<char> bytes;
    for (string symbol : ordered_dic) {
        string_to_bytes(bytes, symbol);
    }
    dic_num_bytes = bytes.size();
    for (code_t code : codes) {
        for (unsigned long i = 0; i < sizeof(code); i++) {
            bytes.push_back(0);
        }
    }
    memcpy(&bytes.data()[dic_num_bytes], codes.data(), codes.size() * sizeof(codes[0]));
    return bytes;
}

vector<char> array_to_vector(const unique_ptr<char[]>& arr, quint32 len) {
    vector<char> new_vec(len);
    memcpy(new_vec.data(), arr.get(), len);
    return new_vec;
}

string bytes_to_string(const vector<char> bytes) {
    string s = "";
    for (char byte : bytes) {
        s += byte;
    }
    return s;
}

void encode(vector<code_t>& out_codes, const string& seq, unordered_map<string, code_t>& dic, vector<string>& ordered_dic) {
    if (seq == "") throw std::invalid_argument("No characters in sequence.");

    string s = seq.substr(0, 1);
    quint64 len = seq.size();
    code_t next_code = dic.size();
    for (quint64 i = 1; i < len; i++) {
        string ch = seq.substr(i, 1);
        string t = s + ch;
        auto res = dic.find(t);
        if (res != dic.end()) {
            s += ch;
        }
        else {
            auto res = dic.find(s);
            out_codes.push_back(res->second);
            dic.insert({t, next_code});
            ordered_dic.push_back(t);
            next_code++;
            s = ch;
        }
    }
    auto res = dic.find(s);
    out_codes.push_back(res->second);
}

string decode(const vector<code_t>& codes, const vector<string>& full_dic) {
    string seq = "";
    for (auto code : codes) {
        seq += full_dic[code];
    }
    return seq;
}
