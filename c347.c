void saveVTask (TNEFStruct *tnef, const gchar *tmpdir) {
    variableLength *vl;
    variableLength *filename;
    gint index,i;
    gchar ifilename[256];
    gchar *charptr, *charptr2;
    dtr thedate;
    FILE *fptr;
    DWORD *dword_ptr;
    DWORD dword_val;

    vl = MAPIFindProperty (&(tnef->MapiProperties), PROP_TAG (PT_STRING8, PR_CONVERSATION_TOPIC));

    if (vl == MAPI_UNDEFINED) {
        return;
    }

    index = strlen (vl->data);
    while (vl->data[index] == ' ')
            vl->data[index--] = 0;

    sprintf(ifilename, "%s/%s.ics", tmpdir, vl->data);
    for (i=0; i<strlen (ifilename); i++)
        if (ifilename[i] == ' ')
            ifilename[i] = '_';
    printf("%s\n", ifilename);

    if ((fptr = fopen(ifilename, "wb"))==NULL) {
            printf("Error writing file to disk!");
    } else {
        fprintf(fptr, "BEGIN:VCALENDAR\n");
        fprintf(fptr, "VERSION:2.0\n");
        fprintf(fptr, "METHOD:PUBLISH\n");
        filename = NULL;

        fprintf(fptr, "BEGIN:VTODO\n");
        if (tnef->messageID[0] != 0) {
            fprintf(fptr,"UID:%s\n", tnef->messageID);
        }
        filename = MAPIFindUserProp (&(tnef->MapiProperties), \
                        PROP_TAG (PT_STRING8, 0x8122));
        if (filename != MAPI_UNDEFINED) {
            fprintf(fptr, "ORGANIZER:%s\n", filename->data);
        }

        if ((filename = MAPIFindProperty (&(tnef->MapiProperties), PROP_TAG (PT_STRING8, PR_DISPLAY_TO))) != MAPI_UNDEFINED) {
            filename = MAPIFindUserProp (&(tnef->MapiProperties), PROP_TAG (PT_STRING8, 0x811f));
        }
        if ((filename != MAPI_UNDEFINED) && (filename->size > 1)) {
            charptr = filename->data-1;
            while (charptr != NULL) {
                charptr++;
                charptr2 = strstr(charptr, ";");
                if (charptr2 != NULL) {
                    *charptr2 = 0;
                }
                while (*charptr == ' ')
                    charptr++;
                fprintf(fptr, "ATTENDEE;CN=%s;ROLE=REQ-PARTICIPANT:%s\n", charptr, charptr);
                charptr = charptr2;
            }
        }

        if (tnef->subject.size > 0) {
            fprintf(fptr,"SUMMARY:");
            cstylefprint (fptr,&(tnef->subject));
            fprintf(fptr,"\n");
        }

        if (tnef->body.size > 0) {
            fprintf(fptr,"DESCRIPTION:");
            cstylefprint (fptr,&(tnef->body));
            fprintf(fptr,"\n");
        }

        filename = MAPIFindProperty (&(tnef->MapiProperties), \
                    PROP_TAG (PT_SYSTIME, PR_CREATION_TIME));
        if (filename != MAPI_UNDEFINED) {
            fprintf(fptr, "DTSTAMP:");
            MAPISysTimetoDTR ((guchar *) filename->data, &thedate);
            fprintf(fptr,"%04i%02i%02iT%02i%02i%02iZ\n",
                    thedate.wYear, thedate.wMonth, thedate.wDay,
                    thedate.wHour, thedate.wMinute, thedate.wSecond);
        }

        filename = MAPIFindUserProp (&(tnef->MapiProperties), \
                    PROP_TAG (PT_SYSTIME, 0x8517));
        if (filename != MAPI_UNDEFINED) {
            fprintf(fptr, "DUE:");
            MAPISysTimetoDTR ((guchar *) filename->data, &thedate);
            fprintf(fptr,"%04i%02i%02iT%02i%02i%02iZ\n",
                    thedate.wYear, thedate.wMonth, thedate.wDay,
                    thedate.wHour, thedate.wMinute, thedate.wSecond);
        }
        filename = MAPIFindProperty (&(tnef->MapiProperties), \
                    PROP_TAG (PT_SYSTIME, PR_LAST_MODIFICATION_TIME));
        if (filename != MAPI_UNDEFINED) {
            fprintf(fptr, "LAST-MODIFIED:");
            MAPISysTimetoDTR ((guchar *) filename->data, &thedate);
            fprintf(fptr,"%04i%02i%02iT%02i%02i%02iZ\n",
                    thedate.wYear, thedate.wMonth, thedate.wDay,
                    thedate.wHour, thedate.wMinute, thedate.wSecond);
        }
        /* Class */
        filename = MAPIFindUserProp (&(tnef->MapiProperties), \
                        PROP_TAG (PT_BOOLEAN, 0x8506));
        if (filename != MAPI_UNDEFINED) {
            dword_ptr = (DWORD*)filename->data;
            dword_val = SwapDWord ((BYTE*)dword_ptr);
            fprintf(fptr, "CLASS:" );
            if (*dword_ptr == 1) {
                fprintf(fptr,"PRIVATE\n");
            } else {
                fprintf(fptr,"PUBLIC\n");
            }
        }
        fprintf(fptr, "END:VTODO\n");
        fprintf(fptr, "END:VCALENDAR\n");
        fclose (fptr);
    }

}