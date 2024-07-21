DocumentSourceGraphLookUp::DocumentSourceGraphLookUp(
    const boost::intrusive_ptr<ExpressionContext>& expCtx,
    NamespaceString from,
    std::string as,
    std::string connectFromField,
    std::string connectToField,
    boost::intrusive_ptr<Expression> startWith,
    boost::optional<BSONObj> additionalFilter,
    boost::optional<FieldPath> depthField,
    boost::optional<long long> maxDepth,
    boost::optional<boost::intrusive_ptr<DocumentSourceUnwind>> unwindSrc)
    : DocumentSource(kStageName, expCtx),
      _from(std::move(from)),
      _as(std::move(as)),
      _connectFromField(std::move(connectFromField)),
      _connectToField(std::move(connectToField)),
      _startWith(std::move(startWith)),
      _additionalFilter(additionalFilter),
      _depthField(depthField),
      _maxDepth(maxDepth),
      _frontier(pExpCtx->getValueComparator().makeUnorderedValueSet()),
      _visited(ValueComparator::kInstance.makeUnorderedValueMap<Document>()),
      _cache(pExpCtx->getValueComparator()),
      _unwind(unwindSrc) {
    const auto& resolvedNamespace = pExpCtx->getResolvedNamespace(_from);
    _fromExpCtx = pExpCtx->copyWith(resolvedNamespace.ns);
    _fromPipeline = resolvedNamespace.pipeline;

    // We append an additional BSONObj to '_fromPipeline' as a placeholder for the $match stage
    // we'll eventually construct from the input document.
    _fromPipeline.reserve(_fromPipeline.size() + 1);
    _fromPipeline.push_back(BSON("$match" << BSONObj()));
}