QPDFAcroFormDocumentHelper::traverseField(
    QPDFObjectHandle field, QPDFObjectHandle parent, int depth,
    std::set<QPDFObjGen>& visited)
{
    if (depth > 100)
    {
        // Arbitrarily cut off recursion at a fixed depth to avoid
        // specially crafted files that could cause stack overflow.
        return;
    }
    if (! field.isIndirect())
    {
        QTC::TC("qpdf", "QPDFAcroFormDocumentHelper direct field");
        field.warnIfPossible(
            "encountered a direct object as a field or annotation while"
            " traversing /AcroForm; ignoring field or annotation");
        return;
    }
    if (! field.isDictionary())
    {
        QTC::TC("qpdf", "QPDFAcroFormDocumentHelper non-dictionary field");
        field.warnIfPossible(
            "encountered a non-dictionary as a field or annotation while"
            " traversing /AcroForm; ignoring field or annotation");
        return;
    }
    QPDFObjGen og(field.getObjGen());
    if (visited.count(og) != 0)
    {
        QTC::TC("qpdf", "QPDFAcroFormDocumentHelper loop");
        field.warnIfPossible("loop detected while traversing /AcroForm");
        return;
    }
    visited.insert(og);

    // A dictionary encountered while traversing the /AcroForm field
    // may be a form field, an annotation, or the merger of the two. A
    // field that has no fields below it is a terminal. If a terminal
    // field looks like an annotation, it is an annotation because
    // annotation dictionary fields can be merged with terminal field
    // dictionaries. Otherwise, the annotation fields might be there
    // to be inherited by annotations below it.

    bool is_annotation = false;
    bool is_field = (0 == depth);
    QPDFObjectHandle kids = field.getKey("/Kids");
    if (kids.isArray())
    {
        is_field = true;
        size_t nkids = kids.getArrayNItems();
        for (size_t k = 0; k < nkids; ++k)
        {
            traverseField(kids.getArrayItem(k), field, 1 + depth, visited);
        }
    }
    else
    {
        if (field.hasKey("/Parent"))
        {
            is_field = true;
        }
        if (field.hasKey("/Subtype") ||
            field.hasKey("/Rect") ||
            field.hasKey("/AP"))
        {
            is_annotation = true;
        }
    }

    QTC::TC("qpdf", "QPDFAcroFormDocumentHelper field found",
            (depth == 0) ? 0 : 1);
    QTC::TC("qpdf", "QPDFAcroFormDocumentHelper annotation found",
            (is_field ? 0 : 1));

    if (is_annotation)
    {
        QPDFObjectHandle our_field = (is_field ? field : parent);
        this->m->field_to_annotations[our_field.getObjGen()].push_back(
            QPDFAnnotationObjectHelper(field));
        this->m->annotation_to_field[og] =
            QPDFFormFieldObjectHelper(our_field);
    }
}