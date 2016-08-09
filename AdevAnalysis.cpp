#include <cstdio>
#include <array>
#include <vector>
#include <map>
#include <ppl.h>

#include "RANSAC.h"
#include "gnuplot.h"

std::vector<double> read_vector(const std::string &filepath) {
    std::vector<double> result;
    FILE *file = fopen(filepath.c_str(), "r");
    if (!file) return result;
    double v;
    while (!feof(file) && fscanf(file, "%Lf", &v) > 0) {
        result.push_back(v);
    }
    fclose(file);
    return result;
}

std::pair<double, double> adev(const std::vector<double> &data, size_t n) {
    size_t M = data.size() / n;
    double ws = 0.0, dev = 0.0, nws, d;
    for (size_t i = 0; i < n; ++i) {
        ws += data[i];
    }
    for (size_t j = 1; j < M; ++j) {
        nws = 0.0;
        for (size_t i = 0; i < n; ++i) {
            nws += data[j*n + i];
        }
        d = (nws - ws) / n;
        ws = nws;
        dev += d*d;
    }
    dev /= 2 * (M - 1);
    dev = sqrt(dev);
    return std::make_pair(dev, dev/sqrt(M+1));
}

struct AdevNoiseModel {
    typedef std::array<double, 3> point_type;
    static const int n_fit = 1;
    bool fit(const std::vector<point_type> &points, const std::vector<unsigned char> &inlier_mask) {
        double new_noise = 0.0;
        size_t count = 0;
        for (size_t i = 0; i < inlier_mask.size(); ++i) {
            if (inlier_mask[i]) {
                double b = log10(points[i][1]) + log10(points[i][0]) * 0.5;
                new_noise += b;
                count++;
            }
        }
        m_noise = new_noise / count;
        return true;
    }

    bool consensus(const point_type &point) {
        return abs(pow(10.0, m_noise - log10(point[0])*0.5) - point[1]) <= m_err;
    }

    double m_err;
    double m_noise;
};

struct AdevRandomWalkModel {
    typedef std::array<double, 3> point_type;
    static const int n_fit = 1;
    bool fit(const std::vector<point_type> &points, const std::vector<unsigned char> &inlier_mask) {
        double new_noise = 0.0;
        size_t count = 0;
        for (size_t i = 0; i < inlier_mask.size(); ++i) {
            if (inlier_mask[i]) {
                double b = log10(points[i][1]) - log10(points[i][0]) * 0.5;
                new_noise += b;
                count++;
            }
        }
        m_noise = new_noise / count;
        return true;
    }

    bool consensus(const point_type &point) {
        return abs(pow(10.0, m_noise + log10(point[0])*0.5) - point[1]) <= m_err;
    }

    double m_err;
    double m_noise;
};

int main(int argc, char* argv[]) {
    using namespace std;

    vector<double> data = read_vector(argv[1]);
    double freq = atof(argv[2]);

    printf("# Points = %zd\n", data.size());
    printf("Freq = %.7e\n", freq);


    std::map<size_t, std::pair<double, double>> adev_data;
    for (size_t N = data.size() / 2; N >= 1; N = (size_t)(N / 1.1)) {
        adev_data[N] = make_pair(0.0, 0.0);
    }

    printf("Calculating");
    concurrency::parallel_for_each(adev_data.begin(), adev_data.end(), [&](pair<const size_t, std::pair<double, double>> &a) {
        a.second = adev(data, a.first);
        printf(".");
    });
    printf("\n");

    std::vector<std::array<double, 3>> adev_data_vector;
    for (auto &p : adev_data) {
        adev_data_vector.push_back({ p.first / freq, p.second.first, p.second.second });
    }

    AdevNoiseModel model;
    AdevRandomWalkModel rwmodel;
    rwmodel.m_err = model.m_err = adev_data_vector[0][2];
    ransac(model, adev_data_vector);
    ransac(rwmodel, adev_data_vector);
    double crw = pow(10, model.m_noise);
    double cws = pow(10, rwmodel.m_noise);
    printf("Continuous White Noise Density: %.7e\n", crw);
    printf("  Discrete White Noise Density: %.7e\n", crw*sqrt(freq));
    printf("Continuous Random Walk Density: %.7e\n", cws*sqrt(3));
    printf("  Discrete Random Walk Density: %.7e\n", cws*sqrt(3) / sqrt(freq));

    gnuplot plot;
    plot.open();

    plot.command("$DATA << EOD");
    for (auto p : adev_data_vector) {
        plot.command(to_string(p[0]) + " " + to_string(p[1]) + " " + to_string(p[2]));
    }
    plot.command("EOD");

    plot.command("set terminal windows enhanced");
    plot.command("set title 'Noise Analysis'");
    plot.command("set ylabel 'ADEV'");
    plot.command("set xlabel '¦Ó'");
    plot.command("set logscale xy");
    plot.command("f(x) = a/sqrt(x)+b*sqrt(x)");
    //plot.command("fit f(x) $DATA using 1:2 via a,b");
    plot.command("plot $DATA using 1:2:3 with errorbars title 'ADEV'");
    //plot.command("replot a / sqrt(x) title 'WN Fit'");
    //plot.command("replot b*sqrt(x) title 'RW Fit'");
    plot.command("replot " + to_string(crw) + " / sqrt(x) title 'White Noise'");
    plot.command("replot " + to_string(cws) + "*sqrt(x) title 'Random Walk'");
    plot.command("pause mouse");
    plot.command("exit");
    plot.close();

    return 0;
}