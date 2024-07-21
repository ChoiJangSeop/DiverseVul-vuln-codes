static void call_read_memory(qpdf_data qpdf)
{
    qpdf->qpdf->processMemoryFile(qpdf->filename, qpdf->buffer,
				  qpdf->size, qpdf->password);
}