QPDFPageDocumentHelper::flattenAnnotationsForPage(
    QPDFPageObjectHelper& page,
    QPDFObjectHandle& resources,
    QPDFAcroFormDocumentHelper& afdh,
    int required_flags,
    int forbidden_flags)
{
    bool need_appearances = afdh.getNeedAppearances();
    std::vector<QPDFAnnotationObjectHelper> annots = page.getAnnotations();
    std::vector<QPDFObjectHandle> new_annots;
    std::string new_content;
    int rotate = 0;
    QPDFObjectHandle rotate_obj =
        page.getObjectHandle().getKey("/Rotate");
    if (rotate_obj.isInteger() && rotate_obj.getIntValue())
    {
        rotate = rotate_obj.getIntValue();
    }
    int next_fx = 1;
    for (std::vector<QPDFAnnotationObjectHelper>::iterator iter =
             annots.begin();
         iter != annots.end(); ++iter)
    {
        QPDFAnnotationObjectHelper& aoh(*iter);
        QPDFObjectHandle as = aoh.getAppearanceStream("/N");
        bool is_widget = (aoh.getSubtype() == "/Widget");
        bool process = true;
        if (need_appearances && is_widget)
        {
            QTC::TC("qpdf", "QPDFPageDocumentHelper skip widget need appearances");
            process = false;
        }
        if (process && as.isStream())
        {
            if (is_widget)
            {
                QTC::TC("qpdf", "QPDFPageDocumentHelper merge DR");
                QPDFFormFieldObjectHelper ff = afdh.getFieldForAnnotation(aoh);
                QPDFObjectHandle as_resources =
                    as.getDict().getKey("/Resources");
                if (as_resources.isIndirect())
                {
                    QTC::TC("qpdf", "QPDFPageDocumentHelper indirect as resources");
                    as.getDict().replaceKey(
                        "/Resources", as_resources.shallowCopy());
                    as_resources = as.getDict().getKey("/Resources");
                }
                as_resources.mergeResources(
                    ff.getInheritableFieldValue("/DR"));
            }
            else
            {
                QTC::TC("qpdf", "QPDFPageDocumentHelper non-widget annotation");
            }
            std::string name = resources.getUniqueResourceName(
                "/Fxo", next_fx);
            std::string content = aoh.getPageContentForAppearance(
                name, rotate, required_flags, forbidden_flags);
            if (! content.empty())
            {
                resources.mergeResources(
                    QPDFObjectHandle::parse("<< /XObject << >> >>"));
                resources.getKey("/XObject").replaceKey(name, as);
                ++next_fx;
            }
            new_content += content;
        }
        else if (process)
        {
            // If an annotation has no appearance stream, just drop
            // the annotation when flattening. This can happen for
            // unchecked checkboxes and radio buttons, popup windows
            // associated with comments that aren't visible, and other
            // types of annotations that aren't visible.
            QTC::TC("qpdf", "QPDFPageDocumentHelper ignore annotation with no appearance");
        }
        else
        {
            new_annots.push_back(aoh.getObjectHandle());
        }
    }
    if (new_annots.size() != annots.size())
    {
        QPDFObjectHandle page_oh = page.getObjectHandle();
        if (new_annots.empty())
        {
            QTC::TC("qpdf", "QPDFPageDocumentHelper remove annots");
            page_oh.removeKey("/Annots");
        }
        else
        {
            QPDFObjectHandle old_annots = page_oh.getKey("/Annots");
            QPDFObjectHandle new_annots_oh =
                QPDFObjectHandle::newArray(new_annots);
            if (old_annots.isIndirect())
            {
                QTC::TC("qpdf", "QPDFPageDocumentHelper replace indirect annots");
                this->qpdf.replaceObject(
                    old_annots.getObjGen(), new_annots_oh);
            }
            else
            {
                QTC::TC("qpdf", "QPDFPageDocumentHelper replace direct annots");
                page_oh.replaceKey("/Annots", new_annots_oh);
            }
        }
        page.addPageContents(
            QPDFObjectHandle::newStream(&qpdf, "q\n"), true);
        page.addPageContents(
            QPDFObjectHandle::newStream(&qpdf, "\nQ\n" + new_content), false);
    }
}