QPDFObjectHandle::parseContentStream_data(
    PointerHolder<Buffer> stream_data,
    std::string const& description,
    ParserCallbacks* callbacks)
{
    size_t length = stream_data->getSize();
    PointerHolder<InputSource> input =
        new BufferInputSource(description, stream_data.getPointer());
    QPDFTokenizer tokenizer;
    tokenizer.allowEOF();
    bool empty = false;
    while (static_cast<size_t>(input->tell()) < length)
    {
        QPDFObjectHandle obj =
            parseInternal(input, "content", tokenizer, empty, 0, 0, true);
        if (! obj.isInitialized())
        {
            // EOF
            break;
        }

        callbacks->handleObject(obj);
        if (obj.isOperator() && (obj.getOperatorValue() == "ID"))
        {
            // Discard next character; it is the space after ID that
            // terminated the token.  Read until end of inline image.
            char ch;
            input->read(&ch, 1);
            tokenizer.expectInlineImage(input);
            QPDFTokenizer::Token t =
                tokenizer.readToken(input, description, true);
            if (t.getType() == QPDFTokenizer::tt_bad)
            {
                QTC::TC("qpdf", "QPDFObjectHandle EOF in inline image");
                throw QPDFExc(qpdf_e_damaged_pdf, input->getName(),
                              "stream data", input->tell(),
                              "EOF found while reading inline image");
            }
            else
            {
                std::string inline_image = t.getValue();
                QTC::TC("qpdf", "QPDFObjectHandle inline image token");
                callbacks->handleObject(
                    QPDFObjectHandle::newInlineImage(inline_image));
            }
        }
    }
}