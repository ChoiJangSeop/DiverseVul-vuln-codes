InlineImageTracker::handleToken(QPDFTokenizer::Token const& token)
{
    if (state == st_bi)
    {
        if (token.getType() == QPDFTokenizer::tt_inline_image)
        {
            std::string image_data(token.getValue());
            size_t len = image_data.length();
            if (len >= this->min_size)
            {
                QTC::TC("qpdf", "QPDFPageObjectHelper externalize inline image");
                Pl_Buffer b("image_data");
                b.write(QUtil::unsigned_char_pointer(image_data), len);
                b.finish();
                QPDFObjectHandle dict =
                    convertIIDict(QPDFObjectHandle::parse(dict_str));
                dict.replaceKey("/Length", QPDFObjectHandle::newInteger(len));
                std::string name = resources.getUniqueResourceName(
                    "/IIm", this->min_suffix);
                QPDFObjectHandle image = QPDFObjectHandle::newStream(
                    this->qpdf, b.getBuffer());
                image.replaceDict(dict);
                resources.getKey("/XObject").replaceKey(name, image);
                write(name);
                write(" Do\n");
                any_images = true;
            }
            else
            {
                QTC::TC("qpdf", "QPDFPageObjectHelper keep inline image");
                write(bi_str);
                writeToken(token);
                state = st_top;
            }
        }
        else if (token == QPDFTokenizer::Token(QPDFTokenizer::tt_word, "ID"))
        {
            bi_str += token.getValue();
            dict_str += " >>";
        }
        else if (token == QPDFTokenizer::Token(QPDFTokenizer::tt_word, "EI"))
        {
            state = st_top;
        }
        else
        {
            bi_str += token.getValue();
            dict_str += token.getValue();
        }
    }
    else if (token == QPDFTokenizer::Token(QPDFTokenizer::tt_word, "BI"))
    {
        bi_str = token.getValue();
        dict_str = "<< ";
        state = st_bi;
    }
    else
    {
        writeToken(token);
    }
}