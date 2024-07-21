TEST_F(OptimizePipeline, MultipleMatchesPushedDown) {
    auto unpack = fromjson(
        "{$_internalUnpackBucket: { exclude: [], timeField: 'time', metaField: 'myMeta', "
        "bucketMaxSpanSeconds: 3600}}");
    auto pipeline = Pipeline::parse(makeVector(unpack,
                                               fromjson("{$match: {myMeta: {$gte: 0, $lte: 5}}}"),
                                               fromjson("{$match: {a: {$lte: 4}}}")),
                                    getExpCtx());
    ASSERT_EQ(3u, pipeline->getSources().size());

    pipeline->optimizePipeline();

    // We should push down both the $match on the metaField and the predicates on the control field.
    // The created $match stages should be added before $_internalUnpackBucket and merged.
    auto stages = pipeline->writeExplainOps(ExplainOptions::Verbosity::kQueryPlanner);
    ASSERT_EQ(3u, stages.size());
    ASSERT_BSONOBJ_EQ(fromjson("{$match: {$and: [{'control.min.a': {$_internalExprLte: 4}}, {meta: "
                               "{$gte: 0}}, {meta: {$lte: 5}}]}}"),
                      stages[0].getDocument().toBson());
    ASSERT_BSONOBJ_EQ(unpack, stages[1].getDocument().toBson());
    ASSERT_BSONOBJ_EQ(fromjson("{$match: {a: {$lte: 4}}}"), stages[2].getDocument().toBson());
}