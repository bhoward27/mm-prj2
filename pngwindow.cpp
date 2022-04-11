#include "pngwindow.h"
#include "ui_pngwindow.h"
#include "utils.h"
#include <QFileDialog>
#include <QImage>
#include <iostream>
#include <memory>

using std::cout;
using std::endl;
using std::array;
using std::unique_ptr;

// const int N = 4;
const int N = 8;

// 4 x 4 Bayer matrix
//const int D[N][N] = {
//    {0, 8, 2, 10},
//    {12, 4, 14, 6},
//    {3, 11, 1, 9},
//    {15, 7, 13, 5}
//};

// 8 x 8 Bayer matrix
const int D[N][N] = {
    // Rows 1 - 4
    {0, 32, 8, 40, 2, 34, 10, 42},
    {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44, 4, 36, 14, 46, 6, 38},
    {60, 28, 52, 20, 62, 30, 54, 22},

    // Rows 5 - 8
    {3, 35, 11, 43, 1, 33, 9, 41},
    {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47, 7, 39, 13, 45, 5, 37},
    {63, 31, 55, 23, 61, 29, 53, 21},
};

PNGWindow::PNGWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PNGWindow)
{
    ui->setupUi(this);
}

PNGWindow::~PNGWindow()
{
    delete ui;
}

