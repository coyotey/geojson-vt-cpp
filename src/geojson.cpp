#include <mapbox/geojsonvt/geojson.hpp>


#include <sstream>
#include <iostream>
#include <vector>
#include <stack>
#include <array>


#include <rapidjson/reader.h>
#include <rapidjson/error/en.h>

namespace mapbox {
namespace util {
namespace geojson {

namespace {

enum class Expected {
    None,

    Root,
    RootTypeKey,
    Type,
    Geometry,
    GeometryTypeKey,
    GeometryType,
    GeometryKey,
    GeometryCoordinates,
    FirstGeometryCoordinate,
    SecondGeometryCoordinate,
    ExcessGeometryCoordinates,
    Ignored,
    CoordinatesKey,
    GeometryDepth1,
    GeometryDepth2,
    GeometryDepth3,
    GeometryDepth4,
};

::std::ostream& operator<<(::std::ostream& os, Expected e) {
    switch (e) {
    case Expected::None:
        return os << "nothing";
    case Expected::Root:
        return os << "a root object";
    case Expected::Type:
        return os << "one of 'Point', 'MultiPoint', 'LineString', 'MultiLineString', "
                     "'Polygon', 'MultiPolygon', 'GeometryCollection', 'Feature', or "
                     "'FeatureCollection'";
    case Expected::RootTypeKey:
        return os << "a key 'type' in the root object";
    case Expected::Geometry:
        return os << "a geometry object";
    case Expected::GeometryTypeKey:
        return os << "a key 'type' in the geometry object";
    case Expected::GeometryType:
        return os << "one of 'Point', 'MultiPoint', 'LineString', 'MultiLineString', 'Polygon', "
                     "'MultiPolygon', or 'GeometryCollection'";
    case Expected::GeometryKey:
        return os << "a 'geometry' key value pair";
    case Expected::GeometryCoordinates:
        return os << "a coordinate array";
    case Expected::FirstGeometryCoordinate:
        return os << "a coordinate";
    case Expected::SecondGeometryCoordinate:
        return os << "a second coordinate";
    case Expected::ExcessGeometryCoordinates:
        return os << "a closing bracket, or a coordinate";
    case Expected::Ignored:
        return os << "an ignored value";
    case Expected::CoordinatesKey:
        return os << "a 'coordinates' key value pair";
    case Expected::GeometryDepth1:
        return os << "'Point'";
    case Expected::GeometryDepth2:
        return os << "'LineString', or 'MultiPoint'";
    case Expected::GeometryDepth3:
        return os << "'Polygon', or 'MultiLineString'";
    case Expected::GeometryDepth4:
        return os << "'MultiPolygon'";
    }
}

enum class Actual {
    None,

    Object,
    ObjectEnded,
    Array,
    ArrayEnded,
    Key,
    Keyword,
    String,
    Number,
    Null,
    Boolean,

