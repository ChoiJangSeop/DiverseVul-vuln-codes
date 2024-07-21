QPDF::insertPage(QPDFObjectHandle newpage, int pos)
{
    // pos is numbered from 0, so pos = 0 inserts at the beginning and
    // pos = npages adds to the end.

    flattenPagesTree();

    if (! newpage.isIndirect())
    {
        QTC::TC("qpdf", "QPDF insert non-indirect page");
        newpage = makeIndirectObject(newpage);
    }
    else if (newpage.getOwningQPDF() != this)
    {
        QTC::TC("qpdf", "QPDF insert foreign page");
        newpage.getOwningQPDF()->pushInheritedAttributesToPage();
        newpage = copyForeignObject(newpage);
    }
    else
    {
        QTC::TC("qpdf", "QPDF insert indirect page");
    }

    QTC::TC("qpdf", "QPDF insert page",
            (pos == 0) ? 0 :                      // insert at beginning
            (pos == static_cast<int>(this->m->all_pages.size())) ? 1 : // at end
            2);                                   // insert in middle

    QPDFObjectHandle pages = getRoot().getKey("/Pages");
    QPDFObjectHandle kids = pages.getKey("/Kids");
    assert ((pos >= 0) &&
            (static_cast<size_t>(pos) <= this->m->all_pages.size()));

    newpage.replaceKey("/Parent", pages);
    kids.insertItem(pos, newpage);
    int npages = kids.getArrayNItems();
    pages.replaceKey("/Count", QPDFObjectHandle::newInteger(npages));
    this->m->all_pages.insert(this->m->all_pages.begin() + pos, newpage);
    assert(this->m->all_pages.size() == static_cast<size_t>(npages));
    for (int i = pos + 1; i < npages; ++i)
    {
        insertPageobjToPage(this->m->all_pages.at(i), i, false);
    }
    insertPageobjToPage(newpage, pos, true);
    assert(this->m->pageobj_to_pages_pos.size() == static_cast<size_t>(npages));
}