#include "wav_compression.h"
#include "wav.h"
#include <array>
#include <vector>
#include <stdexcept>
#include <cmath>

using std::invalid_argument;
using std::array;
using std::vector;

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

    // These bytes shall be the final output for the compressed audio file.
    vector<quint8> bytes;
};

const quint16 NUM_SAMPLES_PER_FRAME = 1152;
const quint8 NUM_PREDICTORS = 4;

template<class T>
T predict(quint8 p, quint16 n, const vector<T>& x_hat) {
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

template<class T>
void compress(const T* samples, quint32 n, CompressedWAV<T>& out_c_wav) {
    array<vector<T>, NUM_PREDICTORS> p;
    for (quint8 i = 0; i < NUM_PREDICTORS; i++) {
        p[i] = vector<T>(n);
        init_predictions(i, n, p[i], samples);
        for (quint32 start_of_frame = 0; start_of_frame < n; start_of_frame += NUM_SAMPLES_PER_FRAME) {
            quint32 start_of_next_frame = start_of_frame += NUM_SAMPLES_PER_FRAME;
            for (quint32 j = start_of_frame + i + 1; j < start_of_next_frame; j++) {
                p[i][j] = predict(i, j, p[i]);
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
            for (quint32 k = i; k < start_of_next_frame; k++) {
                err_sum += abs(e[i][k]);
            }
            if (err_sum < min_err[1]) {
                min_err[0] = i; // p
                min_err[1] = err_sum;
                min_err[2] = (quint8) ceil(log2(log(2) * ((double) err_sum / NUM_SAMPLES_PER_FRAME))); // m
            }
        }
        Frame<T> frame;
        frame.is_constant = (min_err[0] == 0);
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
        out_c_wav.frames.push_back(frame);
    }
}

// Function assumes n >= 8.
quint8 next_byte(int& n, quint64 bits) {
    quint64 mask = 0xFFu << (n - 8);
    quint8 byte = (bits & mask) >> (n - 8);
    n -= 8;
    return byte;
}

void save_pending_bytes(vector<quint8>& bytes, quint64& b, int& n, quint64 bits) {
    while (n >= 8) bytes[b++] = next_byte(n, bits);
}

void add_bits(int& n, quint64& bits, quint64 num_new_bits, quint64 x) {
    bits <<= num_new_bits;
    bits += x;
    n += num_new_bits;
}

template<class T>
void encode(CompressedWAV<T>& out_c_wav) {
    const int NUM_M_BITS = 6;
    vector<quint8> bytes;
    // TODO: Write the file header to bytes.

    // Encode each frame.
    // Num bytes saved to vector.
    quint64 b = 0; // TODO: won't be zero after writing to file header.
    for (auto frame : out_c_wav.frames) {
        auto p = frame.p;
        auto m = frame.m;
        int n = 2; // Num bits.
        quint64 bits = p;

        add_bits(n, bits, NUM_M_BITS, m);
        save_pending_bytes(bytes, b, n, bits);

        // TODO: Do something different if frame.is_constant.

        // Store the first p samples into bytes.
        for (int i = 0; i < p; i++) {
            add_bits(n, bits, out_c_wav.bit_depth, frame.first_p_samples[i]);
            save_pending_bytes(bytes, b, n, bits);
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
            quint64 num_zeros = abs((res & ~m_mask) >> m);

            add_bits(n, bits, 1, sign);
            add_bits(n, bits, m, m_bits);
            add_bits(n, bits, num_zeros + 1, 1);

            save_pending_bytes(bytes, b, n, bits);
        }
    }
}

// Kinda weird to do this since we have T already.
template<class T>
void compress(const WAV& wav, CompressedWAV<T>& out_c_wav) {
    switch (wav.bits_per_sample) {
        case 8:
            compress((quint8*) wav.bytes.get(), wav.data_size, out_c_wav);
            break;
        case 16:
            compress((qint16*) wav.bytes.get(), wav.data_size/2, out_c_wav);
            break;
        default:
            // Do nothing.
            break;
    }
}

template<class T>
void decompress(const CompressedWAV<T>& c_wav, WAV& out_wav) {

}
