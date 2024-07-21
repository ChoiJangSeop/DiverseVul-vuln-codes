Finder::check()
{
    QPDFTokenizer tokenizer;
    QPDFTokenizer::Token t = tokenizer.readToken(is, "finder", true);
    qpdf_offset_t offset = this->is->tell();
    bool result = (t == QPDFTokenizer::Token(QPDFTokenizer::tt_word, str));
    this->is->seek(offset - this->str.length(), SEEK_SET);
    return result;
}