QPDFAcroFormDocumentHelper::analyze()
{
    if (this->m->cache_valid)
    {
        return;
    }
    this->m->cache_valid = true;
    QPDFObjectHandle acroform = this->qpdf.getRoot().getKey("/AcroForm");
    if (! (acroform.isDictionary() && acroform.hasKey("/Fields")))
    {
        return;
    }
    QPDFObjectHandle fields = acroform.getKey("/Fields");
    if (! fields.isArray())
    {
        QTC::TC("qpdf", "QPDFAcroFormDocumentHelper fields not array");
        acroform.warnIfPossible(
            "/Fields key of /AcroForm dictionary is not an array; ignoring");
        fields = QPDFObjectHandle::newArray();
    }

    // Traverse /AcroForm to find annotations and map them
    // bidirectionally to fields.

    std::set<QPDFObjGen> visited;
    size_t nfields = fields.getArrayNItems();
    QPDFObjectHandle null(QPDFObjectHandle::newNull());
    for (size_t i = 0; i < nfields; ++i)
    {
        traverseField(fields.getArrayItem(i), null, 0, visited);
    }

    // All Widget annotations should have been encountered by
    // traversing /AcroForm, but in case any weren't, find them by
    // walking through pages, and treat any widget annotation that is
    // not associated with a field as its own field. This just ensures
    // that requesting the field for any annotation we find through a
    // page's /Annots list will have some associated field. Note that
    // a file that contains this kind of error will probably not
    // actually work with most viewers.

    QPDFPageDocumentHelper dh(this->qpdf);
    std::vector<QPDFPageObjectHelper> pages = dh.getAllPages();
    for (std::vector<QPDFPageObjectHelper>::iterator iter = pages.begin();
         iter != pages.end(); ++iter)
    {
        QPDFPageObjectHelper ph(*iter);
        std::vector<QPDFAnnotationObjectHelper> annots =
            getWidgetAnnotationsForPage(ph);
        for (std::vector<QPDFAnnotationObjectHelper>::iterator i2 =
                 annots.begin();
             i2 != annots.end(); ++i2)
        {
            QPDFObjectHandle annot((*i2).getObjectHandle());
            QPDFObjGen og(annot.getObjGen());
            if (this->m->annotation_to_field.count(og) == 0)
            {
                QTC::TC("qpdf", "QPDFAcroFormDocumentHelper orphaned widget");
                // This is not supposed to happen, but it's easy
                // enough for us to handle this case. Treat the
                // annotation as its own field. This could allow qpdf
                // to sensibly handle a case such as a PDF creator
                // adding a self-contained annotation (merged with the
                // field dictionary) to the page's /Annots array and
                // forgetting to also put it in /AcroForm.
                annot.warnIfPossible(
                    "this widget annotation is not"
                    " reachable from /AcroForm in the document catalog");
                this->m->annotation_to_field[og] =
                    QPDFFormFieldObjectHelper(annot);
                this->m->field_to_annotations[og].push_back(
                    QPDFAnnotationObjectHelper(annot));
            }
        }
    }
}