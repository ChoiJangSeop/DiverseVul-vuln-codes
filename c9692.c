bool isEqualityOrInNull(MatchExpression* me) {
    if (MatchExpression::EQ == me->matchType()) {
        return static_cast<ComparisonMatchExpression*>(me)->getData().type() == BSONType::jstNULL;
    }

    if (me->matchType() == MatchExpression::MATCH_IN) {
        const InMatchExpression* in = static_cast<const InMatchExpression*>(me);
        return in->hasNull();
    }

    return false;
}