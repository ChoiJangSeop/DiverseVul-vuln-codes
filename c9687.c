void IndexBoundsBuilder::_translatePredicate(const MatchExpression* expr,
                                             const BSONElement& elt,
                                             const IndexEntry& index,
                                             OrderedIntervalList* oilOut,
                                             BoundsTightness* tightnessOut) {
    // We expect that the OIL we are constructing starts out empty.
    invariant(oilOut->intervals.empty());

    oilOut->name = elt.fieldName();

    bool isHashed = false;
    if (elt.valueStringDataSafe() == "hashed") {
        isHashed = true;
    }

    if (isHashed) {
        invariant(MatchExpression::MATCH_IN == expr->matchType() ||
                  ComparisonMatchExpressionBase::isEquality(expr->matchType()));
    }

    if (MatchExpression::ELEM_MATCH_VALUE == expr->matchType()) {
        OrderedIntervalList acc;
        _translatePredicate(expr->getChild(0), elt, index, &acc, tightnessOut);

        for (size_t i = 1; i < expr->numChildren(); ++i) {
            OrderedIntervalList next;
            BoundsTightness tightness;
            _translatePredicate(expr->getChild(i), elt, index, &next, &tightness);
            intersectize(next, &acc);
        }

        for (size_t i = 0; i < acc.intervals.size(); ++i) {
            oilOut->intervals.push_back(acc.intervals[i]);
        }

        if (!oilOut->intervals.empty()) {
            std::sort(oilOut->intervals.begin(), oilOut->intervals.end(), IntervalComparison);
        }

        // $elemMatch value requires an array.
        // Scalars and directly nested objects are not matched with $elemMatch.
        // We can't tell if a multi-key index key is derived from an array field.
        // Therefore, a fetch is required.
        *tightnessOut = IndexBoundsBuilder::INEXACT_FETCH;
    } else if (MatchExpression::NOT == expr->matchType()) {
        // A NOT is indexed by virtue of its child. If we're here then the NOT's child
        // must be a kind of node for which we can index negations. It can't be things like
        // $mod, $regex, or $type.
        MatchExpression* child = expr->getChild(0);

        // If we have a NOT -> EXISTS, we must handle separately.
        if (MatchExpression::EXISTS == child->matchType()) {
            // We should never try to use a sparse index for $exists:false.
            invariant(!index.sparse);
            BSONObjBuilder bob;
            bob.appendNull("");
            bob.appendNull("");
            BSONObj dataObj = bob.obj();
            oilOut->intervals.push_back(
                makeRangeInterval(dataObj, BoundInclusion::kIncludeBothStartAndEndKeys));

            *tightnessOut = IndexBoundsBuilder::INEXACT_FETCH;
            return;
        }

        _translatePredicate(child, elt, index, oilOut, tightnessOut);
        oilOut->complement();

        // Until the index distinguishes between missing values and literal null values, we cannot
        // build exact bounds for equality predicates on the literal value null. However, we _can_
        // build exact bounds for the inverse, for example the query {a: {$ne: null}}.
        if (isEqualityOrInNull(child)) {
            *tightnessOut = IndexBoundsBuilder::EXACT;
        }

        // If this invariant would fail, we would otherwise return incorrect query results.
        invariant(*tightnessOut == IndexBoundsBuilder::EXACT);

        // If the index is multikey on this path, it doesn't matter what the tightness of the child
        // is, we must return INEXACT_FETCH. Consider a multikey index on 'a' with document
        // {a: [1, 2, 3]} and query {a: {$ne: 3}}. If we treated the bounds [MinKey, 3), (3, MaxKey]
        // as exact, then we would erroneously return the document!
        if (index.pathHasMultikeyComponent(elt.fieldNameStringData())) {
            *tightnessOut = IndexBoundsBuilder::INEXACT_FETCH;
        }
    } else if (MatchExpression::EXISTS == expr->matchType()) {
        oilOut->intervals.push_back(allValues());

        // We only handle the {$exists:true} case, as {$exists:false}
        // will have been translated to {$not:{ $exists:true }}.
        //
        // Documents with a missing value are stored *as if* they were
        // explicitly given the value 'null'.  Given:
        //    X = { b : 1 }
        //    Y = { a : null, b : 1 }
        // X and Y look identical from within a standard index on { a : 1 }.
        // HOWEVER a sparse index on { a : 1 } will treat X and Y differently,
        // storing Y and not storing X.
        //
        // We can safely use an index in the following cases:
        // {a:{ $exists:true }} - normal index helps, but we must still fetch
        // {a:{ $exists:true }} - sparse index is exact
        // {a:{ $exists:false }} - normal index requires a fetch
        // {a:{ $exists:false }} - sparse indexes cannot be used at all.
        //
        // Noted in SERVER-12869, in case this ever changes some day.
        if (index.sparse) {
            // A sparse, compound index on { a:1, b:1 } will include entries
            // for all of the following documents:
            //    { a:1 }, { b:1 }, { a:1, b:1 }
            // So we must use INEXACT bounds in this case.
            if (1 < index.keyPattern.nFields()) {
                *tightnessOut = IndexBoundsBuilder::INEXACT_FETCH;
            } else {
                *tightnessOut = IndexBoundsBuilder::EXACT;
            }
        } else {
            *tightnessOut = IndexBoundsBuilder::INEXACT_FETCH;
        }
    } else if (ComparisonMatchExpressionBase::isEquality(expr->matchType())) {
        const auto* node = static_cast<const ComparisonMatchExpressionBase*>(expr);
        // There is no need to sort intervals or merge overlapping intervals here since the output
        // is from one element.
        translateEquality(node->getData(), index, isHashed, oilOut, tightnessOut);
    } else if (MatchExpression::LTE == expr->matchType()) {
        const LTEMatchExpression* node = static_cast<const LTEMatchExpression*>(expr);
        BSONElement dataElt = node->getData();

        // Everything is <= MaxKey.
        if (MaxKey == dataElt.type()) {
            oilOut->intervals.push_back(allValues());
            *tightnessOut =
                index.collator ? IndexBoundsBuilder::INEXACT_FETCH : IndexBoundsBuilder::EXACT;
            return;
        }

        // Only NaN is <= NaN.
        if (std::isnan(dataElt.numberDouble())) {
            double nan = dataElt.numberDouble();
            oilOut->intervals.push_back(makePointInterval(nan));
            *tightnessOut = IndexBoundsBuilder::EXACT;
            return;
        }

        BSONObjBuilder bob;
        // Use -infinity for one-sided numerical bounds
        if (dataElt.isNumber()) {
            bob.appendNumber("", -std::numeric_limits<double>::infinity());
        } else {
            bob.appendMinForType("", dataElt.type());
        }
        CollationIndexKey::collationAwareIndexKeyAppend(dataElt, index.collator, &bob);
        BSONObj dataObj = bob.obj();
        verify(dataObj.isOwned());
        oilOut->intervals.push_back(makeRangeInterval(
            dataObj, IndexBounds::makeBoundInclusionFromBoundBools(typeMatch(dataObj), true)));

        *tightnessOut = getInequalityPredicateTightness(dataElt, index);
    } else if (MatchExpression::LT == expr->matchType()) {
        const LTMatchExpression* node = static_cast<const LTMatchExpression*>(expr);
        BSONElement dataElt = node->getData();

        // Everything is < MaxKey, except for MaxKey.
        if (MaxKey == dataElt.type()) {
            oilOut->intervals.push_back(allValuesRespectingInclusion(
                IndexBounds::makeBoundInclusionFromBoundBools(true, false)));
            *tightnessOut =
                index.collator ? IndexBoundsBuilder::INEXACT_FETCH : IndexBoundsBuilder::EXACT;
            return;
        }

        // Nothing is < NaN.
        if (std::isnan(dataElt.numberDouble())) {
            *tightnessOut = IndexBoundsBuilder::EXACT;
            return;
        }

        BSONObjBuilder bob;
        // Use -infinity for one-sided numerical bounds
        if (dataElt.isNumber()) {
            bob.appendNumber("", -std::numeric_limits<double>::infinity());
        } else {
            bob.appendMinForType("", dataElt.type());
        }
        CollationIndexKey::collationAwareIndexKeyAppend(dataElt, index.collator, &bob);
        BSONObj dataObj = bob.obj();
        verify(dataObj.isOwned());
        Interval interval = makeRangeInterval(
            dataObj, IndexBounds::makeBoundInclusionFromBoundBools(typeMatch(dataObj), false));

        // If the operand to LT is equal to the lower bound X, the interval [X, X) is invalid
        // and should not be added to the bounds.
        if (!interval.isNull()) {
            oilOut->intervals.push_back(interval);
        }

        *tightnessOut = getInequalityPredicateTightness(dataElt, index);
    } else if (MatchExpression::GT == expr->matchType()) {
        const GTMatchExpression* node = static_cast<const GTMatchExpression*>(expr);
        BSONElement dataElt = node->getData();

        // Everything is > MinKey, except MinKey.
        if (MinKey == dataElt.type()) {
            oilOut->intervals.push_back(allValuesRespectingInclusion(
                IndexBounds::makeBoundInclusionFromBoundBools(false, true)));
            *tightnessOut =
                index.collator ? IndexBoundsBuilder::INEXACT_FETCH : IndexBoundsBuilder::EXACT;
            return;
        }

        // Nothing is > NaN.
        if (std::isnan(dataElt.numberDouble())) {
            *tightnessOut = IndexBoundsBuilder::EXACT;
            return;
        }

        BSONObjBuilder bob;
        CollationIndexKey::collationAwareIndexKeyAppend(dataElt, index.collator, &bob);
        if (dataElt.isNumber()) {
            bob.appendNumber("", std::numeric_limits<double>::infinity());
        } else {
            bob.appendMaxForType("", dataElt.type());
        }
        BSONObj dataObj = bob.obj();
        verify(dataObj.isOwned());
        Interval interval = makeRangeInterval(
            dataObj, IndexBounds::makeBoundInclusionFromBoundBools(false, typeMatch(dataObj)));

        // If the operand to GT is equal to the upper bound X, the interval (X, X] is invalid
        // and should not be added to the bounds.
        if (!interval.isNull()) {
            oilOut->intervals.push_back(interval);
        }

        *tightnessOut = getInequalityPredicateTightness(dataElt, index);
    } else if (MatchExpression::GTE == expr->matchType()) {
        const GTEMatchExpression* node = static_cast<const GTEMatchExpression*>(expr);
        BSONElement dataElt = node->getData();

        // Everything is >= MinKey.
        if (MinKey == dataElt.type()) {
            oilOut->intervals.push_back(allValues());
            *tightnessOut =
                index.collator ? IndexBoundsBuilder::INEXACT_FETCH : IndexBoundsBuilder::EXACT;
            return;
        }

        // Only NaN is >= NaN.
        if (std::isnan(dataElt.numberDouble())) {
            double nan = dataElt.numberDouble();
            oilOut->intervals.push_back(makePointInterval(nan));
            *tightnessOut = IndexBoundsBuilder::EXACT;
            return;
        }

        BSONObjBuilder bob;
        CollationIndexKey::collationAwareIndexKeyAppend(dataElt, index.collator, &bob);
        if (dataElt.isNumber()) {
            bob.appendNumber("", std::numeric_limits<double>::infinity());
        } else {
            bob.appendMaxForType("", dataElt.type());
        }
        BSONObj dataObj = bob.obj();
        verify(dataObj.isOwned());

        oilOut->intervals.push_back(makeRangeInterval(
            dataObj, IndexBounds::makeBoundInclusionFromBoundBools(true, typeMatch(dataObj))));

        *tightnessOut = getInequalityPredicateTightness(dataElt, index);
    } else if (MatchExpression::REGEX == expr->matchType()) {
        const RegexMatchExpression* rme = static_cast<const RegexMatchExpression*>(expr);
        translateRegex(rme, index, oilOut, tightnessOut);
    } else if (MatchExpression::MOD == expr->matchType()) {
        BSONObjBuilder bob;
        bob.appendMinForType("", NumberDouble);
        bob.appendMaxForType("", NumberDouble);
        BSONObj dataObj = bob.obj();
        verify(dataObj.isOwned());
        oilOut->intervals.push_back(
            makeRangeInterval(dataObj, BoundInclusion::kIncludeBothStartAndEndKeys));
        *tightnessOut = IndexBoundsBuilder::INEXACT_COVERED;
    } else if (MatchExpression::TYPE_OPERATOR == expr->matchType()) {
        const TypeMatchExpression* tme = static_cast<const TypeMatchExpression*>(expr);

        if (tme->typeSet().hasType(BSONType::Array)) {
            // We have $type:"array". Since arrays are indexed by creating a key for each element,
            // we have to fetch all indexed documents and check whether the full document contains
            // an array.
            oilOut->intervals.push_back(allValues());
            *tightnessOut = IndexBoundsBuilder::INEXACT_FETCH;
            return;
        }

        // If we are matching all numbers, we just use the bounds for NumberInt, as these bounds
        // also include all NumberDouble and NumberLong values.
        if (tme->typeSet().allNumbers) {
            BSONObjBuilder bob;
            bob.appendMinForType("", BSONType::NumberInt);
            bob.appendMaxForType("", BSONType::NumberInt);
            oilOut->intervals.push_back(
                makeRangeInterval(bob.obj(), BoundInclusion::kIncludeBothStartAndEndKeys));
        }

        for (auto type : tme->typeSet().bsonTypes) {
            BSONObjBuilder bob;
            bob.appendMinForType("", type);
            bob.appendMaxForType("", type);
            oilOut->intervals.push_back(
                makeRangeInterval(bob.obj(), BoundInclusion::kIncludeBothStartAndEndKeys));
        }

        // If we're only matching the "number" type, then the bounds are exact. Otherwise, the
        // bounds may be inexact.
        *tightnessOut = (tme->typeSet().isSingleType() && tme->typeSet().allNumbers)
            ? IndexBoundsBuilder::EXACT
            : IndexBoundsBuilder::INEXACT_FETCH;

        // Sort the intervals, and merge redundant ones.
        unionize(oilOut);
    } else if (MatchExpression::MATCH_IN == expr->matchType()) {
        const InMatchExpression* ime = static_cast<const InMatchExpression*>(expr);

        *tightnessOut = IndexBoundsBuilder::EXACT;

        // Create our various intervals.

        IndexBoundsBuilder::BoundsTightness tightness;
        bool arrayOrNullPresent = false;
        for (auto&& equality : ime->getEqualities()) {
            translateEquality(equality, index, isHashed, oilOut, &tightness);
            // The ordering invariant of oil has been violated by the call to translateEquality.
            arrayOrNullPresent = arrayOrNullPresent || equality.type() == BSONType::jstNULL ||
                equality.type() == BSONType::Array;
            if (tightness != IndexBoundsBuilder::EXACT) {
                *tightnessOut = tightness;
            }
        }

        for (auto&& regex : ime->getRegexes()) {
            translateRegex(regex.get(), index, oilOut, &tightness);
            if (tightness != IndexBoundsBuilder::EXACT) {
                *tightnessOut = tightness;
            }
        }

        if (ime->hasNull()) {
            // A null index key does not always match a null query value so we must fetch the
            // doc and run a full comparison.  See SERVER-4529.
            // TODO: Do we already set the tightnessOut by calling translateEquality?
            *tightnessOut = INEXACT_FETCH;
        }

        if (ime->hasEmptyArray()) {
            // Empty arrays are indexed as undefined.
            BSONObjBuilder undefinedBob;
            undefinedBob.appendUndefined("");
            oilOut->intervals.push_back(makePointInterval(undefinedBob.obj()));
            *tightnessOut = IndexBoundsBuilder::INEXACT_FETCH;
        }

        // Equalities are already sorted and deduped so unionize is unneccesary if no regexes
        // are present. Hashed indexes may also cause the bounds to be out-of-order.
        // Arrays and nulls introduce multiple elements that neccesitate a sort and deduping.
        if (!ime->getRegexes().empty() || index.type == IndexType::INDEX_HASHED ||
            arrayOrNullPresent)
            unionize(oilOut);
    } else if (MatchExpression::GEO == expr->matchType()) {
        const GeoMatchExpression* gme = static_cast<const GeoMatchExpression*>(expr);
        if ("2dsphere" == elt.valueStringDataSafe()) {
            verify(gme->getGeoExpression().getGeometry().hasS2Region());
            const S2Region& region = gme->getGeoExpression().getGeometry().getS2Region();
            S2IndexingParams indexParams;
            ExpressionParams::initialize2dsphereParams(index.infoObj, index.collator, &indexParams);
            ExpressionMapping::cover2dsphere(region, indexParams, oilOut);
            *tightnessOut = IndexBoundsBuilder::INEXACT_FETCH;
        } else if ("2d" == elt.valueStringDataSafe()) {
            verify(gme->getGeoExpression().getGeometry().hasR2Region());
            const R2Region& region = gme->getGeoExpression().getGeometry().getR2Region();

            ExpressionMapping::cover2d(
                region, index.infoObj, gInternalGeoPredicateQuery2DMaxCoveringCells.load(), oilOut);

            *tightnessOut = IndexBoundsBuilder::INEXACT_FETCH;
        } else {
            warning() << "Planner error trying to build geo bounds for " << elt.toString()
                      << " index element.";
            verify(0);
        }
    } else {
        warning() << "Planner error, trying to build bounds for expression: "
                  << redact(expr->debugString());
        verify(0);
    }
}