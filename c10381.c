TEST(QueryProjectionTest, InvalidElemMatchGeoNearProjection) {
    assertInvalidProjection(
        "{}",
        "{a: {$elemMatch: {$nearSphere: {$geometry: {type: 'Point', coordinates: [0, 0]}}}}}");
}