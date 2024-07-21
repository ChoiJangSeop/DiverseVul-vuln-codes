IndexBoundsBuilder::BoundsTightness getInequalityPredicateTightness(const BSONElement& dataElt,
                                                                    const IndexEntry& index) {
    return Indexability::isExactBoundsGenerating(dataElt) ? IndexBoundsBuilder::EXACT
                                                          : IndexBoundsBuilder::INEXACT_FETCH;
}