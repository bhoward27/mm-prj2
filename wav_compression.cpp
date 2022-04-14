#include "wav_compression.h"

quint8 next_byte(int& n, quint64 bits) {
    if (n < 8) throw std::runtime_error("n must be >= 8.");
    quint64 mask = 0xFFu << (n - 8);
    quint8 byte = (bits & mask) >> (n - 8);
    n -= 8;
    return byte;
}

void save_pending_bytes(vector<quint8>& bytes, int& n, quint64 bits) {
    while (n >= 8) bytes.push_back(next_byte(n, bits));
}

void add_bits(int& n, quint64& bits, quint64 num_new_bits, quint64 x) {
    bits <<= num_new_bits;
    bits += x;
    n += num_new_bits;
}
