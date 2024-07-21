QPDFWriter::pushEncryptionFilter()
{
    if (this->m->encrypted && (! this->m->cur_data_key.empty()))
    {
	Pipeline* p = 0;
	if (this->m->encrypt_use_aes)
	{
	    p = new Pl_AES_PDF(
		"aes stream encryption", this->m->pipeline, true,
		QUtil::unsigned_char_pointer(this->m->cur_data_key),
                this->m->cur_data_key.length());
	}
	else
	{
	    p = new Pl_RC4("rc4 stream encryption", this->m->pipeline,
			   QUtil::unsigned_char_pointer(this->m->cur_data_key),
			   this->m->cur_data_key.length());
	}
	pushPipeline(p);
    }
    // Must call this unconditionally so we can call popPipelineStack
    // to balance pushEncryptionFilter().
    activatePipelineStack();
}