void PNGWindow::on_selectFileButton_clicked() {
    QString file_name = QFileDialog::getOpenFileName(this, "Open the file");
    unique_ptr<QImage> og_img(new QImage(file_name));
    if (og_img->isNull()) {
        // TODO: Use messagebox
        cout << "img is null." << endl;
        return;
    }
    // TODO: Check the format using img->format().

    array<quint64, FREQ_LEN> red_freqs = {0};
    array<quint64, FREQ_LEN> green_freqs = {0};
    array<quint64, FREQ_LEN> blue_freqs = {0};
    auto height = og_img->height();
    auto width = og_img->width();

    // For histograms.
    for (int y = 0; y < height; ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(og_img->scanLine(y));

        for (int x = 0; x < width; ++x) {
            QRgb &rgb = line[x];
            int red = qRed(rgb);
            int green = qGreen(rgb);
            int blue = qBlue(rgb);

            red_freqs[red]++;
            green_freqs[green]++;
            blue_freqs[blue]++;
        }
    }

    unique_ptr<QImage> dither_img;
    bool isBayer = ui->radioButtonBayer->isChecked();
    if (isBayer) { // Bayer
        dither_img = unique_ptr<QImage>(new QImage(og_img->size(), QImage::Format_RGB32));
    }
    else { // Floyd-Steinberg
        dither_img = unique_ptr<QImage>(new QImage(*og_img));
    }
    for (int y = 0; y < height; ++y) {
        QRgb* line = (isBayer) ? reinterpret_cast<QRgb*>(og_img->scanLine(y)) : reinterpret_cast<QRgb*>(dither_img->scanLine(y));
        for (int x = 0; x < width; ++x) {
            QRgb &rgb = line[x];
            int red = qRed(rgb);
            int green = qGreen(rgb);
            int blue = qBlue(rgb);

            // Bayer dither.
            // Dither for each channel.
            if (isBayer) {
                int d = D[y % N][x % N];
                auto point = QPoint(x, y);
                auto colour = QColor(dither(red, d), dither(green, d), dither(blue, d));
                // NOTE: setPixelColor is NOT an efficient function. Documentation reccommends other ways.
                dither_img->setPixelColor(point, colour);
            }
            else {
                // Floyd-Steinberg dither.
                int new_red = find_nearest_colour(red);
                int new_green = find_nearest_colour(green);
                int new_blue = find_nearest_colour(blue);

                QColor new_colour = QColor(new_red, new_green, new_blue);
                dither_img->setPixelColor(QPoint(x, y), new_colour);

                int red_quant_err = red - new_red;
                int green_quant_err = green - new_green;
                int blue_quant_err = blue - new_blue;
                array<int, 3> quant_errs = {red_quant_err, green_quant_err, blue_quant_err};
                array<int, 3> x_p_1_y_summands;
                array<int, 3> x_m_1_y_p_1_summands;
                array<int, 3> x_y_p_1_summands;
                array<int, 3> x_p_1_y_p_1_summands;
                for (int i = 0; i < 3; i++) {
                    int quant_err = quant_errs[i];
                    x_p_1_y_summands[i] = err_disperse(quant_err, 7);
                    x_m_1_y_p_1_summands[i] = err_disperse(quant_err, 3);
                    x_y_p_1_summands[i] = err_disperse(quant_err, 5);
                    x_p_1_y_p_1_summands[i] = err_disperse(quant_err, 1);
                }

                bool x_p_1_y_is_in_range = is_in_range(x + 1, width) && is_in_range(y, height);
                bool x_m_1_y_p_1_is_in_range = is_in_range(x - 1, width) && is_in_range(y + 1, height);
                bool x_y_p_1_is_in_range = is_in_range(x, width) && is_in_range(y + 1, height);
                bool x_p_1_y_p_1_is_in_range = is_in_range(x + 1, width) && is_in_range(y + 1, height);

                array<int, 3> x_p_1_y_colrs;
                array<int, 3> x_m_1_y_p_1_colrs;
                array<int, 3> x_y_p_1_colrs;
                array<int, 3> x_p_1_y_p_1_colrs;
                // NOTE: pixel() is not efficient.
                for (int i = 0; i < 3; i++) {
                    if (x_p_1_y_is_in_range)
                        x_p_1_y_colrs[i] = scale(get_colr_component(dither_img->pixel(x + 1, y), i) + x_p_1_y_summands[i]);
                    if (x_m_1_y_p_1_is_in_range)
                        x_m_1_y_p_1_colrs[i] = scale(get_colr_component(dither_img->pixel(x - 1, y + 1), i) + x_m_1_y_p_1_summands[i]);
                    if (x_y_p_1_is_in_range)
                        x_y_p_1_colrs[i] = scale(get_colr_component(dither_img->pixel(x, y + 1), i) + x_y_p_1_summands[i]);
                    if (x_p_1_y_p_1_is_in_range)
                        x_p_1_y_p_1_colrs[i] = scale(get_colr_component(dither_img->pixel(x + 1, y + 1), i) + x_p_1_y_p_1_summands[i]);
                }

                if (x_p_1_y_is_in_range)
                    dither_img->setPixelColor(QPoint(x + 1, y), get_rgb(x_p_1_y_colrs));
                if (x_m_1_y_p_1_is_in_range)
                    dither_img->setPixelColor(QPoint(x - 1, y + 1), get_rgb(x_m_1_y_p_1_colrs));
                if (x_y_p_1_is_in_range)
                    dither_img->setPixelColor(QPoint(x, y + 1), get_rgb(x_y_p_1_colrs));
                if (x_p_1_y_p_1_is_in_range)
                    dither_img->setPixelColor(QPoint(x + 1, y + 1), get_rgb(x_p_1_y_p_1_colrs));
            }
        }
    }

    // Display original image.
    *og_img = og_img->scaled(ui->labelOriginalImage->width(), ui->labelOriginalImage->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    unique_ptr<QPixmap> og_pm(new QPixmap());
    *og_pm = og_pm->fromImage(*og_img);
    ui->labelOriginalImage->setPixmap(*og_pm);

    // Display dither image.
    *dither_img = dither_img->scaled(ui->labelDitherImage->width(), ui->labelDitherImage->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    unique_ptr<QPixmap> dither_pm(new QPixmap());
    *dither_pm = dither_pm->fromImage(*dither_img);
    ui->labelDitherImage->setPixmap(*dither_pm);

    plot_freq_not_histogram(QColor("red"), red_hist_window, red_hist_chartView, red_chart, red_freq_series, red_freqs, "Red Histogram", 0, 0);
    plot_freq_not_histogram(QColor("green"), green_hist_window, green_hist_chartView, green_chart, green_freq_series, green_freqs, "Green Histogram", 600, 0);
    plot_freq_not_histogram(QColor("blue"), blue_hist_window, blue_hist_chartView, blue_chart, blue_freq_series, blue_freqs, "Blue Histogram", 300, 600);
}

// TODO: As currently implemented this is technically NOT a histogram, but merely a frequency plot.
// Therefore, you should modify this function (and then name it plot_histogram).
// Histogram is defined as follows:
// "a diagram consisting of rectangles whose area is proportional to the frequency of a variable and
// whose width is equal to the class interval."
void PNGWindow::plot_freq_not_histogram(
        QColor line_colour,
        unique_ptr<QMainWindow>& window,
        unique_ptr<QChartView>& chartView,
        unique_ptr<QChart>& chart,
        unique_ptr<QLineSeries>& freq_series,
        const array<quint64, FREQ_LEN>& colour_freqs,
        QString title, int x, int y
) {
    freq_series = unique_ptr<QLineSeries>(new QLineSeries());
    freq_series->setColor(line_colour);
    for (int i = 0; i < FREQ_LEN; i++) {
        freq_series->append(i, colour_freqs[i]);
    }
    show_chart(window, chartView, chart, freq_series.get(), title, x, y);
}

int PNGWindow::get_colr_component(QRgb rgb, int i) {
    switch(i) {
        case 0:
            return qRed(rgb);
        case 1:
            return qGreen(rgb);
        case 2:
            return qBlue(rgb);
        default:
            return 0;
    }
}

QRgb PNGWindow::get_rgb(array<int, 3> colrs) {
    return qRgb(colrs[0], colrs[1], colrs[2]);
}

// TODO: These could be all be macros.
// a: the colour component of the pixel we're at
// b: the corresponding element in the Bayer matrix
int PNGWindow::dither(int a, int b) {
    return (a > 256/(N*N) * b) ? 255 : 0;
}

int PNGWindow::scale(int colr) {
    if (colr > 255) return 255;
    if (colr < 0) return 0;
    return colr;
}

int PNGWindow::find_nearest_colour(int a) {
    return (a < 127) ? 0 : 255;
}

int PNGWindow::err_disperse(int quant_err, int filter) {
    return (quant_err * filter)/16;
}

bool PNGWindow::is_in_range(int a, int max) {
    return (a >= 0 && a < max);
}
