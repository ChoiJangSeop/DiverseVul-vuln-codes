void saveVCalendar (TNEFStruct *tnef, const gchar *tmpdir) {
    gchar ifilename[256];
    variableLength *filename;
    gchar *charptr, *charptr2;
    FILE *fptr;
    gint index;
    DWORD *dword_ptr;
    DWORD dword_val;
    dtr thedate;

    sprintf(ifilename, "%s/calendar.ics", tmpdir);
    printf("%s\n", ifilename);

    if ((fptr = fopen(ifilename, "wb"))==NULL) {
            printf("Error writing file to disk!");
    } else {
        fprintf(fptr, "BEGIN:VCALENDAR\n");
        if (tnef->messageClass[0] != 0) {
            charptr2=tnef->messageClass;
            charptr=charptr2;
            while (*charptr != 0) {
                if (*charptr == '.') {
                    charptr2 = charptr;
                }
                charptr++;
            }
            if (strcmp(charptr2, ".MtgCncl") == 0) {
                fprintf(fptr, "METHOD:CANCEL\n");
            } else {
                fprintf(fptr, "METHOD:REQUEST\n");
            }
        } else {
            fprintf(fptr, "METHOD:REQUEST\n");
        }
        fprintf(fptr, "VERSION:2.0\n");
        fprintf(fptr, "BEGIN:VEVENT\n");

	/* UID
	   After alot of comparisons, I'm reasonably sure this is totally
	   wrong.  But it's not really necessary. */

	/* I think it only exists to connect future modification entries to
	   this entry. so as long as it's incorrectly interpreted the same way
	   every time, it should be ok :) */
        filename = NULL;
        if ((filename=MAPIFindUserProp (&(tnef->MapiProperties),
                        PROP_TAG (PT_BINARY, 0x3))) == MAPI_UNDEFINED) {
            if ((filename=MAPIFindUserProp (&(tnef->MapiProperties),
                            PROP_TAG (PT_BINARY, 0x23))) == MAPI_UNDEFINED) {
                filename = NULL;
            }
        }
        if (filename!=NULL) {
            fprintf(fptr, "UID:");
            for (index=0;index<filename->size;index++) {
                fprintf(fptr,"%02X", (guchar)filename->data[index]);
            }
            fprintf(fptr,"\n");
        }

        /* Sequence */
        filename = NULL;
        if ((filename=MAPIFindUserProp (&(tnef->MapiProperties),
                        PROP_TAG (PT_LONG, 0x8201))) != MAPI_UNDEFINED) {
            dword_ptr = (DWORD*)filename->data;
            fprintf(fptr, "SEQUENCE:%i\n", (gint) *dword_ptr);
        }
        if ((filename=MAPIFindProperty (&(tnef->MapiProperties),
                        PROP_TAG (PT_BINARY, PR_SENDER_SEARCH_KEY)))
                != MAPI_UNDEFINED) {
            charptr = filename->data;
            charptr2 = strstr(charptr, ":");
            if (charptr2 == NULL)
                charptr2 = charptr;
            else
                charptr2++;
            fprintf(fptr, "ORGANIZER;CN=\"%s\":MAILTO:%s\n",
                    charptr2, charptr2);
        }

        /* Required Attendees */
        if ((filename = MAPIFindUserProp (&(tnef->MapiProperties),
                        PROP_TAG (PT_STRING8, 0x823b))) != MAPI_UNDEFINED) {
	    /* We have a list of required participants, so
	       write them out. */
            if (filename->size > 1) {
                charptr = filename->data-1;
                while (charptr != NULL) {
                    charptr++;
                    charptr2 = strstr(charptr, ";");
                    if (charptr2 != NULL) {
                        *charptr2 = 0;
                    }
                    while (*charptr == ' ')
                        charptr++;
                    fprintf(fptr, "ATTENDEE;PARTSTAT=NEEDS-ACTION;");
                    fprintf(fptr, "ROLE=REQ-PARTICIPANT;RSVP=TRUE;");
                    fprintf(fptr, "CN=\"%s\":MAILTO:%s\n",
                                charptr, charptr);
                    charptr = charptr2;
                }
            }
            /* Optional attendees */
            if ((filename = MAPIFindUserProp (&(tnef->MapiProperties),
                            PROP_TAG (PT_STRING8, 0x823c))) != MAPI_UNDEFINED) {
                    /* The list of optional participants */
                if (filename->size > 1) {
                    charptr = filename->data-1;
                    while (charptr != NULL) {
                        charptr++;
                        charptr2 = strstr(charptr, ";");
                        if (charptr2 != NULL) {
                            *charptr2 = 0;
                        }
                        while (*charptr == ' ')
                            charptr++;
                        fprintf(fptr, "ATTENDEE;PARTSTAT=NEEDS-ACTION;");
                        fprintf(fptr, "ROLE=OPT-PARTICIPANT;RSVP=TRUE;");
                        fprintf(fptr, "CN=\"%s\":MAILTO:%s\n",
                                charptr, charptr);
                        charptr = charptr2;
                    }
                }
            }
        } else if ((filename = MAPIFindUserProp (&(tnef->MapiProperties),
                        PROP_TAG (PT_STRING8, 0x8238))) != MAPI_UNDEFINED) {
            if (filename->size > 1) {
                charptr = filename->data-1;
                while (charptr != NULL) {
                    charptr++;
                    charptr2 = strstr(charptr, ";");
                    if (charptr2 != NULL) {
                        *charptr2 = 0;
                    }
                    while (*charptr == ' ')
                        charptr++;
                    fprintf(fptr, "ATTENDEE;PARTSTAT=NEEDS-ACTION;");
                    fprintf(fptr, "ROLE=REQ-PARTICIPANT;RSVP=TRUE;");
                    fprintf(fptr, "CN=\"%s\":MAILTO:%s\n",
                                charptr, charptr);
                    charptr = charptr2;
                }
            }

        }
        /* Summary */
        filename = NULL;
        if ((filename=MAPIFindProperty (&(tnef->MapiProperties),
                        PROP_TAG (PT_STRING8, PR_CONVERSATION_TOPIC)))
                != MAPI_UNDEFINED) {
            fprintf(fptr, "SUMMARY:");
            cstylefprint (fptr, filename);
            fprintf(fptr, "\n");
        }

        /* Description */
        if ((filename=MAPIFindProperty (&(tnef->MapiProperties),
                                PROP_TAG (PT_BINARY, PR_RTF_COMPRESSED)))
                != MAPI_UNDEFINED) {
            variableLength buf;
            if ((buf.data = (gchar *) DecompressRTF (filename, &buf.size)) != NULL) {
                fprintf(fptr, "DESCRIPTION:");
                printRtf (fptr, &buf);
                free (buf.data);
            }

        }

        /* Location */
        filename = NULL;
        if ((filename=MAPIFindUserProp (&(tnef->MapiProperties),
                        PROP_TAG (PT_STRING8, 0x0002))) == MAPI_UNDEFINED) {
            if ((filename=MAPIFindUserProp (&(tnef->MapiProperties),
                            PROP_TAG (PT_STRING8, 0x8208))) == MAPI_UNDEFINED) {
                filename = NULL;
            }
        }
        if (filename != NULL) {
            fprintf(fptr,"LOCATION: %s\n", filename->data);
        }
        /* Date Start */
        filename = NULL;
        if ((filename=MAPIFindUserProp (&(tnef->MapiProperties),
                        PROP_TAG (PT_SYSTIME, 0x820d))) == MAPI_UNDEFINED) {
            if ((filename=MAPIFindUserProp (&(tnef->MapiProperties),
                            PROP_TAG (PT_SYSTIME, 0x8516))) == MAPI_UNDEFINED) {
                filename=NULL;
            }
        }
        if (filename != NULL) {
            fprintf(fptr, "DTSTART:");
            MAPISysTimetoDTR ((guchar *) filename->data, &thedate);
            fprintf(fptr,"%04i%02i%02iT%02i%02i%02iZ\n",
                    thedate.wYear, thedate.wMonth, thedate.wDay,
                    thedate.wHour, thedate.wMinute, thedate.wSecond);
        }
        /* Date End */
        filename = NULL;
        if ((filename=MAPIFindUserProp (&(tnef->MapiProperties),
                        PROP_TAG (PT_SYSTIME, 0x820e))) == MAPI_UNDEFINED) {
            if ((filename=MAPIFindUserProp (&(tnef->MapiProperties),
                            PROP_TAG (PT_SYSTIME, 0x8517))) == MAPI_UNDEFINED) {
                filename=NULL;
            }
        }
        if (filename != NULL) {
            fprintf(fptr, "DTEND:");
            MAPISysTimetoDTR ((guchar *) filename->data, &thedate);
            fprintf(fptr,"%04i%02i%02iT%02i%02i%02iZ\n",
                    thedate.wYear, thedate.wMonth, thedate.wDay,
                    thedate.wHour, thedate.wMinute, thedate.wSecond);
        }
        /* Date Stamp */
        filename = NULL;
        if ((filename=MAPIFindUserProp (&(tnef->MapiProperties),
                        PROP_TAG (PT_SYSTIME, 0x8202))) != MAPI_UNDEFINED) {
            fprintf(fptr, "CREATED:");
            MAPISysTimetoDTR ((guchar *) filename->data, &thedate);
            fprintf(fptr,"%04i%02i%02iT%02i%02i%02iZ\n",
                    thedate.wYear, thedate.wMonth, thedate.wDay,
                    thedate.wHour, thedate.wMinute, thedate.wSecond);
        }
        /* Class */
        filename = NULL;
        if ((filename=MAPIFindUserProp (&(tnef->MapiProperties),
                        PROP_TAG (PT_BOOLEAN, 0x8506))) != MAPI_UNDEFINED) {
            dword_ptr = (DWORD*)filename->data;
            dword_val = SwapDWord ((BYTE*)dword_ptr);
            fprintf(fptr, "CLASS:" );
            if (*dword_ptr == 1) {
                fprintf(fptr,"PRIVATE\n");
            } else {
                fprintf(fptr,"PUBLIC\n");
            }
        }
        /* Recurrence */
        filename = NULL;
        if ((filename=MAPIFindUserProp (&(tnef->MapiProperties),
                        PROP_TAG (PT_BINARY, 0x8216))) != MAPI_UNDEFINED) {
            printRrule (fptr, filename->data, filename->size, tnef);
        }

        /* Wrap it up */
        fprintf(fptr, "END:VEVENT\n");
        fprintf(fptr, "END:VCALENDAR\n");
        fclose (fptr);
    }
}