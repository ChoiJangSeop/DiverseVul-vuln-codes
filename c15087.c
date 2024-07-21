QPDFTokenizer::findEI(PointerHolder<InputSource> input)
{
    if (! input.getPointer())
    {
        return;
    }

    qpdf_offset_t last_offset = input->getLastOffset();
    qpdf_offset_t pos = input->tell();

    // Use QPDFWordTokenFinder to find EI surrounded by delimiters.
    // Then read the next several tokens or up to EOF. If we find any
    // suspicious-looking or tokens, this is probably still part of
    // the image data, so keep looking for EI. Stop at the first EI
    // that passes. If we get to the end without finding one, return
    // the last EI we found. Store the number of bytes expected in the
    // inline image including the EI and use that to break out of
    // inline image, falling back to the old method if needed.

    bool okay = false;
    bool first_try = true;
    while (! okay)
    {
        QPDFWordTokenFinder f(input, "EI");
        if (! input->findFirst("EI", input->tell(), 0, f))
        {
            break;
        }
        this->m->inline_image_bytes = input->tell() - pos - 2;

        QPDFTokenizer check;
        bool found_bad = false;
        // Look at the next 10 tokens or up to EOF. The next inline
        // image's image data would look like bad tokens, but there
        // will always be at least 10 tokens between one inline
        // image's EI and the next valid one's ID since width, height,
        // bits per pixel, and color space are all required as well as
        // a BI and ID. If we get 10 good tokens in a row or hit EOF,
        // we can be pretty sure we've found the actual EI.
        for (int i = 0; i < 10; ++i)
        {
            QPDFTokenizer::Token t =
                check.readToken(input, "checker", true);
            token_type_e type = t.getType();
            if (type == tt_eof)
            {
                okay = true;
            }
            else if (type == tt_bad)
            {
                found_bad = true;
            }
            else if (type == tt_word)
            {
                // The qpdf tokenizer lumps alphabetic and otherwise
                // uncategorized characters into "words". We recognize
                // strings of alphabetic characters as potential valid
                // operators for purposes of telling whether we're in
                // valid content or not. It's not perfect, but it
                // should work more reliably than what we used to do,
                // which was already good enough for the vast majority
                // of files.
                bool found_alpha = false;
                bool found_non_printable = false;
                bool found_other = false;
                std::string value = t.getValue();
                for (std::string::iterator iter = value.begin();
                     iter != value.end(); ++iter)
                {
                    char ch = *iter;
                    if (((ch >= 'a') && (ch <= 'z')) ||
                        ((ch >= 'A') && (ch <= 'Z')) ||
                        (ch == '*'))
                    {
                        // Treat '*' as alpha since there are valid
                        // PDF operators that contain * along with
                        // alphabetic characters.
                        found_alpha = true;
                    }
                    else if (((ch < 32) && (! isSpace(ch))) || (ch > 127))
                    {
                        found_non_printable = true;
                        break;
                    }
                    else
                    {
                        found_other = true;
                    }
                }
                if (found_non_printable || (found_alpha && found_other))
                {
                    found_bad = true;
                }
            }
            if (okay || found_bad)
            {
                break;
            }
        }
        if (! found_bad)
        {
            okay = true;
        }
        if (! okay)
        {
            first_try = false;
        }
    }
    if (okay && (! first_try))
    {
        QTC::TC("qpdf", "QPDFTokenizer found EI after more than one try");
    }

    input->seek(pos, SEEK_SET);
    input->setLastOffset(last_offset);
}