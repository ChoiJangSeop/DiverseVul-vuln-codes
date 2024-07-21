QPDFObjectHandle::parseContentStream_internal(PointerHolder<Buffer> stream_data,
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
            parseInternal(input, "content", tokenizer, empty,
                          0, 0, false, false, true);
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
            char buf[4];
            memset(buf, '\0', sizeof(buf));
            bool done = false;
            std::string inline_image;
            while (! done)
            {
                if (input->read(&ch, 1) == 0)
                {
                    QTC::TC("qpdf", "QPDFObjectHandle EOF in inline image");
                    throw QPDFExc(qpdf_e_damaged_pdf, input->getName(),
                                  "stream data", input->tell(),
                                  "EOF found while reading inline image");
                }
                inline_image += ch;
                memmove(buf, buf + 1, sizeof(buf) - 1);
                buf[sizeof(buf) - 1] = ch;
                if (strchr(" \t\n\v\f\r", buf[0]) &&
                    (buf[1] == 'E') &&
                    (buf[2] == 'I') &&
                    strchr(" \t\n\v\f\r", buf[3]))
                {
                    // We've found an EI operator.
                    done = true;
                    input->seek(-3, SEEK_CUR);
                    for (int i = 0; i < 4; ++i)
                    {
                        if (inline_image.length() > 0)
                        {
                            inline_image.erase(inline_image.length() - 1);
                        }
                    }
                }
            }
            QTC::TC("qpdf", "QPDFObjectHandle inline image token");
            callbacks->handleObject(
                QPDFObjectHandle::newInlineImage(inline_image));
        }
    }
}