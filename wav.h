#ifndef WAV_H
#define WAV_H

#include <QtGlobal>
#include <QFile>
#include <memory>
#include <vector>

const quint16 LPCM = 1;
const quint16 LOSSLESS_COMPRESSED = 77;

enum class WAVReadResult {
    not_riff,
    not_wav,
    not_fmt,
    not_lpcm_or_lossless,
    not_mono_or_stereo,
    not_data,
    ok
};

class WAV {
    public:
        QString file_name;
        quint16 num_channels;
        quint32 sample_rate;
        quint32 byte_rate;
        quint16 block_align;
        quint16 bits_per_sample;
        quint32 data_size;
        std::unique_ptr<char[]> bytes;
        std::vector<quint8> header_bytes;

        WAV();
        WAVReadResult read(QFile& open_file);
        void read_data_header(QDataStream& in);
        void read_LIST_header(QDataStream& in);

        // Append x to the byte buffer, bytes.
        template<class T>
        void add_bytes(std::vector<quint8>& bytes, T x) {
            for (unsigned long i = 0; i < sizeof(x); i++) {
                bytes.push_back(0);
            }
            memcpy(&bytes.data()[bytes.size() - sizeof(x)], &x, sizeof(x));
        }
};

#endif // WAV_H
