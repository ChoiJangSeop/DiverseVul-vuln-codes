void getElemMatchOrPushdownDescendants(MatchExpression* node, std::vector<MatchExpression*>* out) {
    if (node->getTag() && node->getTag()->getType() == TagType::OrPushdownTag) {
        out->push_back(node);
    } else if (node->matchType() == MatchExpression::ELEM_MATCH_OBJECT ||
               node->matchType() == MatchExpression::AND) {
        for (size_t i = 0; i < node->numChildren(); ++i) {
            getElemMatchOrPushdownDescendants(node->getChild(i), out);
        }
    }
}