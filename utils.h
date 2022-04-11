#ifndef UTILS_H
#define UTILS_H

#include <QMainWindow>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChartView>
#include <memory>

void show_chart(
        std::unique_ptr<QMainWindow>& window,
        std::unique_ptr<QChartView>& chartView,
        std::unique_ptr<QChart>& chart,
        QLineSeries* series, QString title,
        int x, int y
);

#endif // UTILS_H
