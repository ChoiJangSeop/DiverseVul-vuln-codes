int ReadJpegSections (FILE * infile, ReadMode_t ReadMode)
{
    int a;
    int HaveCom = FALSE;

    a = fgetc(infile);

    if (a != 0xff || fgetc(infile) != M_SOI){
        return FALSE;
    }

    ImageInfo.JfifHeader.XDensity = ImageInfo.JfifHeader.YDensity = 300;
    ImageInfo.JfifHeader.ResolutionUnits = 1;

    for(;;){
        int itemlen;
        int prev;
        int marker = 0;
        int ll,lh, got;
        uchar * Data;

        CheckSectionsAllocated();

        prev = 0;
        for (a=0;;a++){
            marker = fgetc(infile);
            if (marker != 0xff && prev == 0xff) break;
            if (marker == EOF){
                ErrFatal("Unexpected end of file");
            }
            prev = marker;
        }

        if (a > 10){
            ErrNonfatal("Extraneous %d padding bytes before section %02X",a-1,marker);
        }

        Sections[SectionsRead].Type = marker;
  
        // Read the length of the section.
        lh = fgetc(infile);
        ll = fgetc(infile);
        if (lh == EOF || ll == EOF){
            ErrFatal("Unexpected end of file");
        }

        itemlen = (lh << 8) | ll;

        if (itemlen < 2){
            ErrFatal("invalid marker");
        }

        Sections[SectionsRead].Size = itemlen;

        Data = (uchar *)malloc(itemlen);
        if (Data == NULL){
            ErrFatal("Could not allocate memory");
        }
        Sections[SectionsRead].Data = Data;

        // Store first two pre-read bytes.
        Data[0] = (uchar)lh;
        Data[1] = (uchar)ll;

        got = fread(Data+2, 1, itemlen-2, infile); // Read the whole section.
        if (got != itemlen-2){
            ErrFatal("Premature end of file?");
        }
        SectionsRead += 1;

        switch(marker){

            case M_SOS:   // stop before hitting compressed data 
                // If reading entire image is requested, read the rest of the data.
                if (ReadMode & READ_IMAGE){
                    int cp, ep, size;
                    // Determine how much file is left.
                    cp = ftell(infile);
                    fseek(infile, 0, SEEK_END);
                    ep = ftell(infile);
                    fseek(infile, cp, SEEK_SET);

                    size = ep-cp;
                    Data = (uchar *)malloc(size);
                    if (Data == NULL){
                        ErrFatal("could not allocate data for entire image");
                    }

                    got = fread(Data, 1, size, infile);
                    if (got != size){
                        ErrFatal("could not read the rest of the image");
                    }

                    CheckSectionsAllocated();
                    Sections[SectionsRead].Data = Data;
                    Sections[SectionsRead].Size = size;
                    Sections[SectionsRead].Type = PSEUDO_IMAGE_MARKER;
                    SectionsRead ++;
                    HaveAll = 1;
                }
                return TRUE;

            case M_DQT:
                // Use for jpeg quality guessing
                process_DQT(Data, itemlen);
                break;

            case M_DHT:   
                // Use for jpeg quality guessing
                process_DHT(Data, itemlen);
                break;


            case M_EOI:   // in case it's a tables-only JPEG stream
                fprintf(stderr,"No image in jpeg!\n");
                return FALSE;

            case M_COM: // Comment section
                if (HaveCom || ((ReadMode & READ_METADATA) == 0)){
                    // Discard this section.
                    free(Sections[--SectionsRead].Data);
                }else{
                    process_COM(Data, itemlen);
                    HaveCom = TRUE;
                }
                break;

            case M_JFIF:
                // Regular jpegs always have this tag, exif images have the exif
                // marker instead, althogh ACDsee will write images with both markers.
                // this program will re-create this marker on absence of exif marker.
                // hence no need to keep the copy from the file.
                if (itemlen < 16){
                    fprintf(stderr,"Jfif header too short\n");
                    goto ignore;
                }
                if (memcmp(Data+2, "JFIF\0",5)){
                    fprintf(stderr,"Header missing JFIF marker\n");
                }

                ImageInfo.JfifHeader.Present = TRUE;
                ImageInfo.JfifHeader.ResolutionUnits = Data[9];
                ImageInfo.JfifHeader.XDensity = (Data[10]<<8) | Data[11];
                ImageInfo.JfifHeader.YDensity = (Data[12]<<8) | Data[13];
                if (ShowTags){
                    printf("JFIF SOI marker: Units: %d ",ImageInfo.JfifHeader.ResolutionUnits);
                    switch(ImageInfo.JfifHeader.ResolutionUnits){
                        case 0: printf("(aspect ratio)"); break;
                        case 1: printf("(dots per inch)"); break;
                        case 2: printf("(dots per cm)"); break;
                        default: printf("(unknown)"); break;
                    }
                    printf("  X-density=%d Y-density=%d\n",ImageInfo.JfifHeader.XDensity, ImageInfo.JfifHeader.YDensity);

                    if (Data[14] || Data[15]){
                        fprintf(stderr,"Ignoring jfif header thumbnail\n");
                    }
                }

                ignore:

                free(Sections[--SectionsRead].Data);
                break;

            case M_EXIF:
                // There can be different section using the same marker.
                if (ReadMode & READ_METADATA){
                    if (memcmp(Data+2, "Exif", 4) == 0){
                        process_EXIF(Data, itemlen);
                        break;
                    }else if (memcmp(Data+2, "http:", 5) == 0){
                        Sections[SectionsRead-1].Type = M_XMP; // Change tag for internal purposes.
                        if (ShowTags){
                            printf("Image contains XMP section, %d bytes long\n", itemlen);
                            if (ShowTags){
                                ShowXmp(Sections[SectionsRead-1]);
                            }
                        }
                        break;
                    }
                }
                // Oterwise, discard this section.
                free(Sections[--SectionsRead].Data);
                break;

            case M_IPTC:
                if (ReadMode & READ_METADATA){
                    if (ShowTags){
                        printf("Image contains IPTC section, %d bytes long\n", itemlen);
                    }
                    // Note: We just store the IPTC section.  Its relatively straightforward
                    // and we don't act on any part of it, so just display it at parse time.
                }else{
                    free(Sections[--SectionsRead].Data);
                }
                break;
           
            case M_SOF0: 
            case M_SOF1: 
            case M_SOF2: 
            case M_SOF3: 
            case M_SOF5: 
            case M_SOF6: 
            case M_SOF7: 
            case M_SOF9: 
            case M_SOF10:
            case M_SOF11:
            case M_SOF13:
            case M_SOF14:
            case M_SOF15:
                if (itemlen < 8){
                    fprintf(stderr,"Section too short\n");
                    break;
                }
                process_SOFn(Data, marker);
                break;
            default:
                // Skip any other sections.
                if (ShowTags){
                    printf("Jpeg section marker 0x%02x size %d\n",marker, itemlen);
                }
                break;
        }
    }
    return TRUE;
}