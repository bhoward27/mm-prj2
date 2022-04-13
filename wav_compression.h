#ifndef WAV_COMPRESSION_H
#define WAV_COMPRESSION_H

#include <QtGlobal>
#include <vector>
#include "wav.h"

using namespace std; // TODO: Bad.

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
void decompress(const CompressedWAV<T>& c_wav, WAV& out_wav) {

}

#endif // WAV_COMPRESSION_H
