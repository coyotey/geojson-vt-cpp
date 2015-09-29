#include "util.hpp"
#include <gtest/gtest.h>

#include <mapbox/geojsonvt/geojson.hpp>

namespace mapbox {
namespace util {
namespace geojson {

::std::ostream& operator<<(::std::ostream& os, const GeoJSON::Error& e) {
    return os << "Error '" << e.message << "' at line " << e.row << ", column " << e.column;
}

} // namespace geojson
} // namespace util
} // namespace mapbox

using namespace mapbox::util::geojson;

TEST(GeoJSON, WrongRootType) {
    GeoJSON geojson(R"GEOJSON({ "type": "" })GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ("Expected one of 'Point', 'MultiPoint', 'LineString', 'MultiLineString', 'Polygon', "
              "'MultiPolygon', 'GeometryCollection', 'Feature', or 'FeatureCollection', but got an "
              "invalid keyword instead.",
              geojson.getError().message);
    EXPECT_EQ(0u, geojson.getError().row);
    EXPECT_EQ(12u, geojson.getError().column);
}

TEST(GeoJSON, Point) {
    GeoJSON geojson(R"GEOJSON({
  "type": "Point",
  "coordinates": [ 1, 2 ]
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Point, feature.getType());
    auto& points = feature.getPoints();
    ASSERT_EQ(1u, points.size()); // points
    EXPECT_EQ(Point({ { 1, 2 } }), points[0]);
}

