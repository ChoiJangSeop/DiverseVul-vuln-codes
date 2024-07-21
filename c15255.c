QPDF::readObject(PointerHolder<InputSource> input,
		 std::string const& description,
		 int objid, int generation, bool in_object_stream)
{
    setLastObjectDescription(description, objid, generation);
    qpdf_offset_t offset = input->tell();

    bool empty = false;
    PointerHolder<StringDecrypter> decrypter_ph;
    StringDecrypter* decrypter = 0;
    if (this->m->encp->encrypted && (! in_object_stream))
    {
        decrypter_ph = new StringDecrypter(this, objid, generation);
        decrypter = decrypter_ph.getPointer();
    }
    QPDFObjectHandle object = QPDFObjectHandle::parse(
        input, this->m->last_object_description,
        this->m->tokenizer, empty, decrypter, this);
    if (empty)
    {
        // Nothing in the PDF spec appears to allow empty objects, but
        // they have been encountered in actual PDF files and Adobe
        // Reader appears to ignore them.
        warn(QPDFExc(qpdf_e_damaged_pdf, input->getName(),
                     this->m->last_object_description,
                     input->getLastOffset(),
                     "empty object treated as null"));
    }
    else if (object.isDictionary() && (! in_object_stream))
    {
        // check for stream
        qpdf_offset_t cur_offset = input->tell();
        if (readToken(input) ==
            QPDFTokenizer::Token(QPDFTokenizer::tt_word, "stream"))
        {
            // The PDF specification states that the word "stream"
            // should be followed by either a carriage return and
            // a newline or by a newline alone.  It specifically
            // disallowed following it by a carriage return alone
            // since, in that case, there would be no way to tell
            // whether the NL in a CR NL sequence was part of the
            // stream data.  However, some readers, including
            // Adobe reader, accept a carriage return by itself
            // when followed by a non-newline character, so that's
            // what we do here.
            {
                char ch;
                if (input->read(&ch, 1) == 0)
                {
                    // A premature EOF here will result in some
                    // other problem that will get reported at
                    // another time.
                }
                else if (ch == '\n')
                {
                    // ready to read stream data
                    QTC::TC("qpdf", "QPDF stream with NL only");
                }
                else if (ch == '\r')
                {
                    // Read another character
                    if (input->read(&ch, 1) != 0)
                    {
                        if (ch == '\n')
                        {
                            // Ready to read stream data
                            QTC::TC("qpdf", "QPDF stream with CRNL");
                        }
                        else
                        {
                            // Treat the \r by itself as the
                            // whitespace after endstream and
                            // start reading stream data in spite
                            // of not having seen a newline.
                            QTC::TC("qpdf", "QPDF stream with CR only");
                            input->unreadCh(ch);
                            warn(QPDFExc(
                                     qpdf_e_damaged_pdf,
                                     input->getName(),
                                     this->m->last_object_description,
                                     input->tell(),
                                     "stream keyword followed"
                                     " by carriage return only"));
                        }
                    }
                }
                else
                {
                    QTC::TC("qpdf", "QPDF stream without newline");
                    if (! QUtil::is_space(ch))
                    {
                        QTC::TC("qpdf", "QPDF stream with non-space");
                        input->unreadCh(ch);
                    }
                    warn(QPDFExc(qpdf_e_damaged_pdf, input->getName(),
                                 this->m->last_object_description,
                                 input->tell(),
                                 "stream keyword not followed"
                                 " by proper line terminator"));
                }
            }

            // Must get offset before accessing any additional
            // objects since resolving a previously unresolved
            // indirect object will change file position.
            qpdf_offset_t stream_offset = input->tell();
            size_t length = 0;

            try
            {
                std::map<std::string, QPDFObjectHandle> dict =
                    object.getDictAsMap();

                if (dict.count("/Length") == 0)
                {
                    QTC::TC("qpdf", "QPDF stream without length");
                    throw QPDFExc(qpdf_e_damaged_pdf, input->getName(),
                                  this->m->last_object_description, offset,
                                  "stream dictionary lacks /Length key");
                }

                QPDFObjectHandle length_obj = dict["/Length"];
                if (! length_obj.isInteger())
                {
                    QTC::TC("qpdf", "QPDF stream length not integer");
                    throw QPDFExc(qpdf_e_damaged_pdf, input->getName(),
                                  this->m->last_object_description, offset,
                                  "/Length key in stream dictionary is not "
                                  "an integer");
                }

                length = length_obj.getIntValue();
                input->seek(stream_offset + length, SEEK_SET);
                if (! (readToken(input) ==
                       QPDFTokenizer::Token(
                           QPDFTokenizer::tt_word, "endstream")))
                {
                    QTC::TC("qpdf", "QPDF missing endstream");
                    throw QPDFExc(qpdf_e_damaged_pdf, input->getName(),
                                  this->m->last_object_description,
                                  input->getLastOffset(),
                                  "expected endstream");
                }
            }
            catch (QPDFExc& e)
            {
                if (this->m->attempt_recovery)
                {
                    warn(e);
                    length = recoverStreamLength(
                        input, objid, generation, stream_offset);
                }
                else
                {
                    throw e;
                }
            }
            object = QPDFObjectHandle::Factory::newStream(
                this, objid, generation, object, stream_offset, length);
        }
        else
        {
            input->seek(cur_offset, SEEK_SET);
        }
    }

    // Override last_offset so that it points to the beginning of the
    // object we just read
    input->setLastOffset(offset);
    return object;
}