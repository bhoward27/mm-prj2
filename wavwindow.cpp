#include "wavwindow.h"
#include "ui_wavwindow.h"
#include "utils.h"
#include "wav_compression.h"
#include "lzw.h"
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QDataStream>
#include <QtCharts/QChartView>
#include <QPoint>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::unique_ptr;

WavWindow::WavWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WavWindow)
{
    ui->setupUi(this);
}

WavWindow::~WavWindow()
{
    delete ui;
}

void WavWindow::on_selectFileButton_clicked() {
    // Get the file name.
    QString file_name = QFileDialog::getOpenFileName(this, "Open the file");
    QFile file(file_name);

    // Open the .wav file.
    // TODO: Only allow the user to select .wav files.
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Warning", "Could not open file: " + file.errorString());
    }

    WAV wav = WAV();
    wav.file_name = file_name;
    WAVReadResult res = wav.read(file);
    file.close();
    switch (res) {
        case WAVReadResult::ok:
            // Do nothing.
            break;
        case WAVReadResult::not_riff:
            // TODO: Display error messagebox.
            cout << "Error: not_riff" << endl;
            break;
        case WAVReadResult::not_wav:
            cout << "Error: not_wav" << endl;
            break;
        case WAVReadResult::not_fmt:
            cout << "Error: not_fmt" << endl;
            break;
        case WAVReadResult::not_lpcm_or_lossless:
            cout << "Error: not_lpcm" << endl;
            break;
        case WAVReadResult::not_mono_or_stereo:
            cout << "Error: not_mono_or_stereo" << endl;
            break;
        case WAVReadResult::not_data:
            cout << "Error: not_data" << endl;
            break;

    }
    switch (wav.bits_per_sample) {
        case 8:
        {
            CompressedWAV<quint8> c_wav8 = CompressedWAV<quint8>();
            compress(wav, c_wav8);
            lzw_compress(wav.bytes, wav.data_size, c_wav8);
            plot_waveform((quint8*) wav.bytes.get(), wav.data_size, wav);
            break;
        }
        case 16:
        {
            CompressedWAV<qint16> c_wav16 = CompressedWAV<qint16>();
            compress(wav, c_wav16);
            lzw_compress(wav.bytes, wav.data_size, c_wav16);
            plot_waveform((qint16*) wav.bytes.get(), wav.data_size/2, wav);
            break;
        }
        default:
            // Throw error?
            cout << "bits per sample == " << wav.bits_per_sample << endl;
            break;
    }
}

template<class T>
void WavWindow::plot_waveform(const T* samples, quint32 len, const WAV& wav) {
    // TODO: Simply plot the samples as lines instead of doing interpolation between points.
    QString title = "";
    title += QString("TOTAL Number of Samples = ") + QString::number(len);
    title += QString(" - Sampling Rate = ") + QString::number(wav.sample_rate) + QString(" Hz");
    switch (wav.num_channels) {
        case 1:
            chan1 = unique_ptr<QLineSeries>(new QLineSeries());
            for (quint32 i = 0; i < len; i++) {
                chan1->append(i, samples[i]);
            }
            title = QString("Audio - ") + title;
            show_chart(chan1_window, chan1_chartView, chart1, chan1.get(), title, 300, 0);
            break;
        case 2:
            chan1 = unique_ptr<QLineSeries>(new QLineSeries());
            chan2 = unique_ptr<QLineSeries>(new QLineSeries());
            for (quint32 i = 0; i < len - 1; i += 2) {
                chan1->append(i/2, samples[i]);
                chan2->append(i/2, samples[i + 1]);
            }
            show_chart(chan1_window, chan1_chartView, chart1, chan1.get(), QString("Left Channel - ") + title, 0, 0);
            show_chart(chan2_window, chan2_chartView, chart2, chan2.get(), QString("Right Channel - ") + title, 600, 0);
            break;
        default:
            // Do nothing.
            break;
    }
}
