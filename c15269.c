void add_page(QPDFPageDocumentHelper& dh, QPDFObjectHandle font,
              std::string const& color_space,
              std::string const& filter)
{
    QPDF& pdf(dh.getQPDF());

    // Create a stream to encode our image. QPDFWriter will fill in
    // the length and will respect our filters based on stream data
    // mode. Since we are not specifying, QPDFWriter will compress
    // with /FlateDecode if we don't provide any other form of
    // compression.
    ImageProvider* p = new ImageProvider(color_space, filter);
    PointerHolder<QPDFObjectHandle::StreamDataProvider> provider(p);
    int width = p->getWidth();
    int height = p->getHeight();
    QPDFObjectHandle image = QPDFObjectHandle::newStream(&pdf);
    image.replaceDict(QPDFObjectHandle::parse(
                          "<<"
                          " /Type /XObject"
                          " /Subtype /Image"
                          " /BitsPerComponent 8"
                          ">>"));
    QPDFObjectHandle image_dict = image.getDict();
    image_dict.replaceKey("/ColorSpace", newName(color_space));
    image_dict.replaceKey("/Width", newInteger(width));
    image_dict.replaceKey("/Height", newInteger(height));

    // Provide the stream data.
    image.replaceStreamData(provider,
                            QPDFObjectHandle::parse(filter),
                            QPDFObjectHandle::newNull());

    // Create direct objects as needed by the page dictionary.
    QPDFObjectHandle procset = QPDFObjectHandle::parse(
        "[/PDF /Text /ImageC]");

    QPDFObjectHandle rfont = QPDFObjectHandle::newDictionary();
    rfont.replaceKey("/F1", font);

    QPDFObjectHandle xobject = QPDFObjectHandle::newDictionary();
    xobject.replaceKey("/Im1", image);

    QPDFObjectHandle resources = QPDFObjectHandle::newDictionary();
    resources.replaceKey("/ProcSet", procset);
    resources.replaceKey("/Font", rfont);
    resources.replaceKey("/XObject", xobject);

    QPDFObjectHandle mediabox = QPDFObjectHandle::newArray();
    mediabox.appendItem(newInteger(0));
    mediabox.appendItem(newInteger(0));
    mediabox.appendItem(newInteger(612));
    mediabox.appendItem(newInteger(392));

    // Create the page content stream
    QPDFObjectHandle contents = createPageContents(
        pdf, color_space + " with filter " + filter);

    // Create the page dictionary
    QPDFObjectHandle page = pdf.makeIndirectObject(
        QPDFObjectHandle::newDictionary());
    page.replaceKey("/Type", newName("/Page"));
    page.replaceKey("/MediaBox", mediabox);
    page.replaceKey("/Contents", contents);
    page.replaceKey("/Resources", resources);

    // Add the page to the PDF file
    dh.addPage(page, false);
}