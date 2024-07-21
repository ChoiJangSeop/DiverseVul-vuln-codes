void parseInclusion(ParseContext* ctx,
                    BSONElement elem,
                    ProjectionPathASTNode* parent,
                    boost::optional<FieldPath> fullPathToParent) {
    // There are special rules about _id being included. _id may be included in both inclusion and
    // exclusion projections.
    const bool isTopLevelIdProjection = elem.fieldNameStringData() == "_id" && parent->isRoot();

    const bool hasPositional = hasPositionalOperator(elem.fieldNameStringData());

    if (!hasPositional) {
        FieldPath path(elem.fieldNameStringData());
        addNodeAtPath(parent, path, std::make_unique<BooleanConstantASTNode>(true));

        if (isTopLevelIdProjection) {
            ctx->idIncludedEntirely = true;
        }
    } else {
        verifyComputedFieldsAllowed(ctx->policies);

        uassert(31276,
                "Cannot specify more than one positional projection per query.",
                !ctx->hasPositional);

        uassert(31256, "Cannot specify positional operator and $elemMatch.", !ctx->hasElemMatch);
        uassert(51050, "Projections with a positional operator require a matcher", ctx->query);

        // Get everything up to the first positional operator.
        // Should at least be ".$"
        StringData elemFieldName = elem.fieldNameStringData();
        invariant(elemFieldName.size() > 2);
        StringData pathWithoutPositionalOperator =
            elemFieldName.substr(0, elemFieldName.size() - 2);

        FieldPath path(pathWithoutPositionalOperator);

        auto matcher = CopyableMatchExpression{ctx->queryObj,
                                               ctx->expCtx,
                                               std::make_unique<ExtensionsCallbackNoop>(),
                                               MatchExpressionParser::kBanAllSpecialFeatures,
                                               true /* optimize expression */};

        invariant(ctx->query);
        addNodeAtPath(parent,
                      path,
                      std::make_unique<ProjectionPositionalASTNode>(
                          std::make_unique<MatchExpressionASTNode>(matcher)));

        ctx->hasPositional = true;
    }

    if (!isTopLevelIdProjection) {
        uassert(31253,
                str::stream() << "Cannot do inclusion on field " << elem.fieldNameStringData()
                              << " in exclusion projection",
                !ctx->type || *ctx->type == ProjectType::kInclusion);
        ctx->type = ProjectType::kInclusion;
    }
}