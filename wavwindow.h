#ifndef WAVWINDOW_H
#define WAVWINDOW_H

#include <QMainWindow>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChartView>
#include "wav.h"
#include <memory>

namespace Ui {
class WavWindow;
}

class WavWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit WavWindow(QWidget *parent = nullptr);
    ~WavWindow();

private:
    Ui::WavWindow *ui;
    std::unique_ptr<QMainWindow> chan1_window;
    std::unique_ptr<QMainWindow> chan2_window;
    std::unique_ptr<QChartView> chan1_chartView, chan2_chartView;
    std::unique_ptr<QChart> chart1, chart2;
    std::unique_ptr<QLineSeries> chan1, chan2;

private slots:
    void on_selectFileButton_clicked();

private:
    template<class T>
    void plot_waveform(const T* samples, quint32 len, const WAV& wav);
};

#endif // WAVWINDOW_H
