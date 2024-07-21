QPDFWriter::write()
{
    doWriteSetup();

    // Set up progress reporting. We spent about equal amounts of time
    // preparing and writing one pass. To get a rough estimate of
    // progress, we track handling of indirect objects. For linearized
    // files, we write two passes. events_expected is an
    // approximation, but it's good enough for progress reporting,
    // which is mostly a guess anyway.
    this->m->events_expected = (
        this->m->pdf.getObjectCount() * (this->m->linearized ? 3 : 2));

    prepareFileForWrite();

    if (this->m->linearized)
    {
	writeLinearized();
    }
    else
    {
	writeStandard();
    }

    this->m->pipeline->finish();
    if (this->m->close_file)
    {
	fclose(this->m->file);
    }
    this->m->file = 0;
    if (this->m->buffer_pipeline)
    {
	this->m->output_buffer = this->m->buffer_pipeline->getBuffer();
	this->m->buffer_pipeline = 0;
    }
    indicateProgress(false, true);
}