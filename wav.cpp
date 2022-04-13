#include "wav.h"
#include "wav_compression.h"
#include <QDataStream>
#include <string>
#include <stdexcept>
#include <memory>
#include <iostream>
#include <vector>

using std::unique_ptr;
using std::string;
using std::cout;
using std::endl;
using std::memcpy;
using std::vector;

WAV::WAV() {
    num_channels = sample_rate = byte_rate = block_align = bits_per_sample = data_size = 0;
}

WAVReadResult WAV::read(QFile& open_file) {
    // Read the file's data into memory.
    // open_file is expected to already have been opened for reading.
    QDataStream in(&open_file);

    // Extract the RIFF chunk descriptor.
    string chunk_id = "";
    char ch;
    for (int i = 0; i < 4; i++) {
        // TODO: Error handling/checking.
        in >> ch;
        chunk_id += ch;
    }
    if (chunk_id != "RIFF") {
        return WAVReadResult::not_riff;
    }
    for (char c : chunk_id) {
        header_bytes.push_back(c);
    }

    // in.setByteOrder(QDataStream::LittleEndian);
    quint32 chunk_size; // This won't be used. Just eat the bytes to get to next spot.
    in >> chunk_size;
    add_bytes(header_bytes, chunk_size);

    // in.setByteOrder(QDataStream::BigEndian);
    string format = "";
    for (int i = 0; i < 4; i++) {
        in >> ch;
        format += ch;
    }
    if (format != "WAVE") {
        return WAVReadResult::not_wav;
    }
    for (char c : format) {
        header_bytes.push_back(c);
    }

    // Extract the "fmt " subchunk.
    string subchunk1_id = "";
    for (int i = 0; i < 4; i++) {
        in >> ch;
        subchunk1_id += ch;
    }
    if (subchunk1_id != "fmt ") {
        return WAVReadResult::not_fmt;
    }
    for (char c : subchunk1_id) {
        header_bytes.push_back(c);
    }

    in.setByteOrder(QDataStream::LittleEndian);
    quint32 subchunk1_size;
    in >> subchunk1_size;
    if (subchunk1_size != 16) cout << "subchunk_1_size != 16" << endl;
    add_bytes(header_bytes, subchunk1_size);

    quint16 audio_format;
    in >> audio_format;
    if (audio_format != LPCM && audio_format != LOSSLESS_COMPRESSED) {
        return WAVReadResult::not_lpcm_or_lossless;
    }
    add_bytes(header_bytes, LOSSLESS_COMPRESSED); // In preparation for creating the compressed file.

    in >> num_channels;
    if (num_channels > 2 || num_channels < 1) {
        return WAVReadResult::not_mono_or_stereo;
    }
    add_bytes(header_bytes, num_channels);

    in >> sample_rate >> byte_rate >> block_align >> bits_per_sample;
    add_bytes(header_bytes, sample_rate);
    add_bytes(header_bytes, byte_rate);
    add_bytes(header_bytes, block_align);
    add_bytes(header_bytes, bits_per_sample);

    if (subchunk1_size > 16) {
        // Skip past these bytes by "eating" them.
        quint32 diff = 16 - subchunk1_size;
        for (quint32 i = 0; i < diff; i++) {
            in >> ch;
            add_bytes(header_bytes, ch);
        }
    }

    // Extract "data" subchunk header.
    read_data_header(in);

    bytes = unique_ptr<char[]>(new char[data_size]);
    // This method won't work on a Big Endian machine; however, all modern computers nowadays are Little Endian, so this is not
    // a problem.
    int res = in.readRawData(bytes.get(), data_size);
    if (bytes == nullptr || res == -1) throw std::runtime_error("bytes == nullptr or res == -1");

    if (audio_format == LOSSLESS_COMPRESSED) {
        // TODO: Don't just use any old type for compressed wav!!!
        decompress(CompressedWAV<qint16>(), *this);
    }

    return WAVReadResult::ok;
}

void WAV::read_data_header(QDataStream& in) {
    // Extract "data" subchunk header.
    string subchunk2_id = "";
    char ch;
    for (int i = 0; i < 4; i++) {
        in >> ch;
        subchunk2_id += ch;
    }
    for (char c : subchunk2_id) {
        header_bytes.push_back(c);
    }
    if (subchunk2_id != "data") {
        if (subchunk2_id == "LIST") {
            read_LIST_header(in);
            read_data_header(in);
        }
    }
    else {
        in >> data_size;
        add_bytes(header_bytes, data_size);
    }
}

void WAV::read_LIST_header(QDataStream& in) {
    quint32 size;
    in >> size;
    char ch;
    for (quint32 i = 0; i < size; i++) {
        in >> ch; // Eat the bytes.
        add_bytes(header_bytes, ch);
    }
}
