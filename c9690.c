TEST_F(QueryPlannerTest, SparseIndexCanSupportGTEOrLTENull) {
    params.options &= ~QueryPlannerParams::INCLUDE_COLLSCAN;
    addIndex(BSON("i" << 1),
             false,  // multikey
             true    // sparse
    );

    runQuery(fromjson("{i: {$gte: null}}"));
    assertNumSolutions(1U);
    assertSolutionExists(
        "{fetch: {filter: {i: {$gte: null}}, node: {ixscan: {pattern: "
        "{i: 1}, bounds: {i: [[null,null,true,true]]}}}}}");

    runQuery(fromjson("{i: {$lte: null}}"));
    assertNumSolutions(1U);
    assertSolutionExists(
        "{fetch: {filter: {i: {$lte: null}}, node: {ixscan: {pattern: "
        "{i: 1}, bounds: {i: [[null,null,true,true]]}}}}}");
}