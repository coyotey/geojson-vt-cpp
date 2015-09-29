#ifndef MAPBOX_UTIL_GEOJSON
#define MAPBOX_UTIL_GEOJSON

#include <string>
#include <array>
#include <vector>
#include <stdexcept>
#include <cassert>

namespace mapbox {
namespace util {
namespace geojson {

struct Exception : std::runtime_error {
    inline Exception(const std::string& msg, size_t r, size_t c)
        : std::runtime_error(msg), row(r), column(c) {
    }
    const size_t row;
    const size_t column;
};

using Point = std::array<double, 2>;
using Line = std::vector<Point>;
using Lines = std::vector<Line>;
using Points = std::vector<Point>;
using Polygon = std::vector<Points>;

class Geometry {
public:
    enum class Type : uint8_t { None, Point, Line, Polygon };

    Geometry(const Geometry&) = delete;
    Geometry& operator=(const Geometry&) = delete;
    Geometry& operator=(Geometry&& f) = delete;

    inline Geometry(Type t, Points&& p) : type(t), points(p) {
        assert(type == Type::Point);
    }
    inline Geometry(Type t, Lines&& l) : type(t), lines(l) {
        assert(type == Type::Line || type == Type::Polygon);
    }
    inline Geometry(Type t) : type(t) {
        assert(type == Type::None);
    }
    inline Geometry(Geometry&& f) : type(f.type) {
        if (type == Type::Point) {
            new (&points) Points(std::move(f.points));
        } else if (type == Type::Line || type == Type::Polygon) {
            new (&lines) Lines(std::move(f.lines));
        }
    }

    inline ~Geometry() {
        if (type == Type::Point) {
            points.~Points();
        } else if (type == Type::Line || type == Type::Polygon) {
            lines.~Lines();
        }
    }

    inline Type getType() const {
        return type;
    }

    inline const Points& getPoints() const {
        assert(type == Type::Point);
        return points;
    }

    inline const Lines& getLines() const {
        assert(type == Type::Line);
        return lines;
    }

    inline const Polygon& getPolygon() const {
        assert(type == Type::Polygon);
        return polygon;
    }

private:
    const Type type;

    union {
        Points points;
        Lines lines;
        Polygon polygon;
    };
};

class Feature {
public:
    inline Feature(Geometry&& g) : geometry(std::move(g)) {}

    Feature(const Feature&) = delete;
    Feature& operator=(const Feature&) = delete;
    Feature(Feature&& f) = default;
    Feature& operator=(Feature&& f) = default;

    inline Geometry::Type getType() const {
        return geometry.getType();
    }

    inline const Points& getPoints() const {
        return geometry.getPoints();
    }

    inline const Lines& getLines() const {
        return geometry.getLines();
    }

    inline const Polygon& getPolygon() const {
        return geometry.getPolygon();
    }

private:
    Geometry geometry;
};

class GeoJSON {
public:
    struct Error {
        inline Error(const std::string& msg, size_t r, size_t c) : message(msg), row(r), column(c) {
        }
        const std::string message;
        const size_t row;
        const size_t column;
    };

public:
    GeoJSON(const std::string& json);

    inline size_t size() const {
        return features.size();
    }

    inline bool isValid() const {
        return !error;
    }

    inline const Error& getError() const {
        assert(error);
        return *error;
    }

    inline const Feature& operator[](size_t i) const {
        return features[i];
    }

private:
    std::unique_ptr<Error> error;
    std::vector<Feature> features;
};

} // namespace geojson
} // namespace util
} // namespace mapbox

#endif // MAPBOX_UTIL_GEOJSON