    KeywordPoint,
    KeywordLineString,
    KeywordPolygon,
    KeywordMultiPoint,
    KeywordMultiLineString,
    KeywordMultiPolygon,
    KeywordGeometryCollection,
};

::std::ostream& operator<<(::std::ostream& os, Actual a) {
    switch (a) {
    case Actual::None:
        return os << "nothing";
    case Actual::Object:
        return os << "an object";
    case Actual::ObjectEnded:
        return os << "a prematurely closed object";
    case Actual::Array:
        return os << "an array";
    case Actual::ArrayEnded:
        return os << "a prematurely closed array";
    case Actual::Key:
        return os << "an object key";
    case Actual::Keyword:
        return os << "an invalid keyword";
    case Actual::String:
        return os << "a string";
    case Actual::Number:
        return os << "a number";
    case Actual::Null:
        return os << "a null object";
    case Actual::Boolean:
        return os << "a boolean";

    case Actual::KeywordPoint:
        return os << "'Point'";
    case Actual::KeywordLineString:
        return os << "'LineString'";
    case Actual::KeywordPolygon:
        return os << "'Polygon'";
    case Actual::KeywordMultiPoint:
        return os << "'MultiPoint'";
    case Actual::KeywordMultiLineString:
        return os << "'MultiLineString'";
    case Actual::KeywordMultiPolygon:
        return os << "'MultiPolygon'";
    case Actual::KeywordGeometryCollection:
        return os << "'GeometryCollection'";
    }
}

enum class GeometryType {
    None,
    Point,
    LineString,
    Polygon,
    MultiPoint,
    MultiLineString,
    MultiPolygon,
    GeometryCollection,
};

// ::std::ostream& operator<<(::std::ostream& os, GeometryType t) {
//     switch (t) {
//     case GeometryType::None:
//         return os << "no geometry";
//     case GeometryType::Point:
//         return os << "a Point";
//     case GeometryType::LineString:
//         return os << "a LineString";
//     case GeometryType::Polygon:
//         return os << "a Polygon";
//     case GeometryType::MultiPoint:
//         return os << "a MultiPoint";
//     case GeometryType::MultiLineString:
//         return os << "a MultiLineString";
//     case GeometryType::MultiPolygon:
//         return os << "a MultiPolygon";
//     case GeometryType::GeometryCollection:
//         return os << "a GeometryCollection";
//     }
// }

enum class RootType {
    None,
    Point,
    LineString,
    Polygon,
    MultiPoint,
    MultiLineString,
    MultiPolygon,
    GeometryCollection,
    Feature,
    FeatureCollection,
};

// Stores the three geometry types that we can see in a root object.
struct Geometries {
    std::unique_ptr<Geometry> coordinates;
    std::unique_ptr<std::vector<Geometry>> geometry;
    std::unique_ptr<std::vector<Geometry>> geometries;
};


struct GeoJSONReader : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, GeoJSONReader> {
    GeoJSONReader(std::back_insert_iterator<std::vector<Feature>> i) : inserter(i) {}

    std::back_insert_iterator<std::vector<Feature>> inserter;



    std::stack<Expected, std::vector<Expected>> expect{ { Expected::Root } };

    GeometryType geometryType = GeometryType::None;
    RootType rootType = RootType::None;

    int ignoredDepth = 0;
    int geometryDepth = 0;
    int deducedGeometryDepth = 0;
    int pointOffset = 0;

    double x;
    Points points;
    Polygon polygon;

    std::unique_ptr<Polygon> coordinates;
    std::unique_ptr<std::vector<Geometry>> geometry;

    struct {
        Expected expected;
        Actual actual;
    } error = { Expected::None, Actual::None };

    bool StartObject() {
        switch (expect.top()) {
        case Expected::Ignored:
            ignoredDepth++;
            fprintf(stderr, "ignored: %d\n", ignoredDepth);
            return true;
        case Expected::Geometry:
            return true;
        case Expected::Root:
            return true;
        default:
            error = { expect.top(), Actual::Object };
            return false;
        }
    }

    bool endRootObject() {
        switch (rootType) {
        case RootType::None:
            error = { Expected::RootTypeKey, Actual::ObjectEnded };
            return false;

        case RootType::Point:
        case RootType::MultiPoint:
            if (!coordinates) {
                error = { Expected::CoordinatesKey, Actual::ObjectEnded };
                return false;
            } else {
                assert(coordinates->size() >= 1);
                inserter = Geometry{ Geometry::Type::Point, std::move(coordinates->front()) };
            }
            break;

        case RootType::LineString:
        case RootType::MultiLineString:
            if (!coordinates) {
                error = { Expected::CoordinatesKey, Actual::ObjectEnded };
                return false;
            } else {
                inserter = Geometry{ Geometry::Type::Line, std::move(*coordinates) };
            }
            break;

        case RootType::Polygon:
        case RootType::MultiPolygon:
            if (!coordinates) {
                error = { Expected::CoordinatesKey, Actual::ObjectEnded };
                return false;
            } else {
                inserter = Geometry{ Geometry::Type::Polygon, std::move(*coordinates) };
            }
            break;

        case RootType::GeometryCollection:
            break;

        case RootType::Feature:
            if (!geometry) {
                error = { Expected::GeometryKey, Actual::ObjectEnded };
                return false;
            } else {
                for (auto& geom : *geometry) {
                    inserter = std::move(geom);
                }
            }
            break;
        case RootType::FeatureCollection:
            break;
        }

        return true;
    }

