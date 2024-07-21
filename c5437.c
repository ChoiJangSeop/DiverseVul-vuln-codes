QPDFObjectHandle::parse(PointerHolder<InputSource> input,
                        std::string const& object_description,
                        QPDFTokenizer& tokenizer, bool& empty,
                        StringDecrypter* decrypter, QPDF* context)
{
    return parseInternal(input, object_description, tokenizer, empty,
                         decrypter, context, false, false, false);
}