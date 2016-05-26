#pragma once

#include <mapbox/geojsonvt/simplify.hpp>
#include <mapbox/geojsonvt/types.hpp>
#include <mapbox/geometry.hpp>

#include <cmath>

namespace mapbox {
namespace geojsonvt {
namespace detail {

inline vt_point project(const geometry::point<double>& p) {
    const double sine = std::sin(p.y * M_PI / 180);
    const double x = p.x / 360 + 0.5;
    const double y =
        std::max(std::min(0.5 - 0.25 * std::log((1 + sine) / (1 - sine)) / M_PI, 1.0), 0.0);
    return { x, y, 0.0 };
}

inline vt_features convert(const geometry::feature_collection<double>& features,
                           const double tolerance) {
    vt_features projected;
    projected.reserve(features.size());

    for (const auto& f : features) {
        vt_feature feature {
            transform<vt_geometry>(f.geometry, project),
            f.properties;
        };

        for_each_line_string(feature.geometry, [&] (vt_line_string& line) {
            line.calculate_dist();
            simplify(line, tolerance);
        });

        for_each_linear_ring(feature.geometry, [&] (vt_linear_ring& ring) {
            ring.calculate_area();
            simplify(ring, tolerance);
        }

        projected.push_back(std::move(feature));
    }

    return projected;
}

} // namespace detail
} // namespace geojsonvt
} // namespace mapbox
