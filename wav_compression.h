#ifndef WAV_COMPRESSION_H
#define WAV_COMPRESSION_H

#include <QtGlobal>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <vector>
#include <string>
#include <array>
#include <stdexcept>
#include <cmath>
#include <iostream>
#include "wav.h"

using std::invalid_argument;
using std::array;
using std::vector;
using std::string;
using std::cout;

template<class T>
struct Frame {
    bool is_constant;
    quint8 p;
    vector<T> first_p_samples;
    vector<T> residuals;
    quint8 m;
};

template<class T>
struct CompressedWAV {
    quint32 n;
    quint16 num_samples_per_frame;
    quint16 bit_depth;
    vector<Frame<T>> frames;
    quint8 num_channels;

    // These bytes shall be the final output for the compressed audio file.
    vector<quint8> bytes;
};

// const quint16 NUM_SAMPLES_PER_FRAME = 1152;
//const quint16 NUM_SAMPLES_PER_FRAME = 576;
// const quint16 NUM_SAMPLES_PER_FRAME = 2304;
// const quint16 NUM_SAMPLES_PER_FRAME = 4608;
const quint32 NUM_SAMPLES_PER_FRAME = 1152 * 128;
const quint8 NUM_PREDICTORS = 4;

template<class T>
T predict(quint8 p, quint32 n, const vector<T>& x_hat) {
    if (n >= x_hat.size()) throw invalid_argument("n must be less than x.size()");
    switch (p) {
        case 0:
            return 0;
            break;
        case 1:
            return x_hat[n - 1];
            break;
        case 2:
            return 2*x_hat[n - 1] - x_hat[n - 2];
            break;
        case 3:
            return 3*x_hat[n - 1] - 3*x_hat[n - 2] + x_hat[n - 3];
            break;
        default:
            // Do nothing.
           break;
    }
}

template<class T>
void init_predictions(quint8 p, quint32 n, vector<T>& x_hat, const T* samples) {
    // TODO: What if n is not divisible by num_samples_per_frame? Then this access may go out of bounds?
    for (quint32 j = 0; j < n; j += NUM_SAMPLES_PER_FRAME) {
        for (quint32 i = 0; i < p; i++) {
            x_hat[j + i] = samples[j + i];
        }
    }
}



// Function assumes n >= 8.
quint8 next_byte(int& n, quint64 bits);

void save_pending_bytes(vector<quint8>& bytes, int& n, quint64 bits);

void add_bits(int& n, quint64& bits, quint64 num_new_bits, quint64 x);

template<class T>
void encode(CompressedWAV<T>& out_c_wav) {
    static long long num_times_zero_was_big = 0;
    vector<quint8> bytes;

    // Encode each frame.
    int n = 0;
    quint64 bits = 0;
    for (auto frame : out_c_wav.frames) {
        auto p = frame.p;
        auto m = frame.m;
        add_bits(n, bits, 1, frame.is_constant);
        if (frame.is_constant) {
            add_bits(n, bits, out_c_wav.bit_depth, frame.first_p_samples[0]);
            save_pending_bytes(bytes, n, bits);
        }
        else {
            add_bits(n, bits, 2, p);
            add_bits(n, bits, 6, m);
            save_pending_bytes(bytes, n, bits);

            // Store the first p samples into bytes.
            for (int i = 0; i < p; i++) {
                add_bits(n, bits, out_c_wav.bit_depth, frame.first_p_samples[i]);
                save_pending_bytes(bytes, n, bits);
            }

            // Rice Encoding.
            // Create a mask with m 1s in the least significant bits.
            quint64 m_mask = 0;
            for (int i = 0; i < m; i++) {
                m_mask <<= 1;
                m_mask++;
            }
            // Encode each residual.
            for (auto res : frame.residuals) {
                quint8 sign = (res >= 0) ? 0 : 1;
                quint8 m_bits = res & m_mask;
                quint64 num_zeros = (abs(res) & ~m_mask) >> m;// (sign) ? -((res & ~m_mask) >> m) : (res & ~m_mask) >> m;
                if (num_zeros > 50) {
                    string msg = " -- num_zeros is getting large! num_zeros = ";
//                    msg += std::to_string(num_zeros);
//                    // throw std::runtime_error(msg);
//                    cout << num_times_zero_was_big++ << msg << std::endl;
                }

                add_bits(n, bits, 1, sign);
                add_bits(n, bits, m, m_bits);
                add_bits(n, bits, num_zeros + 1, 1);

                save_pending_bytes(bytes, n, bits);
            }
        }
    }
    save_pending_bytes(bytes, n, bits);
    if (n > 0) {
        // Pad last bits with (8 - n) 0s at the end.
        add_bits(n, bits, 8 - n, 0);
        save_pending_bytes(bytes, n, bits);
    }
    out_c_wav.bytes = bytes;
}

