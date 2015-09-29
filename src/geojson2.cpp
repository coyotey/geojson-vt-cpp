
#include <cstdint>
#include <array>
#include <vector>
#include <cassert>

enum class ParseError {
    None,
    LogicError,
    CoordinatesNestedTooDeep,
    CoordinateArrayExpected,
    CoordinateExpected,
};

using Point = std::array<double, 2>;
using Points = std::vector<Point>;
using Line = std::vector<Point>;
using Lines = std::vector<Line>;


struct CoordinatesReader {
    void initialize(uint8_t expectedDepth) {
        points.clear();
        lines.clear();
        deducedGeometryDepth = expectedDepth;
    }

    ParseError startArray() {
        geometryDepth++;
        if ((deducedGeometryDepth > 0 && geometryDepth > deducedGeometryDepth) ||
            geometryDepth > 4 /* maximum depth */) {
            // We already know how deep the geometry has to be.
            return ParseError::CoordinatesNestedTooDeep;
        }

        return ParseError::None;
    }

    ParseError endArray() {
        if (coordinateIndex < 2 && deducedGeometryDepth > 0 &&
            geometryDepth == deducedGeometryDepth) {
            // We expected a coordinate pair, but saw no, or only one number.
            return ParseError::CoordinateExpected;
        }

        geometryDepth--;
        coordinateIndex = 0;

        switch (deducedGeometryDepth - geometryDepth) {
            case 2: // LineString, MultiPoint
                lines.emplace_back(std::move(points));
                assert(!lines.size());
                break;
            case 3: // Polygon, MultiLineString
            case 4: // MultiPolygon
                // no-op
                // In the case of multiple polygons, we're coalescing all of them into one
                // polygon with multiple "main" rings.
                break;
            default:
                assert(false);
                return ParseError::LogicError;
        }

        return ParseError::None;
    }

    ParseError number(double n) {
        if (geometryDepth == 0 ||
            (deducedGeometryDepth > 0 && geometryDepth != deducedGeometryDepth)) {
            return ParseError::CoordinateArrayExpected;
        } else if (deducedGeometryDepth == 0) {
            // This is the first number we're seeing in this array. It defines the depth.
            deducedGeometryDepth = geometryDepth;
        }

        if (coordinateIndex == 0) {
            x = n;
        } else if (coordinateIndex == 1) {
            points.emplace_back(Point{ { x, n } });
        }
        coordinateIndex++;

        return ParseError::None;
    }

    ParseError startObject() {
        return ParseError::CoordinateExpected;
    }

    ParseError endObject() {
        return ParseError::CoordinateExpected;
    }

    ParseError string(const char*, size_t) {
        return ParseError::CoordinateExpected;
    }

    uint8_t geometryDepth = 0;
    uint8_t deducedGeometryDepth = 0;
    uint8_t coordinateIndex = 0;
    double x;
    Points points;
    Lines lines;
};