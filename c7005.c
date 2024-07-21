bool PDFDoc::setup(const GooString *ownerPassword, const GooString *userPassword) {
  pdfdocLocker();

  if (str->getLength() <= 0)
  {
    error(errSyntaxError, -1, "Document stream is empty");
    return false;
  }

  str->setPos(0, -1);
  if (str->getPos() < 0)
  {
    error(errSyntaxError, -1, "Document base stream is not seekable");
    return false;
  }

  str->reset();

  // check footer
  // Adobe does not seem to enforce %%EOF, so we do the same
//  if (!checkFooter()) return false;
  
  // check header
  checkHeader();

  bool wasReconstructed = false;

  // read xref table
  xref = new XRef(str, getStartXRef(), getMainXRefEntriesOffset(), &wasReconstructed);
  if (!xref->isOk()) {
    if (wasReconstructed) {
      delete xref;
      startXRefPos = -1;
      xref = new XRef(str, getStartXRef(true), getMainXRefEntriesOffset(true), &wasReconstructed);
    }
    if (!xref->isOk()) {
      error(errSyntaxError, -1, "Couldn't read xref table");
      errCode = xref->getErrorCode();
      return false;
    }
  }

  // check for encryption
  if (!checkEncryption(ownerPassword, userPassword)) {
    errCode = errEncrypted;
    return false;
  }

  // read catalog
  catalog = new Catalog(this);
  if (catalog && !catalog->isOk()) {
    if (!wasReconstructed)
    {
      // try one more time to construct the Catalog, maybe the problem is damaged XRef
      delete catalog;
      delete xref;
      xref = new XRef(str, 0, 0, nullptr, true);
      catalog = new Catalog(this);
    }

    if (catalog && !catalog->isOk()) {
      error(errSyntaxError, -1, "Couldn't read page catalog");
      errCode = errBadCatalog;
      return false;
    }
  }

  // Extract PDF Subtype information
  extractPDFSubtype();

  // done
  return xref->isOk();
}