void processTnef (TNEFStruct *tnef, const gchar *tmpdir) {
    variableLength *filename;
    variableLength *filedata;
    Attachment *p;
    gint RealAttachment;
    gint object;
    gchar ifilename[256];
    gint i, count;
    gint foundCal=0;

    FILE *fptr;

    /* First see if this requires special processing. */
    /* ie: it's a Contact Card, Task, or Meeting request (vCal/vCard) */
    if (tnef->messageClass[0] != 0)  {
        if (strcmp(tnef->messageClass, "IPM.Contact") == 0) {
            saveVCard (tnef, tmpdir);
        }
        if (strcmp(tnef->messageClass, "IPM.Task") == 0) {
            saveVTask (tnef, tmpdir);
        }
        if (strcmp(tnef->messageClass, "IPM.Appointment") == 0) {
            saveVCalendar (tnef, tmpdir);
            foundCal = 1;
        }
    }

    if ((filename = MAPIFindUserProp (&(tnef->MapiProperties),
                        PROP_TAG (PT_STRING8,0x24))) != MAPI_UNDEFINED) {
        if (strcmp(filename->data, "IPM.Appointment") == 0) {
             /* If it's "indicated" twice, we don't want to save 2 calendar entries. */
            if (foundCal == 0) {
                saveVCalendar (tnef, tmpdir);
            }
        }
    }

    if (strcmp(tnef->messageClass, "IPM.Microsoft Mail.Note") == 0) {
        if ((saveRTF == 1) && (tnef->subject.size > 0)) {
            /*  Description */
            if ((filename=MAPIFindProperty (&(tnef->MapiProperties),
					   PROP_TAG (PT_BINARY, PR_RTF_COMPRESSED)))
                    != MAPI_UNDEFINED) {
                variableLength buf;
                if ((buf.data = (gchar *) DecompressRTF (filename, &buf.size)) != NULL) {
                    sprintf(ifilename, "%s/%s.rtf", tmpdir, tnef->subject.data);
                    for (i=0; i<strlen (ifilename); i++)
                        if (ifilename[i] == ' ')
                            ifilename[i] = '_';

                    if ((fptr = fopen(ifilename, "wb"))==NULL) {
                        printf("ERROR: Error writing file to disk!");
                    } else {
                        fwrite (buf.data,
                                sizeof (BYTE),
                                buf.size,
                                fptr);
                        fclose (fptr);
                    }
                    free (buf.data);
                }
            }
	}
    }

    /* Now process each attachment */
    p = tnef->starting_attach.next;
    count = 0;
    while (p != NULL) {
        count++;
        /* Make sure it has a size. */
        if (p->FileData.size > 0) {
            object = 1;

            /* See if the contents are stored as "attached data" */
	    /* Inside the MAPI blocks. */
            if ((filedata = MAPIFindProperty (&(p->MAPI),
                                    PROP_TAG (PT_OBJECT, PR_ATTACH_DATA_OBJ)))
                    == MAPI_UNDEFINED) {
                if ((filedata = MAPIFindProperty (&(p->MAPI),
                                    PROP_TAG (PT_BINARY, PR_ATTACH_DATA_OBJ)))
		   == MAPI_UNDEFINED) {
                    /* Nope, standard TNEF stuff. */
                    filedata = &(p->FileData);
                    object = 0;
                }
            }
            /* See if this is an embedded TNEF stream. */
            RealAttachment = 1;
            if (object == 1) {
                /*  This is an "embedded object", so skip the */
                /* 16-byte identifier first. */
                TNEFStruct emb_tnef;
                DWORD signature;
                memcpy (&signature, filedata->data+16, sizeof (DWORD));
                if (TNEFCheckForSignature (signature) == 0) {
                    /* Has a TNEF signature, so process it. */
                    TNEFInitialize (&emb_tnef);
                    emb_tnef.Debug = tnef->Debug;
                    if (TNEFParseMemory ((guchar *) filedata->data+16,
                             filedata->size-16, &emb_tnef) != -1) {
                        processTnef (&emb_tnef, tmpdir);
                        RealAttachment = 0;
                    }
                    TNEFFree (&emb_tnef);
                }
            } else {
                TNEFStruct emb_tnef;
                DWORD signature;
                memcpy (&signature, filedata->data, sizeof (DWORD));
                if (TNEFCheckForSignature (signature) == 0) {
                    /* Has a TNEF signature, so process it. */
                    TNEFInitialize (&emb_tnef);
                    emb_tnef.Debug = tnef->Debug;
                    if (TNEFParseMemory ((guchar *) filedata->data,
                            filedata->size, &emb_tnef) != -1) {
                        processTnef (&emb_tnef, tmpdir);
                        RealAttachment = 0;
                    }
                    TNEFFree (&emb_tnef);
                }
            }
            if ((RealAttachment == 1) || (saveintermediate == 1)) {
		gchar tmpname[20];
                /* Ok, it's not an embedded stream, so now we */
		/* process it. */
                if ((filename = MAPIFindProperty (&(p->MAPI),
                                        PROP_TAG (PT_STRING8, PR_ATTACH_LONG_FILENAME)))
                        == MAPI_UNDEFINED) {
                    if ((filename = MAPIFindProperty (&(p->MAPI),
                                        PROP_TAG (PT_STRING8, PR_DISPLAY_NAME)))
                            == MAPI_UNDEFINED) {
                        filename = &(p->Title);
                    }
                }
                if (filename->size == 1) {
                    filename->size = 20;
                    sprintf(tmpname, "file_%03i.dat", count);
                    filename->data = tmpname;
                }
                sprintf(ifilename, "%s/%s", tmpdir, filename->data);
                for (i=0; i<strlen (ifilename); i++)
                    if (ifilename[i] == ' ')
                        ifilename[i] = '_';

		if ((fptr = fopen(ifilename, "wb"))==NULL) {
		    printf("ERROR: Error writing file to disk!");
		} else {
		    if (object == 1) {
			fwrite (filedata->data + 16,
			       sizeof (BYTE),
			       filedata->size - 16,
			       fptr);
		    } else {
			fwrite (filedata->data,
			       sizeof (BYTE),
			       filedata->size,
			       fptr);
		    }
		    fclose (fptr);
		}
            }
        } /* if size>0 */
        p=p->next;
    } /* while p!= null */
}