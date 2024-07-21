static QPDFObjectHandle create_page_contents(QPDF& pdf, int pageno)
{
    return QPDFObjectHandle::newStream(&pdf, generate_page_contents(pageno));
}