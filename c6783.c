poppler_document_get_attachments (PopplerDocument *document)
{
  Catalog *catalog;
  int n_files, i;
  GList *retval = nullptr;

  g_return_val_if_fail (POPPLER_IS_DOCUMENT (document), NULL);

  catalog = document->doc->getCatalog ();
  if (catalog == nullptr || ! catalog->isOk ())
    return nullptr;

  n_files = catalog->numEmbeddedFiles ();
  for (i = 0; i < n_files; i++)
    {
      PopplerAttachment *attachment;
      FileSpec *emb_file;

      emb_file = catalog->embeddedFile (i);
      if (!emb_file->isOk () || !emb_file->getEmbeddedFile()->isOk()) {
        delete emb_file;
	continue;
      }

      attachment = _poppler_attachment_new (emb_file);
      delete emb_file;

      retval = g_list_prepend (retval, attachment);
    }
  return g_list_reverse (retval);
}