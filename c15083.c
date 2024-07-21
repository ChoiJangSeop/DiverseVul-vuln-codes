QPDF::removePage(QPDFObjectHandle page)
{
    int pos = findPage(page); // also ensures flat /Pages
    QTC::TC("qpdf", "QPDF remove page",
            (pos == 0) ? 0 :                            // remove at beginning
            (pos == static_cast<int>(
                this->m->all_pages.size() - 1)) ? 1 :   // end
            2);                                         // remove in middle

    QPDFObjectHandle pages = getRoot().getKey("/Pages");
    QPDFObjectHandle kids = pages.getKey("/Kids");

    kids.eraseItem(pos);
    int npages = kids.getArrayNItems();
    pages.replaceKey("/Count", QPDFObjectHandle::newInteger(npages));
    this->m->all_pages.erase(this->m->all_pages.begin() + pos);
    assert(this->m->all_pages.size() == static_cast<size_t>(npages));
    this->m->pageobj_to_pages_pos.erase(page.getObjGen());
    assert(this->m->pageobj_to_pages_pos.size() == static_cast<size_t>(npages));
    for (int i = pos; i < npages; ++i)
    {
        insertPageobjToPage(this->m->all_pages.at(i), i, false);
    }
}