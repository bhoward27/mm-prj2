#ifndef UTILS_H
#define UTILS_H

#include <QMainWindow>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChartView>
#include <memory>
#include <vector>

void show_chart(
        std::unique_ptr<QMainWindow>& window,
        std::unique_ptr<QChartView>& chartView,
        std::unique_ptr<QChart>& chart,
        QLineSeries* series, QString title,
        int x, int y
);

// Append x to the byte buffer, bytes.
template<class T>
void add_bytes(std::vector<quint8>& bytes, T x) {
    for (unsigned long i = 0; i < sizeof(x); i++) {
        bytes.push_back(0);
    }
    std::memcpy(&bytes.data()[bytes.size() - sizeof(x)], &x, sizeof(x));
}

#endif // UTILS_H
