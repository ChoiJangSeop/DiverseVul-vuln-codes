QPDFFormFieldObjectHelper::getQuadding()
{
    int result = 0;
    QPDFObjectHandle fv = getInheritableFieldValue("/Q");
    if (fv.isInteger())
    {
        QTC::TC("qpdf", "QPDFFormFieldObjectHelper Q present");
        result = static_cast<int>(fv.getIntValue());
    }
    return result;
}