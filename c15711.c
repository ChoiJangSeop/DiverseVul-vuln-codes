TEST_F(InternalUnpackBucketSplitMatchOnMetaAndRename, OptimizeSplitsMatchAndMapsControlPredicates) {
    auto unpack = fromjson(
        "{$_internalUnpackBucket: { exclude: [], timeField: 'foo', metaField: 'myMeta', "
        "bucketMaxSpanSeconds: 3600}}");
    auto pipeline = Pipeline::parse(
        makeVector(unpack, fromjson("{$match: {myMeta: {$gte: 0, $lte: 5}, a: {$lte: 4}}}")),
        getExpCtx());
    ASSERT_EQ(2u, pipeline->getSources().size());

    pipeline->optimizePipeline();

    // We should split and rename the $match. A separate optimization maps the predicate on 'a' to a
    // predicate on 'control.min.a'. These two created $match stages should be added before
    // $_internalUnpackBucket and merged.
    auto serialized = pipeline->serializeToBson();
    ASSERT_EQ(3u, serialized.size());
    ASSERT_BSONOBJ_EQ(fromjson("{$match: {$and: [{$and: [{meta: {$gte: 0}}, {meta: {$lte: 5}}]}, "
                               "{'control.min.a': {$_internalExprLte: 4}}]}}"),
                      serialized[0]);
    ASSERT_BSONOBJ_EQ(unpack, serialized[1]);
    ASSERT_BSONOBJ_EQ(fromjson("{$match: {a: {$lte: 4}}}"), serialized[2]);
}