    bool endGeometryObject() {
        expect.pop(); // Geometry

        switch (geometryType) {
        case GeometryType::None:
            error = { Expected::GeometryTypeKey, Actual::ObjectEnded };
            return false;

        case GeometryType::Point:
        case GeometryType::MultiPoint:
            if (!coordinates) {
                error = { Expected::CoordinatesKey, Actual::ObjectEnded };
                return false;
            } else {
                geometry = std::make_unique<std::vector<Geometry>>();
                geometry->emplace_back(Geometry::Type::Point, std::move(coordinates->front()));
                coordinates.reset();
            }
            break;
        case GeometryType::LineString:
        case GeometryType::MultiLineString:
            break;
        case GeometryType::Polygon:
        case GeometryType::MultiPolygon:
            if (!coordinates) {
                error = { Expected::CoordinatesKey, Actual::ObjectEnded };
                return false;
            } else {
                geometry = std::make_unique<std::vector<Geometry>>();
                geometry->emplace_back(Geometry::Type::Polygon, std::move(*coordinates));
                coordinates.reset();
            }
            break;
        case GeometryType::GeometryCollection:
            break;
        }

        return true;
    }

    bool endIgnored() {
        if (ignoredDepth > 0) {
            ignoredDepth--;
        }
        if (ignoredDepth == 0) {
            expect.pop(); // Ignored
        }
        return true;
    }

    bool EndObject(rapidjson::SizeType) {
        switch (expect.top()) {
        case Expected::Ignored:
            return endIgnored();
        case Expected::Geometry:
            return endGeometryObject();
        case Expected::Root:
            return endRootObject();
        default:
            error = { Expected::None, Actual::ObjectEnded };
            return false;
        }
    }

    bool startCoordinatesArray() {
        geometryDepth++;

        switch (geometryType) {
        case GeometryType::None:
        case GeometryType::GeometryCollection:
            break;
        case GeometryType::Point:
        case GeometryType::LineString:
        case GeometryType::Polygon:
        case GeometryType::MultiPoint:
        case GeometryType::MultiLineString:
        case GeometryType::MultiPolygon:
            if (geometryDepth > getExpectedGeometryDepth(geometryType)) {
                error = { Expected::FirstGeometryCoordinate, Actual::Array };
                return false;
            }
        }

        return true;
    }

    bool StartArray() {
        switch (expect.top()) {
        case Expected::Ignored:
            ignoredDepth++;
            return true;
        case Expected::GeometryCoordinates:
            return startCoordinatesArray();
        default:
            error = { expect.top(), Actual::Array };
            return false;
        }
    }

    bool endCoordinatesArray() {
        if (deducedGeometryDepth <= geometryDepth) {
            error = { Expected::FirstGeometryCoordinate, Actual::ArrayEnded };
            return false;
        }

        assert(geometryDepth > 0);
        geometryDepth--;

        switch (deducedGeometryDepth - geometryDepth) {
        case 2: // LineString, MultiPoint
            polygon.emplace_back(std::move(points));
            assert(!points.size());
            break;
        case 3: // Polygon, MultiLineString
        case 4: // MultiPolygon
            // no-op
            // In the case of multiple polygons, we're coalescing all of them into one
            // polygon with multiple "main" rings.
            break;
        default:
            assert(false);
            return false;
        }

        if (geometryDepth == 0) {
            expect.pop(); // GeometryCoordinates
            coordinates = std::make_unique<Polygon>(std::move(polygon));
            assert(polygon.empty());
        }

        return true;
    }

    bool EndArray(rapidjson::SizeType) {
        switch (expect.top()) {
        case Expected::Ignored:
            return endIgnored();
        case Expected::GeometryCoordinates:
            return endCoordinatesArray();
        case Expected::ExcessGeometryCoordinates:
            expect.pop(); // ExcessGeometryCoordinates
            geometryDepth--;
            if (geometryDepth == 0) {
                expect.pop(); // GeometryCoordinates
                polygon.emplace_back(std::move(points));
                coordinates = std::make_unique<Polygon>(std::move(polygon));
                assert(polygon.empty());
            }
            return true;

        default:
            error = { expect.top(), Actual::ArrayEnded };
            return false;
        }
    }

