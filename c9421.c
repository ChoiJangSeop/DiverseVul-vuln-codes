StatusWith<QueryMetadataBitSet> CanonicalQuery::isValid(MatchExpression* root,
                                                        const QueryRequest& request) {
    QueryMetadataBitSet unavailableMetadata{};

    // There can only be one TEXT.  If there is a TEXT, it cannot appear inside a NOR.
    //
    // Note that the query grammar (as enforced by the MatchExpression parser) forbids TEXT
    // inside of value-expression clauses like NOT, so we don't check those here.
    size_t numText = countNodes(root, MatchExpression::TEXT);
    if (numText > 1) {
        return Status(ErrorCodes::BadValue, "Too many text expressions");
    } else if (1 == numText) {
        if (hasNodeInSubtree(root, MatchExpression::TEXT, MatchExpression::NOR)) {
            return Status(ErrorCodes::BadValue, "text expression not allowed in nor");
        }
    } else {
        // Text metadata is not available.
        unavailableMetadata.set(DocumentMetadataFields::kTextScore);
    }

    // There can only be one NEAR.  If there is a NEAR, it must be either the root or the root
    // must be an AND and its child must be a NEAR.
    size_t numGeoNear = countNodes(root, MatchExpression::GEO_NEAR);
    if (numGeoNear > 1) {
        return Status(ErrorCodes::BadValue, "Too many geoNear expressions");
    } else if (1 == numGeoNear) {
        bool topLevel = false;
        if (MatchExpression::GEO_NEAR == root->matchType()) {
            topLevel = true;
        } else if (MatchExpression::AND == root->matchType()) {
            for (size_t i = 0; i < root->numChildren(); ++i) {
                if (MatchExpression::GEO_NEAR == root->getChild(i)->matchType()) {
                    topLevel = true;
                    break;
                }
            }
        }
        if (!topLevel) {
            return Status(ErrorCodes::BadValue, "geoNear must be top-level expr");
        }
    } else {
        // Geo distance and geo point metadata are unavailable.
        unavailableMetadata |= DepsTracker::kAllGeoNearData;
    }

    const BSONObj& sortObj = request.getSort();
    BSONElement sortNaturalElt = sortObj["$natural"];
    const BSONObj& hintObj = request.getHint();
    BSONElement hintNaturalElt = hintObj["$natural"];

    if (sortNaturalElt && sortObj.nFields() != 1) {
        return Status(ErrorCodes::BadValue,
                      str::stream() << "Cannot include '$natural' in compound sort: " << sortObj);
    }

    if (hintNaturalElt && hintObj.nFields() != 1) {
        return Status(ErrorCodes::BadValue,
                      str::stream() << "Cannot include '$natural' in compound hint: " << hintObj);
    }

    // NEAR cannot have a $natural sort or $natural hint.
    if (numGeoNear > 0) {
        if (sortNaturalElt) {
            return Status(ErrorCodes::BadValue,
                          "geoNear expression not allowed with $natural sort order");
        }

        if (hintNaturalElt) {
            return Status(ErrorCodes::BadValue,
                          "geoNear expression not allowed with $natural hint");
        }
    }

    // TEXT and NEAR cannot both be in the query.
    if (numText > 0 && numGeoNear > 0) {
        return Status(ErrorCodes::BadValue, "text and geoNear not allowed in same query");
    }

    // TEXT and {$natural: ...} sort order cannot both be in the query.
    if (numText > 0 && sortNaturalElt) {
        return Status(ErrorCodes::BadValue, "text expression not allowed with $natural sort order");
    }

    // TEXT and hint cannot both be in the query.
    if (numText > 0 && !hintObj.isEmpty()) {
        return Status(ErrorCodes::BadValue, "text and hint not allowed in same query");
    }

    // TEXT and tailable are incompatible.
    if (numText > 0 && request.isTailable()) {
        return Status(ErrorCodes::BadValue, "text and tailable cursor not allowed in same query");
    }

    // $natural sort order must agree with hint.
    if (sortNaturalElt) {
        if (!hintObj.isEmpty() && !hintNaturalElt) {
            return Status(ErrorCodes::BadValue, "index hint not allowed with $natural sort order");
        }
        if (hintNaturalElt) {
            if (hintNaturalElt.numberInt() != sortNaturalElt.numberInt()) {
                return Status(ErrorCodes::BadValue,
                              "$natural hint must be in the same direction as $natural sort order");
            }
        }
    }

    return unavailableMetadata;
}