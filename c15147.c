QPDFWriter::writeHintStream(int hint_id)
{
    PointerHolder<Buffer> hint_buffer;
    int S = 0;
    int O = 0;
    QPDF::Writer::generateHintStream(
        this->m->pdf, this->m->xref, this->m->lengths,
        this->m->obj_renumber_no_gen,
        hint_buffer, S, O);

    openObject(hint_id);
    setDataKey(hint_id);

    size_t hlen = hint_buffer->getSize();

    writeString("<< /Filter /FlateDecode /S ");
    writeString(QUtil::int_to_string(S));
    if (O)
    {
	writeString(" /O ");
	writeString(QUtil::int_to_string(O));
    }
    writeString(" /Length ");
    adjustAESStreamLength(hlen);
    writeString(QUtil::int_to_string(hlen));
    writeString(" >>\nstream\n");

    if (this->m->encrypted)
    {
	QTC::TC("qpdf", "QPDFWriter encrypted hint stream");
    }
    pushEncryptionFilter();
    writeBuffer(hint_buffer);
    char last_char = this->m->pipeline->getLastChar();
    popPipelineStack();

    if (last_char != '\n')
    {
	writeString("\n");
    }
    writeString("endstream");
    closeObject(hint_id);
}