TEST(GeoJSON, CoordinatesWithoutType) {
    GeoJSON geojson(R"GEOJSON({
  "coordinates": [ 1, 2 ]
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ(
        "Expected a key 'type' in the root object, but got a prematurely closed object instead.",
        geojson.getError().message);
    EXPECT_EQ(2u, geojson.getError().row);
    EXPECT_EQ(1u, geojson.getError().column);
}

TEST(GeoJSON, PointDuplicateMember) {
    GeoJSON geojson(R"GEOJSON({
  "type": "Point",
  "coordinates": [ 1, 2 ],
  "coordinates": [ 2, 3 ]
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Point, feature.getType());
    auto& points = feature.getPoints();
    ASSERT_EQ(1u, points.size()); // points
    EXPECT_EQ(Point({ { 2, 3 } }), points[0]);
}

TEST(GeoJSON, PointMissingFirstCoordinate) {
    GeoJSON geojson(R"GEOJSON({ "type": "Point", "coordinates": [] })GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ("Expected a coordinate, but got a prematurely closed array instead.",
              geojson.getError().message);
    EXPECT_EQ(0u, geojson.getError().row);
    EXPECT_EQ(36u, geojson.getError().column);
}

TEST(GeoJSON, PointMissingFirstCoordinateAndTypeAfter) {
    GeoJSON geojson(R"GEOJSON({ "coordinates": [], "type": "Point" })GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ("Expected a coordinate, but got a prematurely closed array instead.",
              geojson.getError().message);
    EXPECT_EQ(0u, geojson.getError().row);
    EXPECT_EQ(19u, geojson.getError().column);
}

TEST(GeoJSON, PointMissingSecondCoordinate) {
    GeoJSON geojson(R"GEOJSON({ "type": "Point", "coordinates": [ 1 ] })GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ("Expected a second coordinate, but got a prematurely closed array instead.",
              geojson.getError().message);
    EXPECT_EQ(0u, geojson.getError().row);
    EXPECT_EQ(39u, geojson.getError().column);
}

TEST(GeoJSON, MultiPoint) {
    GeoJSON geojson(R"GEOJSON({
  "type": "MultiPoint",
  "coordinates": [[1, 2], [3, 4]]
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Point, feature.getType());
    auto& points = feature.getPoints();
    ASSERT_EQ(2u, points.size()); // points
    EXPECT_EQ(Point({ { 1, 2 } }), points[0]);
    EXPECT_EQ(Point({ { 3, 4 } }), points[1]);
}
TEST(GeoJSON, MultiPointDuplicateMember) {
    GeoJSON geojson(R"GEOJSON({
  "type": "MultiPoint",
  "coordinates": [[0, 1], [2, 3]],
  "coordinates": [[1, 2], [3, 4]]
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Point, feature.getType());
    auto& points = feature.getPoints();
    ASSERT_EQ(2u, points.size()); // points
    EXPECT_EQ(Point({ { 1, 2 } }), points[0]);
    EXPECT_EQ(Point({ { 3, 4 } }), points[1]);
}

TEST(GeoJSON, MultiPointTypeAfter) {
    GeoJSON geojson(R"GEOJSON({
  "coordinates": [[1, 2], [3, 4]],
  "type": "MultiPoint"
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Point, feature.getType());
    auto& points = feature.getPoints();
    ASSERT_EQ(2u, points.size()); // points
    EXPECT_EQ(Point({ { 1, 2 } }), points[0]);
    EXPECT_EQ(Point({ { 3, 4 } }), points[1]);
}

TEST(GeoJSON, MultiPointWrongNesting) {
    GeoJSON geojson(R"GEOJSON({
  "type": "MultiPoint",
  "coordinates": [[[1, 2], [3, 4]]]
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ("Expected a coordinate, but got an array instead.", geojson.getError().message);
    EXPECT_EQ(2u, geojson.getError().row);
    EXPECT_EQ(20u, geojson.getError().column);
}

TEST(GeoJSON, MultiPointTypeAfterAndWrongNesting) {
    GeoJSON geojson(R"GEOJSON({
  "coordinates": [[[1, 2], [3, 4]]],
  "type": "MultiPoint"
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ("Expected 'Polygon', or 'MultiLineString', but got 'MultiPoint' instead.",
              geojson.getError().message);
    EXPECT_EQ(2u, geojson.getError().row);
    EXPECT_EQ(22u, geojson.getError().column);
}

TEST(GeoJSON, MultiPointEmpty) {
    GeoJSON geojson(R"GEOJSON({
  "type": "MultiPoint",
  "coordinates": []
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Point, feature.getType());
    auto& points = feature.getPoints();
    ASSERT_TRUE(points.empty()); // points
}

TEST(GeoJSON, ExcessCoordinate) {
    GeoJSON geojson(R"GEOJSON({
  "type": "Point",
  "coordinates": [ 1, 2, 3 ]
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Point, feature.getType());
    auto& points = feature.getPoints();
    ASSERT_EQ(1u, points.size()); // points
    Point point{ { 1, 2 } };
    EXPECT_EQ(point, points[0]);
}

TEST(GeoJSON, ExcessCoordinateString) {
    GeoJSON geojson(R"GEOJSON({
  "type": "Point",
  "coordinates": [ 1, 2, "string" ]
})GEOJSON");
    ASSERT_FALSE(geojson.isValid()) << geojson.getError();
    EXPECT_EQ("Expected a closing bracket, or a coordinate, but got a string instead.",
              geojson.getError().message);
    EXPECT_EQ(2u, geojson.getError().row);
    EXPECT_EQ(33u, geojson.getError().column);
}

TEST(GeoJSON, LineString) {
    GeoJSON geojson(R"GEOJSON({
  "type": "LineString",
  "coordinates": [[ 1, 2 ], [3, 4]]
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Line, feature.getType());
    auto& lines = feature.getLines();
    ASSERT_EQ(1u, lines.size()); // lines
    auto& line = lines[0];
    EXPECT_EQ(Point({ { 1, 2 } }), line[0]);
    EXPECT_EQ(Point({ { 3, 4 } }), line[1]);
}

TEST(GeoJSON, MultiLineString) {
    GeoJSON geojson(R"GEOJSON({
  "type": "MultiLineString",
  "coordinates": [[[ 1, 2 ], [3, 4]]]
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Line, feature.getType());
    auto& lines = feature.getLines();
    ASSERT_EQ(1u, lines.size()); // lines
    auto& line = lines[0];
    EXPECT_EQ(Point({ { 1, 2 } }), line[0]);
    EXPECT_EQ(Point({ { 3, 4 } }), line[1]);
}

TEST(GeoJSON, MultiLineStringTypeAfter) {
    GeoJSON geojson(R"GEOJSON({
  "coordinates": [[[ 1, 2 ], [3, 4]]],
  "type": "MultiLineString"
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Line, feature.getType());
    auto& lines = feature.getLines();
    ASSERT_EQ(1u, lines.size()); // lines
    auto& line = lines[0];
    EXPECT_EQ(Point({ { 1, 2 } }), line[0]);
    EXPECT_EQ(Point({ { 3, 4 } }), line[1]);
}

TEST(GeoJSON, MultiLineStringWrongNesting) {
    GeoJSON geojson(R"GEOJSON({
  "type": "MultiLineString",
  "coordinates": [[ 1, 2 ], [3, 4]]
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ("Expected a coordinate array, but got a number instead.", geojson.getError().message);
    EXPECT_EQ(2u, geojson.getError().row);
    EXPECT_EQ(21u, geojson.getError().column);
}

TEST(GeoJSON, MultiLineStringTypeAfterAndWrongNesting) {
    GeoJSON geojson(R"GEOJSON({
  "coordinates": [[ 1, 2 ], [3, 4]],
  "type": "MultiLineString"
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ("Expected 'LineString', or 'MultiPoint', but got 'MultiLineString' instead.",
              geojson.getError().message);
    EXPECT_EQ(2u, geojson.getError().row);
    EXPECT_EQ(27u, geojson.getError().column);
}

TEST(GeoJSON, Polygon) {
    GeoJSON geojson(R"GEOJSON({
  "type": "Polygon",
  "coordinates": [ [ [ 100, 0 ], [ 101.0, 0 ], [ 101.0, 1.0 ], [ 100.0, 1.0 ], [ 100.0, 0.0 ] ] ]
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Polygon, feature.getType());
    auto& polygon = feature.getPolygon();
    ASSERT_EQ(1u, polygon.size());    // rings
    ASSERT_EQ(5u, polygon[0].size()); // first ring
    ASSERT_EQ(Points({ { { 100, 0 } },
                       { { 101.0, 0 } },
                       { { 101.0, 1.0 } },
                       { { 100.0, 1.0 } },
                       { { 100.0, 0.0 } } }),
              polygon[0]);
}

TEST(GeoJSON, PolygonTypeAfter) {
    GeoJSON geojson(R"GEOJSON({
  "coordinates": [ [ [ 100, 0 ], [ 101.0, 0 ], [ 101.0, 1.0 ], [ 100.0, 1.0 ], [ 100.0, 0.0 ] ] ],
  "type": "Polygon"
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Polygon, feature.getType());
    auto& polygon = feature.getPolygon();
    ASSERT_EQ(1u, polygon.size());    // rings
    ASSERT_EQ(5u, polygon[0].size()); // first ring
    ASSERT_EQ(Points({ { { 100, 0 } },
                       { { 101.0, 0 } },
                       { { 101.0, 1.0 } },
                       { { 100.0, 1.0 } },
                       { { 100.0, 0.0 } } }),
              polygon[0]);
}

TEST(GeoJSON, PolygonNoType) {
    GeoJSON geojson(R"GEOJSON({
  "coordinates": [ [ [ 100, 0 ], [ 101.0, 0 ], [ 101.0, 1.0 ], [ 100.0, 1.0 ], [ 100.0, 0.0 ] ] ]
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ(
        "Expected a key 'type' in the root object, but got a prematurely closed object instead.",
        geojson.getError().message);
    EXPECT_EQ(2u, geojson.getError().row);
    EXPECT_EQ(1u, geojson.getError().column);
}

TEST(GeoJSON, PolygonNoClosingBrace) {
    GeoJSON geojson(R"GEOJSON({
  "type": "Polygon",
  "coordinates": [ [ [ 100, 0 ], [ 101.0, 0 ], [ 101.0, 1.0 ], [ 100.0, 1.0 ], [ 100.0, 0.0 ] ] ]
)GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ("Missing a comma or '}' after an object member.", geojson.getError().message);
    EXPECT_EQ(3u, geojson.getError().row);
    EXPECT_EQ(1u, geojson.getError().column);
}

TEST(GeoJSON, Feature) {
    GeoJSON geojson(R"GEOJSON({
  "type": "Feature",
  "geometry": {
    "type": "Polygon",
    "coordinates": [ [ [ 100, 0 ], [ 101.0, 0 ], [ 101.0, 1.0 ], [ 100.0, 1.0 ], [ 100.0, 0.0 ] ] ]
  }
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Polygon, feature.getType());
    auto& polygon = feature.getPolygon();
    ASSERT_EQ(1u, polygon.size()); // rings
    ASSERT_EQ(Points({ { { 100, 0 } },
                       { { 101.0, 0 } },
                       { { 101.0, 1.0 } },
                       { { 100.0, 1.0 } },
                       { { 100.0, 0.0 } } }),
              polygon[0]);
}

TEST(GeoJSON, FeatureWithIgnoredCoordinates) {
    GeoJSON geojson(R"GEOJSON({
  "type": "Feature",
  "coordinates": [],
  "geometry": {
    "type": "Polygon",
    "coordinates": [ [ [ 100, 0 ], [ 101.0, 0 ], [ 101.0, 1.0 ], [ 100.0, 1.0 ], [ 100.0, 0.0 ] ] ]
  }
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Polygon, feature.getType());
    auto& polygon = feature.getPolygon();
    ASSERT_EQ(1u, polygon.size()); // rings
    ASSERT_EQ(Points({ { { 100, 0 } },
                       { { 101.0, 0 } },
                       { { 101.0, 1.0 } },
                       { { 100.0, 1.0 } },
                       { { 100.0, 0.0 } } }),
              polygon[0]);
}

TEST(GeoJSON, FeatureWithIgnoredCoordinatesAndTypeInBetween) {
    GeoJSON geojson(R"GEOJSON({
  "coordinates": [[[0,0],["yes"]]],
  "type": "Feature",
  "geometry": {
    "type": "Polygon",
    "coordinates": [ [ [ 100, 0 ], [ 101.0, 0 ], [ 101.0, 1.0 ], [ 100.0, 1.0 ], [ 100.0, 0.0 ] ] ]
  }
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Polygon, feature.getType());
    auto& polygon = feature.getPolygon();
    ASSERT_EQ(1u, polygon.size()); // rings
    ASSERT_EQ(Points({ { { 100, 0 } },
                       { { 101.0, 0 } },
                       { { 101.0, 1.0 } },
                       { { 100.0, 1.0 } },
                       { { 100.0, 0.0 } } }),
              polygon[0]);
}

TEST(GeoJSON, FeatureWithIgnoredCoordinatesAndTypeAfter) {
    GeoJSON geojson(R"GEOJSON({
  "geometry": {
    "type": "Polygon",
    "coordinates": [ [ [ 100, 0 ], [ 101.0, 0 ], [ 101.0, 1.0 ], [ 100.0, 1.0 ], [ 100.0, 0.0 ] ] ]
  },
  "coordinates": [[[0,0]]],
  "type": "Feature"
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Polygon, feature.getType());
    auto& polygon = feature.getPolygon();
    ASSERT_EQ(1u, polygon.size()); // rings
    ASSERT_EQ(Points({ { { 100, 0 } },
                       { { 101.0, 0 } },
                       { { 101.0, 1.0 } },
                       { { 100.0, 1.0 } },
                       { { 100.0, 0.0 } } }),
              polygon[0]);
}

TEST(GeoJSON, FeatureWithoutGeometry) {
    GeoJSON geojson(R"GEOJSON({
  "coordinates": [[[0,0]]],
  "type": "Feature"
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ("Expected a 'geometry' key value pair, but got a prematurely closed object instead.",
              geojson.getError().message);
    EXPECT_EQ(3u, geojson.getError().row);
    EXPECT_EQ(1u, geojson.getError().column);
}

TEST(GeoJSON, PointFeature) {
    GeoJSON geojson(R"GEOJSON({
  "type": "Feature",
  "geometry": {
    "type": "Point",
    "coordinates": [ 2, 3 ]
  }
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Point, feature.getType());
    auto& points = feature.getPoints();
    ASSERT_EQ(Points({ { { 2, 3 } } }), points);
}

TEST(GeoJSON, PointFeatureTypeAfter) {
    GeoJSON geojson(R"GEOJSON({
  "geometry": {
    "type": "Point",
    "coordinates": [ 2, 3 ]
  },
  "type": "Feature"
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Point, feature.getType());
    auto& points = feature.getPoints();
    ASSERT_EQ(Points({ { { 2, 3 } } }), points);
}

TEST(GeoJSON, PointFeatureTypeAfter2x) {
    GeoJSON geojson(R"GEOJSON({
  "geometry": {
    "coordinates": [ 2, 3 ],
    "type": "Point"
  },
  "type": "Feature"
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Point, feature.getType());
    auto& points = feature.getPoints();
    ASSERT_EQ(Points({ { { 2, 3 } } }), points);
}

TEST(GeoJSON, PointFeatureWithoutType) {
    GeoJSON geojson(R"GEOJSON({
  "geometry": {
    "coordinates": [ 2, 3 ]
  },
  "type": "Feature"
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ("Expected a key 'type' in the geometry object, but got a prematurely closed object "
              "instead.",
              geojson.getError().message);
    EXPECT_EQ(3u, geojson.getError().row);
    EXPECT_EQ(3u, geojson.getError().column);
}

TEST(GeoJSON, PointFeatureWithoutCoordinates) {
    GeoJSON geojson(R"GEOJSON({
  "geometry": {
    "type": "Point"
  },
  "type": "Feature"
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ(
        "Expected a 'coordinates' key value pair, but got a prematurely closed object instead.",
        geojson.getError().message);
    EXPECT_EQ(3u, geojson.getError().row);
    EXPECT_EQ(3u, geojson.getError().column);
}

TEST(GeoJSON, PointFeatureInvalidCoordinates1) {
    GeoJSON geojson(R"GEOJSON({
  "geometry": {
    "type": "Point",
    "coordinates": [ ]
  },
  "type": "Feature"
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ(
        "Expected a coordinate, but got a prematurely closed array instead.",
        geojson.getError().message);
    EXPECT_EQ(3u, geojson.getError().row);
    EXPECT_EQ(22u, geojson.getError().column);
}

TEST(GeoJSON, PointFeatureInvalidCoordinates2) {
    GeoJSON geojson(R"GEOJSON({
  "geometry": {
    "type": "Point",
    "coordinates": [ 2 ]
  },
  "type": "Feature"
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ(
        "Expected a second coordinate, but got a prematurely closed array instead.",
        geojson.getError().message);
    EXPECT_EQ(3u, geojson.getError().row);
    EXPECT_EQ(24u, geojson.getError().column);
}

TEST(GeoJSON, PointFeatureInvalidCoordinates3) {
    GeoJSON geojson(R"GEOJSON({
  "geometry": {
    "type": "Point",
    "coordinates": [ [ ] ]
  },
  "type": "Feature"
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ(
        "Expected a coordinate, but got an array instead.",
        geojson.getError().message);
    EXPECT_EQ(3u, geojson.getError().row);
    EXPECT_EQ(22u, geojson.getError().column);
}

TEST(GeoJSON, PointFeatureInvalidCoordinates4) {
    GeoJSON geojson(R"GEOJSON({
  "geometry": {
    "type": "Point",
    "coordinates": [ false ]
  },
  "type": "Feature"
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ(
        "Expected a coordinate array, but got a boolean instead.",
        geojson.getError().message);
    EXPECT_EQ(3u, geojson.getError().row);
    EXPECT_EQ(26u, geojson.getError().column);
}


TEST(GeoJSON, PointFeatureInvalidCoordinates5) {
    GeoJSON geojson(R"GEOJSON({
  "geometry": {
    "type": "Point",
    "coordinates": null
  },
  "type": "Feature"
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ(
        "Expected a coordinate array, but got a null object instead.",
        geojson.getError().message);
    EXPECT_EQ(3u, geojson.getError().row);
    EXPECT_EQ(23u, geojson.getError().column);
}


TEST(GeoJSON, PointFeatureGeometryTypeAfterInvalidCoordinates1) {
    GeoJSON geojson(R"GEOJSON({
  "geometry": {
    "coordinates": [ ],
    "type": "Point"
  },
  "type": "Feature"
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ(
        "Expected a coordinate, but got a prematurely closed array instead.",
        geojson.getError().message);
    EXPECT_EQ(2u, geojson.getError().row);
    EXPECT_EQ(22u, geojson.getError().column);
}

TEST(GeoJSON, PointFeatureGeometryTypeAfterInvalidCoordinates2) {
    GeoJSON geojson(R"GEOJSON({
  "geometry": {
    "coordinates": [ 2 ],
    "type": "Point"
  },
  "type": "Feature"
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ(
        "Expected a second coordinate, but got a prematurely closed array instead.",
        geojson.getError().message);
    EXPECT_EQ(2u, geojson.getError().row);
    EXPECT_EQ(24u, geojson.getError().column);
}

TEST(GeoJSON, PointFeatureGeometryTypeAfterInvalidCoordinates3) {
    GeoJSON geojson(R"GEOJSON({
  "geometry": {
    "coordinates": [ [ ] ],
    "type": "Point"
  },
  "type": "Feature"
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ(
        "Expected a coordinate, but got a prematurely closed array instead.",
        geojson.getError().message);
    EXPECT_EQ(2u, geojson.getError().row);
    EXPECT_EQ(24u, geojson.getError().column);
}

TEST(GeoJSON, PointFeatureGeometryTypeAfterInvalidCoordinates4) {
    GeoJSON geojson(R"GEOJSON({
  "geometry": {
    "coordinates": [ false ]
    "type": "Point"
  },
  "type": "Feature"
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ(
        "Expected a coordinate array, but got a boolean instead.",
        geojson.getError().message);
    EXPECT_EQ(2u, geojson.getError().row);
    EXPECT_EQ(26u, geojson.getError().column);
}


TEST(GeoJSON, PointFeatureGeometryTypeAfterInvalidCoordinates5) {
    GeoJSON geojson(R"GEOJSON({
  "geometry": {
    "coordinates": null
    "type": "Point"
  },
  "type": "Feature"
})GEOJSON");
    ASSERT_FALSE(geojson.isValid());
    EXPECT_EQ(
        "Expected a coordinate array, but got a null object instead.",
        geojson.getError().message);
    EXPECT_EQ(2u, geojson.getError().row);
    EXPECT_EQ(23u, geojson.getError().column);
}

TEST(GeoJSON, PointFeatureTypeAfterCoordinates) {
    GeoJSON geojson(R"GEOJSON({
  "coordinates": [ 1, 1 ],
  "geometry": {
    "coordinates": [ 2, 3 ],
    "type": "Point"
  },
  "coordinates": [ 1, 1 ],
  "type": "Feature"
})GEOJSON");
    ASSERT_TRUE(geojson.isValid()) << geojson.getError();
    ASSERT_EQ(1u, geojson.size());
    auto& feature = geojson[0];
    ASSERT_EQ(Geometry::Type::Point, feature.getType());
    auto& points = feature.getPoints();
    ASSERT_EQ(Points({ { { 2, 3 } } }), points);
}
