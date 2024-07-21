DocumentSource::GetNextResult DocumentSourceUnionWith::doGetNext() {
    if (!_pipeline) {
        // We must have already been disposed, so we're finished.
        return GetNextResult::makeEOF();
    }

    if (_executionState == ExecutionProgress::kIteratingSource) {
        auto nextInput = pSource->getNext();
        if (!nextInput.isEOF()) {
            return nextInput;
        }
        _executionState = ExecutionProgress::kStartingSubPipeline;
        // All documents from the base collection have been returned, switch to iterating the sub-
        // pipeline by falling through below.
    }

    if (_executionState == ExecutionProgress::kStartingSubPipeline) {
        auto serializedPipe = _pipeline->serializeToBson();
        LOGV2_DEBUG(23869,
                    1,
                    "$unionWith attaching cursor to pipeline {pipeline}",
                    "pipeline"_attr = serializedPipe);
        // $$SEARCH_META can be set during runtime earlier in the pipeline, and therefore must be
        // copied to the subpipeline manually.
        if (pExpCtx->variables.hasConstantValue(Variables::kSearchMetaId)) {
            _pipeline->getContext()->variables.setReservedValue(
                Variables::kSearchMetaId,
                pExpCtx->variables.getValue(Variables::kSearchMetaId, Document()),
                true);
        }
        try {
            _pipeline =
                pExpCtx->mongoProcessInterface->attachCursorSourceToPipeline(_pipeline.release());
            _executionState = ExecutionProgress::kIteratingSubPipeline;
        } catch (const ExceptionFor<ErrorCodes::CommandOnShardedViewNotSupportedOnMongod>& e) {
            _pipeline = buildPipelineFromViewDefinition(
                pExpCtx,
                ExpressionContext::ResolvedNamespace{e->getNamespace(), e->getPipeline()},
                serializedPipe);
            LOGV2_DEBUG(4556300,
                        3,
                        "$unionWith found view definition. ns: {ns}, pipeline: {pipeline}. New "
                        "$unionWith sub-pipeline: {new_pipe}",
                        "ns"_attr = e->getNamespace(),
                        "pipeline"_attr = Value(e->getPipeline()),
                        "new_pipe"_attr = _pipeline->serializeToBson());
            return doGetNext();
        }
    }

    auto res = _pipeline->getNext();
    if (res)
        return std::move(*res);

    // Record the plan summary stats after $unionWith operation is done.
    recordPlanSummaryStats(*_pipeline);

    _executionState = ExecutionProgress::kFinished;
    return GetNextResult::makeEOF();
}