    GeometryType geometryTypeFromRootType(RootType type) {
        switch (type) {
        case RootType::Feature:
        case RootType::FeatureCollection:
        case RootType::None:
            return GeometryType::None;
        case RootType::Point:
            return GeometryType::Point;
        case RootType::LineString:
            return GeometryType::LineString;
        case RootType::Polygon:
            return GeometryType::Polygon;
        case RootType::MultiPoint:
            return GeometryType::MultiPoint;
        case RootType::MultiLineString:
            return GeometryType::MultiLineString;
        case RootType::MultiPolygon:
            return GeometryType::MultiPolygon;
        case RootType::GeometryCollection:
            return GeometryType::GeometryCollection;
        }
    }

    bool parsedCoordinates = false;

    bool speculative = false;

    void parseRootCoordinates() {
        switch (rootType) {
        case RootType::None:
            // We haven't seen a type yet. We are going to speculatively parse this coordinates
            // array. When we encounter a parse error, we are going to ignore the remainder of
            // this array.
            speculative = true;
            points.clear();
            polygon.clear();
            expect.push(Expected::GeometryCoordinates);
            break;

        case RootType::GeometryCollection:
        case RootType::Feature:
        case RootType::FeatureCollection:
            //  "coordinates" is not a meaningful key for this root type. We're ignoring it.
            expect.push(Expected::Ignored);
            break;

        case RootType::Point:
        case RootType::LineString:
        case RootType::Polygon:
        case RootType::MultiPoint:
        case RootType::MultiLineString:
        case RootType::MultiPolygon:
            geometryType = geometryTypeFromRootType(rootType);
            points.clear();
            polygon.clear();
            expect.push(Expected::GeometryCoordinates);
            break;
        }
    }

    void parseGeometryCoordinates() {
        switch (geometryType) {
        case GeometryType::None:
            speculative = true;
            points.clear();
            polygon.clear();
            expect.push(Expected::GeometryCoordinates);
            break;

        case GeometryType::GeometryCollection:
            // "coordinates" is not a meaningful key for this geometry type. We're ignoring it.
            expect.push(Expected::Ignored);
            break;

        case GeometryType::Point:
        case GeometryType::LineString:
        case GeometryType::Polygon:
        case GeometryType::MultiPoint:
        case GeometryType::MultiLineString:
        case GeometryType::MultiPolygon:
            points.clear();
            polygon.clear();
            expect.push(Expected::GeometryCoordinates);
            break;
        }
    }

    void parseGeometry() {
        switch (rootType) {
        case RootType::None:
            // We haven't seen a type yet. We are going to speculatively parse this geometry
            // object. When we encounter a parse error, we are going to ignore the remainder of
            // this object.
            speculative = true;
            expect.push(Expected::Geometry);
            break;

        case RootType::Point:
        case RootType::LineString:
        case RootType::Polygon:
        case RootType::MultiPoint:
        case RootType::MultiLineString:
        case RootType::MultiPolygon:
        case RootType::GeometryCollection:
        case RootType::FeatureCollection:
            // "geometry" is not a meaningful key for this root type. We're ignoring it.
            expect.push(Expected::Ignored);
            break;

        case RootType::Feature:
            expect.push(Expected::Geometry);
            break;

        }
    }

    bool ignored() {
        if (ignoredDepth == 0) {
            expect.pop(); // Ignored
        }
        return true;
    }

    bool Key(const char* str, rapidjson::SizeType length, bool) {
        switch (expect.top()) {
        case Expected::Root: {
            const std::string key{ str, length };
            if (key == "type") {
                expect.push(Expected::Type);
            } else if (key == "geometry") {
                parseGeometry();
            } else if (key == "coordinates") {
                parseRootCoordinates();
            } else {
                fprintf(stderr, "ignoring key %s\n", key.c_str());
                expect.push(Expected::Ignored);
            }
            return true;
        } break;
        case Expected::Geometry: {
            const std::string key{ str, length };
            if (key == "type") {
                expect.push(Expected::GeometryType);
            } else if (key == "coordinates") {
                parseGeometryCoordinates();
            } else {
                fprintf(stderr, "ignoring key %s\n", key.c_str());
                expect.push(Expected::Ignored);
            }
            return true;
        } break;
        case Expected::Ignored:
            return true;
        default:
            error = { expect.top(), Actual::Key };
            return false;
        }
    }

