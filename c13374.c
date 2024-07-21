int RemoveUnknownSections(void)
{
    int a;
    int Modified = FALSE;
    for (a=0;a<SectionsRead-1;){
        switch(Sections[a].Type){
            case  M_SOF0:
            case  M_SOF1:
            case  M_SOF2:
            case  M_SOF3:
            case  M_SOF5:
            case  M_SOF6:
            case  M_SOF7:
            case  M_SOF9:
            case  M_SOF10:
            case  M_SOF11:
            case  M_SOF13:
            case  M_SOF14:
            case  M_SOF15:
            case  M_SOI:
            case  M_EOI:
            case  M_SOS:
            case  M_JFIF:
            case  M_EXIF:
            case  M_XMP:
            case  M_COM:
            case  M_DQT:
            case  M_DHT:
            case  M_DRI:
            case  M_IPTC:
                // keep.
                a++;
                break;
            default:
                // Unknown.  Delete.
                free (Sections[a].Data);
                // Move succeding sections back by one to close space in array.
                memmove(Sections+a, Sections+a+1, sizeof(Section_t) * (SectionsRead-a));
                SectionsRead -= 1;
                Modified = TRUE;
        }
    }
    return Modified;
}