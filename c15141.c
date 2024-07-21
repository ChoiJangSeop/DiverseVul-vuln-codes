static void create_pdf(char const* filename)
{
    QPDF pdf;

    pdf.emptyPDF();

    QPDFObjectHandle font = pdf.makeIndirectObject(
        QPDFObjectHandle::newDictionary());
    font.replaceKey("/Type", newName("/Font"));
    font.replaceKey("/Subtype", newName("/Type1"));
    font.replaceKey("/Name", newName("/F1"));
    font.replaceKey("/BaseFont", newName("/Helvetica"));
    font.replaceKey("/Encoding", newName("/WinAnsiEncoding"));

    QPDFObjectHandle procset =
        pdf.makeIndirectObject(QPDFObjectHandle::newArray());
    procset.appendItem(newName("/PDF"));
    procset.appendItem(newName("/Text"));
    procset.appendItem(newName("/ImageC"));

    QPDFObjectHandle rfont = QPDFObjectHandle::newDictionary();
    rfont.replaceKey("/F1", font);

    QPDFObjectHandle mediabox = QPDFObjectHandle::newArray();
    mediabox.appendItem(newInteger(0));
    mediabox.appendItem(newInteger(0));
    mediabox.appendItem(newInteger(612));
    mediabox.appendItem(newInteger(792));

    QPDFPageDocumentHelper dh(pdf);
    for (int pageno = 1; pageno <= npages; ++pageno)
    {
        QPDFObjectHandle image = QPDFObjectHandle::newStream(&pdf);
        QPDFObjectHandle image_dict = image.getDict();
        image_dict.replaceKey("/Type", newName("/XObject"));
        image_dict.replaceKey("/Subtype", newName("/Image"));
        image_dict.replaceKey("/ColorSpace", newName("/DeviceGray"));
        image_dict.replaceKey("/BitsPerComponent", newInteger(8));
        image_dict.replaceKey("/Width", newInteger(width));
        image_dict.replaceKey("/Height", newInteger(height));
        ImageProvider* p = new ImageProvider(pageno);
        PointerHolder<QPDFObjectHandle::StreamDataProvider> provider(p);
        image.replaceStreamData(provider,
                                QPDFObjectHandle::newNull(),
                                QPDFObjectHandle::newNull());

        QPDFObjectHandle xobject = QPDFObjectHandle::newDictionary();
        xobject.replaceKey("/Im1", image);

        QPDFObjectHandle resources = QPDFObjectHandle::newDictionary();
        resources.replaceKey("/ProcSet", procset);
        resources.replaceKey("/Font", rfont);
        resources.replaceKey("/XObject", xobject);

        QPDFObjectHandle contents = create_page_contents(pdf, pageno);

        QPDFObjectHandle page = pdf.makeIndirectObject(
            QPDFObjectHandle::newDictionary());
        page.replaceKey("/Type", newName("/Page"));
        page.replaceKey("/MediaBox", mediabox);
        page.replaceKey("/Contents", contents);
        page.replaceKey("/Resources", resources);

        dh.addPage(page, false);
    }

    QPDFWriter w(pdf, filename);
    w.setStaticID(true);    // for testing only
    w.setStreamDataMode(qpdf_s_preserve);
    w.setObjectStreamMode(qpdf_o_disable);
    w.write();
}