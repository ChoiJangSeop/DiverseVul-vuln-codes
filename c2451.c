keybox_compress (KEYBOX_HANDLE hd)
{
  int read_rc, rc;
  const char *fname;
  FILE *fp, *newfp;
  char *bakfname = NULL;
  char *tmpfname = NULL;
  int first_blob;
  KEYBOXBLOB blob = NULL;
  u32 cut_time;
  int any_changes = 0;
  int skipped_deleted;

  if (!hd)
    return gpg_error (GPG_ERR_INV_HANDLE);
  if (!hd->kb)
    return gpg_error (GPG_ERR_INV_HANDLE);
  if (hd->secret)
    return gpg_error (GPG_ERR_NOT_IMPLEMENTED);
  fname = hd->kb->fname;
  if (!fname)
    return gpg_error (GPG_ERR_INV_HANDLE);

  _keybox_close_file (hd);

  /* Open the source file. Because we do a rename, we have to check the
     permissions of the file */
  if (access (fname, W_OK))
    return gpg_error_from_syserror ();

  fp = fopen (fname, "rb");
  if (!fp && errno == ENOENT)
    return 0; /* Ready. File has been deleted right after the access above. */
  if (!fp)
    {
      rc = gpg_error_from_syserror ();
      return rc;
    }

  /* A quick test to see if we need to compress the file at all.  We
     schedule a compress run after 3 hours. */
  if ( !_keybox_read_blob (&blob, fp) )
    {
      const unsigned char *buffer;
      size_t length;

      buffer = _keybox_get_blob_image (blob, &length);
      if (length > 4 && buffer[4] == KEYBOX_BLOBTYPE_HEADER)
        {
          u32 last_maint = ((buffer[20] << 24) | (buffer[20+1] << 16)
                            | (buffer[20+2] << 8) | (buffer[20+3]));

          if ( (last_maint + 3*3600) > time (NULL) )
            {
              fclose (fp);
              _keybox_release_blob (blob);
              return 0; /* Compress run not yet needed. */
            }
        }
      _keybox_release_blob (blob);
      fseek (fp, 0, SEEK_SET);
      clearerr (fp);
    }

  /* Create the new file. */
  rc = create_tmp_file (fname, &bakfname, &tmpfname, &newfp);
  if (rc)
    {
      fclose (fp);
      return rc;;
    }


  /* Processing loop.  By reading using _keybox_read_blob we
     automagically skip any blobs flagged as deleted.  Thus what we
     only have to do is to check all ephemeral flagged blocks whether
     their time has come and write out all other blobs. */
  cut_time = time(NULL) - 86400;
  first_blob = 1;
  skipped_deleted = 0;
  for (rc=0; !(read_rc = _keybox_read_blob2 (&blob, fp, &skipped_deleted));
       _keybox_release_blob (blob), blob = NULL )
    {
      unsigned int blobflags;
      const unsigned char *buffer;
      size_t length, pos, size;
      u32 created_at;

      if (skipped_deleted)
        any_changes = 1;
      buffer = _keybox_get_blob_image (blob, &length);
      if (first_blob)
        {
          first_blob = 0;
          if (length > 4 && buffer[4] == KEYBOX_BLOBTYPE_HEADER)
            {
              /* Write out the blob with an updated maintenance time
                 stamp and if needed (ie. used by gpg) set the openpgp
                 flag.  */
              _keybox_update_header_blob (blob, hd->for_openpgp);
              rc = _keybox_write_blob (blob, newfp);
              if (rc)
                break;
              continue;
            }

          /* The header blob is missing.  Insert it.  */
          rc = _keybox_write_header_blob (newfp, hd->for_openpgp);
          if (rc)
            break;
          any_changes = 1;
        }
      else if (length > 4 && buffer[4] == KEYBOX_BLOBTYPE_HEADER)
        {
          /* Oops: There is another header record - remove it. */
          any_changes = 1;
          continue;
        }

      if (_keybox_get_flag_location (buffer, length,
                                     KEYBOX_FLAG_BLOB, &pos, &size)
          || size != 2)
        {
          rc = gpg_error (GPG_ERR_BUG);
          break;
        }
      blobflags = ((buffer[pos] << 8) | (buffer[pos+1]));
      if ((blobflags & KEYBOX_FLAG_BLOB_EPHEMERAL))
        {
          /* This is an ephemeral blob. */
          if (_keybox_get_flag_location (buffer, length,
                                         KEYBOX_FLAG_CREATED_AT, &pos, &size)
              || size != 4)
            created_at = 0; /* oops. */
          else
            created_at = ((buffer[pos] << 24) | (buffer[pos+1] << 16)
                          | (buffer[pos+2] << 8) | (buffer[pos+3]));

          if (created_at && created_at < cut_time)
            {
              any_changes = 1;
              continue; /* Skip this blob. */
            }
        }

      rc = _keybox_write_blob (blob, newfp);
      if (rc)
        break;
    }
  if (skipped_deleted)
    any_changes = 1;
  _keybox_release_blob (blob); blob = NULL;
  if (!rc && read_rc == -1)
    rc = 0;
  else if (!rc)
    rc = read_rc;

  /* Close both files. */
  if (fclose(fp) && !rc)
    rc = gpg_error_from_syserror ();
  if (fclose(newfp) && !rc)
    rc = gpg_error_from_syserror ();

  /* Rename or remove the temporary file. */
  if (rc || !any_changes)
    gnupg_remove (tmpfname);
  else
    rc = rename_tmp_file (bakfname, tmpfname, fname, hd->secret);

  xfree(bakfname);
  xfree(tmpfname);
  return rc;
}