DocumentSource::GetNextResult DocumentSourceGroup::initialize() {
    const size_t numAccumulators = _accumulatedFields.size();

    // Barring any pausing, this loop exhausts 'pSource' and populates '_groups'.
    GetNextResult input = pSource->getNext();

    for (; input.isAdvanced(); input = pSource->getNext()) {
        if (_memoryTracker.shouldSpillWithAttemptToSaveMemory([this]() { return freeMemory(); })) {
            _sortedFiles.push_back(spill());
        }

        // We release the result document here so that it does not outlive the end of this loop
        // iteration. Not releasing could lead to an array copy when this group follows an unwind.
        auto rootDocument = input.releaseDocument();
        Value id = computeId(rootDocument);

        // Look for the _id value in the map. If it's not there, add a new entry with a blank
        // accumulator. This is done in a somewhat odd way in order to avoid hashing 'id' and
        // looking it up in '_groups' multiple times.
        const size_t oldSize = _groups->size();
        vector<intrusive_ptr<AccumulatorState>>& group = (*_groups)[id];
        const bool inserted = _groups->size() != oldSize;

        if (inserted) {
            _memoryTracker.memoryUsageBytes += id.getApproximateSize();

            // Initialize and add the accumulators
            Value expandedId = expandId(id);
            Document idDoc =
                expandedId.getType() == BSONType::Object ? expandedId.getDocument() : Document();
            group.reserve(numAccumulators);
            for (auto&& accumulatedField : _accumulatedFields) {
                auto accum = accumulatedField.makeAccumulator();
                Value initializerValue =
                    accumulatedField.expr.initializer->evaluate(idDoc, &pExpCtx->variables);
                accum->startNewGroup(initializerValue);
                group.push_back(accum);
            }
        } else {
            for (auto&& groupObj : group) {
                // subtract old mem usage. New usage added back after processing.
                _memoryTracker.memoryUsageBytes -= groupObj->memUsageForSorter();
            }
        }

        /* tickle all the accumulators for the group we found */
        dassert(numAccumulators == group.size());

        for (size_t i = 0; i < numAccumulators; i++) {
            group[i]->process(
                _accumulatedFields[i].expr.argument->evaluate(rootDocument, &pExpCtx->variables),
                _doingMerge);

            _memoryTracker.memoryUsageBytes += group[i]->memUsageForSorter();
        }

        if (kDebugBuild && !storageGlobalParams.readOnly) {
            // In debug mode, spill every time we have a duplicate id to stress merge logic.
            if (!inserted &&                     // is a dup
                !pExpCtx->inMongos &&            // can't spill to disk in mongos
                !_memoryTracker.allowDiskUse &&  // don't change behavior when testing external sort
                _sortedFiles.size() < 20) {      // don't open too many FDs

                _sortedFiles.push_back(spill());
            }
        }
    }

    switch (input.getStatus()) {
        case DocumentSource::GetNextResult::ReturnStatus::kAdvanced: {
            MONGO_UNREACHABLE;  // We consumed all advances above.
        }
        case DocumentSource::GetNextResult::ReturnStatus::kPauseExecution: {
            return input;  // Propagate pause.
        }
        case DocumentSource::GetNextResult::ReturnStatus::kEOF: {
            // Do any final steps necessary to prepare to output results.
            if (!_sortedFiles.empty()) {
                _spilled = true;
                if (!_groups->empty()) {
                    _sortedFiles.push_back(spill());
                }

                // We won't be using groups again so free its memory.
                _groups = pExpCtx->getValueComparator().makeUnorderedValueMap<Accumulators>();

                _sorterIterator.reset(Sorter<Value, Value>::Iterator::merge(
                    _sortedFiles, SortOptions(), SorterComparator(pExpCtx->getValueComparator())));

                // prepare current to accumulate data
                _currentAccumulators.reserve(numAccumulators);
                for (auto&& accumulatedField : _accumulatedFields) {
                    _currentAccumulators.push_back(accumulatedField.makeAccumulator());
                }

                verify(_sorterIterator->more());  // we put data in, we should get something out.
                _firstPartOfNextGroup = _sorterIterator->next();
            } else {
                // start the group iterator
                groupsIterator = _groups->begin();
            }

            // This must happen last so that, unless control gets here, we will re-enter
            // initialization after getting a GetNextResult::ResultState::kPauseExecution.
            _initialized = true;
            return input;
        }
    }
    MONGO_UNREACHABLE;
}