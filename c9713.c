bool ModMatchExpression::matchesSingleElement(const BSONElement& e, MatchDetails* details) const {
    if (!e.isNumber())
        return false;
    return e.numberLong() % _divisor == _remainder;
}