#ifndef LZW_H
#define LZW_H

#include <QtGlobal>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "wav_compression.h"

typedef quint32 code_t;

std::unordered_map<std::string, code_t> new_dic(std::vector<std::string>& ordered_dic);
void string_to_bytes(std::vector<char>& bytes, std::string s);
std::vector<char> pickle(const std::vector<std::string>& ordered_dic, const std::vector<code_t>& codes, quint64& dic_num_bytes);
std::vector<char> array_to_vector(const std::unique_ptr<char[]>& arr, quint32 len);
std::string bytes_to_string(const std::vector<char> bytes);
void encode(std::vector<code_t>& out_codes, const std::string& seq, std::unordered_map<std::string, code_t>& dic, std::vector<std::string>& ordered_dic);
std::string decode(const std::vector<code_t>& codes, const std::vector<std::string>& full_dic);
template<class T>
void lzw_compress(const std::unique_ptr<char[]>& samples_ptr, quint32 n, CompressedWAV<T>& out_c_wav) {
    std::vector<string> ordered_dic;
    auto dic = new_dic(ordered_dic);

    auto bytes = array_to_vector(samples_ptr, n);
    auto seq = bytes_to_string(bytes);
    std::vector<code_t> codes;
    encode(codes, seq, dic, ordered_dic);
    quint64 dic_num_bytes;
    auto encoded_bytes = pickle(ordered_dic, codes, dic_num_bytes);

    std::cout << "ordered_dic.size() == " << ordered_dic.size() << std::endl;
    std::cout << "bytes.size() == " << bytes.size() << std::endl;
    std::cout << "encoded_bytes.size() == " << encoded_bytes.size() << std::endl;
    std::cout << "Compression ratio = " << ((double) bytes.size() / encoded_bytes.size()) * 100 << "%" << std::endl;
    std::cout << "Compression ratio without dictionary = " << ((double) bytes.size() / (encoded_bytes.size() - dic_num_bytes)) * 100 << "%" << std::endl;
    std::cout << std::endl;
}

#endif // LZW_H