    inline Expected getExpectedGeometryDepth(int depth) {
        switch (depth) {
        case 1:
            return Expected::GeometryDepth1;
        case 2:
            return Expected::GeometryDepth2;
        case 3:
            return Expected::GeometryDepth3;
        case 4:
            return Expected::GeometryDepth4;
        default:
            assert(false);
        }
    }

    inline Actual getActualGeometryType(GeometryType type) {
        switch (type) {
        case GeometryType::Point:
            return Actual::KeywordPoint;
        case GeometryType::LineString:
            return Actual::KeywordLineString;
        case GeometryType::Polygon:
            return Actual::KeywordPolygon;
        case GeometryType::MultiPoint:
            return Actual::KeywordMultiPoint;
        case GeometryType::MultiLineString:
            return Actual::KeywordMultiLineString;
        case GeometryType::MultiPolygon:
            return Actual::KeywordMultiPolygon;
        case GeometryType::GeometryCollection:
            return Actual::KeywordGeometryCollection;
        default:
            assert(false);
        }
    }

    inline int getExpectedGeometryDepth(GeometryType type) {
        switch (type) {
        case GeometryType::Point: return 1;
        case GeometryType::LineString: return 2;
        case GeometryType::Polygon: return 3;
        case GeometryType::MultiPoint: return 2;
        case GeometryType::MultiLineString: return 3;
        case GeometryType::MultiPolygon: return 4;
        default:
            assert(false);
        }
    }

    inline bool setGeometryType(GeometryType type) {
        int expectedGeometryDepth = getExpectedGeometryDepth(type);
        if (deducedGeometryDepth > 0 && deducedGeometryDepth != expectedGeometryDepth) {
            error = { getExpectedGeometryDepth(deducedGeometryDepth), getActualGeometryType(type) };
            return false;
        }
        geometryType = type;
        deducedGeometryDepth = expectedGeometryDepth;
        return true;
    }

    inline bool setRootType(RootType type) {
        rootType = type;
        return true;
    }

    bool rootTypeString(const std::string& value) {
        bool success = false;
        if (value == "Feature") {
            success = setRootType(RootType::Feature);
        } else if (value == "Point") {
            success = setRootType(RootType::Point) && setGeometryType(GeometryType::Point);
        } else if (value == "LineString") {
            success =
                setRootType(RootType::LineString) && setGeometryType(GeometryType::LineString);
        } else if (value == "Polygon") {
            success = setRootType(RootType::Polygon) && setGeometryType(GeometryType::Polygon);
        } else if (value == "MultiPoint") {
            success =
                setRootType(RootType::MultiPoint) && setGeometryType(GeometryType::MultiPoint);
        } else if (value == "MultiLineString") {
            success = setRootType(RootType::MultiLineString) &&
                      setGeometryType(GeometryType::MultiLineString);
        } else if (value == "MultiPolygon") {
            success =
                setRootType(RootType::MultiPolygon) && setGeometryType(GeometryType::MultiPolygon);
        } else if (value == "FeatureCollection") {
            success = setRootType(RootType::FeatureCollection);
            error = { expect.top(), Actual::Keyword };
        } else {
            error = { expect.top(), Actual::Keyword };
        }
        expect.pop(); // Type
        return success;
    }

    bool geometryTypeString(const std::string& value) {
        bool success = false;
        if (value == "Point") {
            success = setGeometryType(GeometryType::Point);
        } else if (value == "LineString") {
            success = setGeometryType(GeometryType::LineString);
        } else if (value == "Polygon") {
            success = setGeometryType(GeometryType::Polygon);
        } else if (value == "MultiPoint") {
            success = setGeometryType(GeometryType::MultiPoint);
        } else if (value == "MultiLineString") {
            success = setGeometryType(GeometryType::MultiLineString);
        } else if (value == "MultiPolygon") {
            success = setGeometryType(GeometryType::MultiPolygon);
        } else if (value == "GeometryCollection") {
            // TODO: handle GeometryCollection
            geometryType = GeometryType::GeometryCollection;
            error = { expect.top(), Actual::Keyword };
        } else {
            error = { expect.top(), Actual::Keyword };
        }
        expect.pop(); // GeometryType
        return success;
    }