template<class T>
void write(const WAV& wav, const CompressedWAV<T>& c_wav) {
    QString name = wav.file_name;
    name.insert(wav.file_name.size() - 4, " - compressed");
    QFile file(name);
    file.open(QIODevice::Append);
    QDataStream out(&file);
    out.writeRawData((const char*) wav.header_bytes.data(), wav.header_bytes.size());
    out.writeRawData((const char*) c_wav.bytes.data(), c_wav.bytes.size());
    file.close();
}

template<class T>
void decompress(const CompressedWAV<T>& c_wav, WAV& out_wav) {

}

template<class T>
void compress(const std::unique_ptr<char[]>& samples_ptr, quint32 n, CompressedWAV<T>& out_c_wav) {
    const T* samples = (const T*) samples_ptr.get();
    array<vector<T>, NUM_PREDICTORS> p;
    for (quint8 i = 0; i < NUM_PREDICTORS; i++) {
        p[i] = vector<T>(n);
        init_predictions(i, n, p[i], samples);
        for (quint32 start_of_frame = 0; start_of_frame < n; start_of_frame += NUM_SAMPLES_PER_FRAME) {
            quint32 start_of_next_frame = start_of_frame + NUM_SAMPLES_PER_FRAME;
            for (quint32 j = start_of_frame + i; j < start_of_next_frame && j < n; j++) {
                p[i][j] = predict(i, j, p[i]);
                if (i > 0) {
                    string x = "do a dance.";
                }
            }
        }
    }

    array<vector<T>, NUM_PREDICTORS> e;
    for (quint8 i = 0; i < NUM_PREDICTORS; i++) {
        e[i] = vector<T>(n);
        for (quint32 j = 0; j < n; j++) {
            e[i][j] = samples[j] - p[i][j];
        }
    }

    // Determine which predictor to use for each frame.
    for (quint32 i = 0; i < n; i += NUM_SAMPLES_PER_FRAME) {
        quint32 start_of_next_frame = i + NUM_SAMPLES_PER_FRAME;
        array<quint64, 3> min_err = {0, LONG_MAX, 0};
        for (quint8 j = 0; j < NUM_PREDICTORS; j++) {
            quint64 err_sum = 0;
            for (quint32 k = i; k < start_of_next_frame && k < n; k++) {
                err_sum += abs(e[j][k]);
            }
            if (err_sum < min_err[1]) {
                min_err[0] = j; // p
                min_err[1] = err_sum;
                double temp = ceil(log2(log(2.0) * ((double) err_sum / NUM_SAMPLES_PER_FRAME)));
                min_err[2] = (quint8) temp; // m
                if (temp > 10) { // This never occurs. Log grows very slowly.
                    string s = "hi";
                }
            }
        }
        Frame<T> frame;
        frame.is_constant = false;// (min_err[0] == 0);
        if (frame.is_constant) {
            frame.first_p_samples = {samples[i]};
        }
        else {
            frame.p = min_err[0];
            frame.m = min_err[2];
            vector<T> first_p_samples;
            for (quint32 j = 0, k = i; j < frame.p; j++, k++) {
                first_p_samples.push_back(samples[k]);
            }
            vector<T> residuals;
            for (quint32 j = frame.p; j < start_of_next_frame; j++) {
                residuals.push_back(e[frame.p][j]);
            }
            frame.first_p_samples = first_p_samples;
            frame.residuals = residuals;
        }
        out_c_wav.frames.push_back(frame);
    }
    encode(out_c_wav);
}

// Kinda weird to do this since we have T already.
template<class T>
void compress(const WAV& wav, CompressedWAV<T>& out_c_wav) {
    out_c_wav.num_channels = wav.num_channels;
    switch (wav.bits_per_sample) {
        case 8:
            out_c_wav.bit_depth = 8;
            compress(wav.bytes, wav.data_size, out_c_wav);
            break;
        case 16:
            out_c_wav.bit_depth = 16;
            compress(wav.bytes, wav.data_size/2, out_c_wav);
            break;
    }
    write(wav, out_c_wav);
}

#endif // WAV_COMPRESSION_H
