#include "wav.h"
#include <QDataStream>
#include <string>
#include <stdexcept>
#include <memory>
#include <iostream>

using std::unique_ptr;
using std::string;
using std::cout;
using std::endl;

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

    // in.setByteOrder(QDataStream::LittleEndian);
    quint32 chunk_size; // This won't be used. Just eat the bytes to get to next spot.
    in >> chunk_size;

    // in.setByteOrder(QDataStream::BigEndian);
    string format = "";
    for (int i = 0; i < 4; i++) {
        in >> ch;
        format += ch;
    }
    if (format != "WAVE") {
        return WAVReadResult::not_wav;
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

    in.setByteOrder(QDataStream::LittleEndian);
    quint32 subchunk1_size;
    in >> subchunk1_size;
    if (subchunk1_size != 16) cout << "subchunk_1_size != 16" << endl;

    quint16 audio_format;
    in >> audio_format;
    if (audio_format != LPCM) {
        return WAVReadResult::not_lpcm;
    }

    in >> num_channels;
    if (num_channels > 2 || num_channels < 1) {
        return WAVReadResult::not_mono_or_stereo;
    }

    in >> sample_rate >> byte_rate >> block_align >> bits_per_sample;
    if (subchunk1_size > 16) {
        // Skip past these bytes by "eating" them.
        quint32 diff = 16 - subchunk1_size;
        for (quint32 i = 0; i < diff; i++) {
            in >> ch;
        }
    }

    // Extract "data" subchunk header.
    read_data_header(in);

    bytes = unique_ptr<char[]>(new char[data_size]);
    // This method won't work on a Big Endian machine; however, all modern computers nowadays are Little Endian, so this is not
    // a problem.
    int res = in.readRawData(bytes.get(), data_size);
    if (bytes == nullptr || res == -1) throw std::runtime_error("bytes == nullptr or res == -1");

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
    if (subchunk2_id != "data") {
        if (subchunk2_id == "LIST") {
            read_LIST_header(in);
            read_data_header(in);
        }
    }
    else {
        in >> data_size;
    }
}

void WAV::read_LIST_header(QDataStream& in) {
    quint32 size;
    in >> size;
    char ch;
    for (quint32 i = 0; i < size; i++) {
        in >> ch; // Eat the bytes.
    }
}
