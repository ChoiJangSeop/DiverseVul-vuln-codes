QPDF::flattenPagesTree()
{
    // If not already done, flatten the /Pages structure and
    // initialize pageobj_to_pages_pos.

    if (! this->m->pageobj_to_pages_pos.empty())
    {
        return;
    }

    // Push inherited objects down to the /Page level.  As a side
    // effect this->m->all_pages will also be generated.
    pushInheritedAttributesToPage(true, true);

    QPDFObjectHandle pages = getRoot().getKey("/Pages");

    int const len = this->m->all_pages.size();
    for (int pos = 0; pos < len; ++pos)
    {
        // populate pageobj_to_pages_pos and fix parent pointer
        insertPageobjToPage(this->m->all_pages.at(pos), pos, true);
        this->m->all_pages.at(pos).replaceKey("/Parent", pages);
    }

    pages.replaceKey("/Kids", QPDFObjectHandle::newArray(this->m->all_pages));
    // /Count has not changed
    if (pages.getKey("/Count").getIntValue() != len)
    {
        throw std::logic_error("/Count is wrong after flattening pages tree");
    }
}