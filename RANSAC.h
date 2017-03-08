#pragma once

#include <vector>

#include "RNG.h"

/*
struct Model {
typedef data_point_type point_type;
static const int n_fit;
bool fit(const std::vector<point_type> &points, const std::vector<unsigned char> &inlier_mask);
bool consensus(const point_type &point);
};
*/

template <typename Model>
inline void ransac(Model &model, const std::vector<typename Model::point_type> &points, std::vector<unsigned char>& inliers, double success_rate = 0.95, int max_iter = 20000000) {
    UniformInteger<size_t> rnd(0, points.size() - 1);

    int n_iter = 0;

    double best_inlier_rate = 0.0;
    std::vector<unsigned char> inlier_set(points.size(), 0);
    inliers.resize(points.size());

    while (n_iter < max_iter) {
        // sample
        for (int i = 0; i < Model::n_fit; ++i) {
            inlier_set[rnd.next()] = 1;
        }

        if (model.fit(points, inlier_set)) {
            size_t inlier_count = 0;
            for (size_t i = 0; i < points.size(); ++i) {
                if (model.consensus(points[i])) {
                    inlier_set[i] = 1;
                    inlier_count++;
                }
            }

            double inlier_rate = double(inlier_count) / double(points.size());

            if (inlier_rate > best_inlier_rate) {
                best_inlier_rate = inlier_rate;
                inliers.swap(inlier_set);
                int new_max_iter = (int)ceil(log(1 - success_rate) / log(1 - pow(best_inlier_rate, Model::n_fit)));
                max_iter = std::min(max_iter, new_max_iter);
            }
        }

        std::fill(inlier_set.begin(), inlier_set.end(), 0);
        n_iter++;
    }

    model.fit(points, inliers);
}

template <typename Model>
inline void ransac(Model &model, const std::vector<typename Model::point_type> &points, double success_rate = 0.95, int max_iter = 20000000) {
    std::vector<unsigned char> inliers;
    ransac(model, points, inliers, success_rate, max_iter);
}