    bool String(const char* str, rapidjson::SizeType length, bool) {
        switch (expect.top()) {
        case Expected::Ignored:
            return ignored();

        case Expected::Type:
            return rootTypeString({ str, length });

        case Expected::GeometryType:
            return geometryTypeString({ str, length });

        case Expected::GeometryCoordinates: {
            // This coordinate array is invalid
            expect.top() = Expected::Ignored;
            ignoredDepth = geometryDepth;
            geometryDepth = 0;
            return true;
        } break;
        default:
            break;
        }

        error = { expect.top(), Actual::String };
        return false;
    }

    bool handleNumber(double number) {
        switch (expect.top()) {
        case Expected::Ignored:
            if (ignoredDepth == 0) {
                expect.pop(); // Ignored
            }
            return true;
        case Expected::GeometryCoordinates:
            if (deducedGeometryDepth == 0) {
                deducedGeometryDepth = geometryDepth;
            }
            if (geometryDepth < deducedGeometryDepth) {
                error = { expect.top(), Actual::Number };
                return false;
            }
            x = number;
            expect.push(Expected::SecondGeometryCoordinate);
            return true;
        case Expected::FirstGeometryCoordinate:
            x = number;
            expect.top() = Expected::SecondGeometryCoordinate;
            return true;
        case Expected::SecondGeometryCoordinate:
            points.emplace_back(Point{ { x, number } });
            expect.top() = Expected::ExcessGeometryCoordinates;
            return true;
        case Expected::ExcessGeometryCoordinates:
            return true;
        default:
            error = { expect.top(), Actual::Number };
            return false;
        }
    }

    bool Int(int i) {
        return handleNumber(i);
    }
    bool Uint(unsigned u) {
        return handleNumber(u);
    }
    bool Int64(int64_t i) {
        return handleNumber(i);
    }
    bool Uint64(uint64_t u) {
        return handleNumber(u);
    }
    bool Double(double d) {
        return handleNumber(d);
    }

    bool Null() {
        switch (expect.top()) {
        case Expected::Ignored:
            if (ignoredDepth == 0) {
                expect.pop(); // Ignored
            }
            return true;
        default:
            error = { expect.top(), Actual::Null };
            return false;
        }
    }

    bool Bool(bool) {
        switch (expect.top()) {
        case Expected::Ignored:
            if (ignoredDepth == 0) {
                expect.pop(); // Ignored
            }
            return true;
        default:
            error = { expect.top(), Actual::Boolean };
            return false;
        }
    }
};

std::pair<size_t, size_t> offsetToRowAndCol(const std::string& json, size_t offset) {
    size_t line = 0;
    size_t column = 0;
    for (auto end = json.begin() + offset, it = json.begin(); it != end; it++) {
        *it == '\n' ? line++, column = 0 : column++;
    }
    return { line, column };
}

} // namespace (anonymous)

GeoJSON::GeoJSON(const std::string& json) {
    std::vector<Feature> f;
    GeoJSONReader handler(std::back_inserter(f));
    rapidjson::Reader reader;
    rapidjson::StringStream ss(json.c_str());
    if (!reader.Parse(ss, handler)) {
        size_t row, column;
        std::tie(row, column) = offsetToRowAndCol(json, reader.GetErrorOffset());

        if (handler.error.expected == Expected::None) {
            // There is a JSON error
            const auto e = reader.GetParseErrorCode();
            error = std::make_unique<GeoJSON::Error>(rapidjson::GetParseError_En(e), row, column);
        } else {
            // This is a GeoJSON format error
            std::stringstream text;
            text << "Expected " << handler.error.expected << ", but got " << handler.error.actual
                 << " instead.";
            error = std::make_unique<GeoJSON::Error>(text.str(), row, column);
        }
    }

    if (!error) {
        features = std::move(f);
    }
}

} // namespace geojson
} // namespace util
} // namespace mapbox
