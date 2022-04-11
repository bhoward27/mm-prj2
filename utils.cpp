#include "utils.h"
#include <QtCharts/QChart>
#include <QtCharts/QChartView>

using std::unique_ptr;

void show_chart(
        unique_ptr<QMainWindow>& window,
        unique_ptr<QChartView>& chartView,
        unique_ptr<QChart>& chart,
        QLineSeries* series, QString title,
        int x, int y
) {
    chart = unique_ptr<QChart>(new QChart());
    chart->legend()->hide();
    chart->addSeries(series);
    chart->createDefaultAxes();
    chart->setTitle(title);

    chartView = unique_ptr<QChartView>(new QChartView(chart.get()));
    chartView->setRenderHint(QPainter::Antialiasing);

    window = unique_ptr<QMainWindow>(new QMainWindow);
    window->setCentralWidget(chartView.get());
    window->setGeometry(x, y, 600, 600);
    window->show();
}
