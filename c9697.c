void resolveOrPushdowns(MatchExpression* tree) {
    if (tree->numChildren() == 0) {
        return;
    }
    if (MatchExpression::AND == tree->matchType()) {
        AndMatchExpression* andNode = static_cast<AndMatchExpression*>(tree);
        MatchExpression* indexedOr = getIndexedOr(andNode);

        for (size_t i = 0; i < andNode->numChildren(); ++i) {
            auto child = andNode->getChild(i);
            if (child->getTag() && child->getTag()->getType() == TagType::OrPushdownTag) {
                invariant(indexedOr);
                OrPushdownTag* orPushdownTag = static_cast<OrPushdownTag*>(child->getTag());
                auto destinations = orPushdownTag->releaseDestinations();
                auto indexTag = orPushdownTag->releaseIndexTag();
                child->setTag(nullptr);
                if (pushdownNode(child, indexedOr, std::move(destinations)) && !indexTag) {

                    // indexedOr can completely satisfy the predicate specified in child, so we can
                    // trim it. We could remove the child even if it had an index tag for this
                    // position, but that could make the index tagging of the tree wrong.
                    auto ownedChild = andNode->removeChild(i);

                    // We removed child i, so decrement the child index.
                    --i;
                } else {
                    child->setTag(indexTag.release());
                }
            } else if (child->matchType() == MatchExpression::NOT && child->getChild(0)->getTag() &&
                       child->getChild(0)->getTag()->getType() == TagType::OrPushdownTag) {
                invariant(indexedOr);
                OrPushdownTag* orPushdownTag =
                    static_cast<OrPushdownTag*>(child->getChild(0)->getTag());
                auto destinations = orPushdownTag->releaseDestinations();
                auto indexTag = orPushdownTag->releaseIndexTag();
                child->getChild(0)->setTag(nullptr);

                // Push down the NOT and its child.
                if (pushdownNode(child, indexedOr, std::move(destinations)) && !indexTag) {

                    // indexedOr can completely satisfy the predicate specified in child, so we can
                    // trim it. We could remove the child even if it had an index tag for this
                    // position, but that could make the index tagging of the tree wrong.
                    auto ownedChild = andNode->removeChild(i);

                    // We removed child i, so decrement the child index.
                    --i;
                } else {
                    child->getChild(0)->setTag(indexTag.release());
                }
            } else if (child->matchType() == MatchExpression::ELEM_MATCH_OBJECT) {

                // Push down all descendants of child with OrPushdownTags.
                std::vector<MatchExpression*> orPushdownDescendants;
                getElemMatchOrPushdownDescendants(child, &orPushdownDescendants);
                if (!orPushdownDescendants.empty()) {
                    invariant(indexedOr);
                }
                for (auto descendant : orPushdownDescendants) {
                    OrPushdownTag* orPushdownTag =
                        static_cast<OrPushdownTag*>(descendant->getTag());
                    auto destinations = orPushdownTag->releaseDestinations();
                    auto indexTag = orPushdownTag->releaseIndexTag();
                    descendant->setTag(nullptr);
                    pushdownNode(descendant, indexedOr, std::move(destinations));
                    descendant->setTag(indexTag.release());

                    // We cannot trim descendants of an $elemMatch object, since the filter must
                    // be applied in its entirety.
                }
            }
        }
    }
    for (size_t i = 0; i < tree->numChildren(); ++i) {
        resolveOrPushdowns(tree->getChild(i));
    }
}