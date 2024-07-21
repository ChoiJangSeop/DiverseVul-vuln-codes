void DocumentSourceMatch::joinMatchWith(intrusive_ptr<DocumentSourceMatch> other) {
    rebuild(BSON("$and" << BSON_ARRAY(_predicate << other->getQuery())));
}