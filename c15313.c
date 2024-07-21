QPDF::addPage(QPDFObjectHandle newpage, bool first)
{
    if (first)
    {
        insertPage(newpage, 0);
    }
    else
    {
        insertPage(newpage,
                   getRoot().getKey("/Pages").getKey("/Count").getIntValue());
    }
}