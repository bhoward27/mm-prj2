#include "wav_compression.h"
#include "wav.h"
#include <array>
#include <vector>
#include <stdexcept>

using std::invalid_argument;
using std::array;
using std::vector;

struct CompressedWAV{};

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
//    quint32 i;
    // TODO: What if n is not divisible by num_samples_per_frame? Then this access may go out of bounds?
    for (quint32 j = 0; j < n; j += NUM_SAMPLES_PER_FRAME) {
        for (quint32 i = 0; i < p; i++) {
            x_hat[j + i] = samples[j + i];
        }
    }
//    for (; i < n; i++) {
//        x_hat.push_back(0);
//    }
}

template<class T>
void compress(const T* samples, quint32 n, CompressedWAV& out_c_wav) {
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


}

void compress(const WAV& wav, CompressedWAV& out_c_wav) {
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

void decompress(const CompressedWAV& c_wav, WAV& out_wav) {

}
