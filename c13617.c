static void ProcessExifDir(unsigned char * DirStart, unsigned char * OffsetBase, 
        int ExifLength, int NestingLevel)
{
    int de;
    int a;
    int NumDirEntries;
    int ThumbnailOffset = 0;
    int ThumbnailSize = 0;
    char IndentString[25];
    int OffsetVal;

    if (NestingLevel > 4){
        ErrNonfatal("Maximum Exif directory nesting exceeded (corrupt Exif header)", 0,0);
        return;
    }

    memset(IndentString, ' ', 25);
    IndentString[NestingLevel * 4] = '\0';


    NumDirEntries = Get16u(DirStart);
    #define DIR_ENTRY_ADDR(Start, Entry) (Start+2+12*(Entry))

    {
        unsigned char * DirEnd;
        DirEnd = DIR_ENTRY_ADDR(DirStart, NumDirEntries);
        if (DirEnd+4 > (OffsetBase+ExifLength)){
            if (DirEnd+2 == OffsetBase+ExifLength || DirEnd == OffsetBase+ExifLength){
                // Version 1.3 of jhead would truncate a bit too much.
                // This also caught later on as well.
            }else{
                ErrNonfatal("Illegally sized Exif subdirectory (%d entries)",NumDirEntries,0);
                return;
            }
        }
        if (DumpExifMap){
            printf("Map: %05u-%05u: Directory\n",(int)(DirStart-OffsetBase), (int)(DirEnd+4-OffsetBase));
        }


    }

    if (ShowTags){
        printf("(dir has %d entries)\n",NumDirEntries);
    }

    for (de=0;de<NumDirEntries;de++){
        int Tag, Format, Components;
        unsigned char * ValuePtr;
        int ByteCount;
        unsigned char * DirEntry;
        DirEntry = DIR_ENTRY_ADDR(DirStart, de);

        Tag = Get16u(DirEntry);
        Format = Get16u(DirEntry+2);
        Components = Get32u(DirEntry+4);

        if (Components > 0x10000){
            //Components count too large could cause overflow on subsequent check
            ErrNonfatal("Bad components count %x", Components,0);
            continue;
        }

        if ((Format-1) >= NUM_FORMATS) {
            // (-1) catches illegal zero case as unsigned underflows to positive large.
            ErrNonfatal("Illegal number format %d for tag %04x in Exif", Format, Tag);
            continue;
        }

        if ((unsigned)Components > 0x10000){
            ErrNonfatal("Too many components %d for tag %04x in Exif", Components, Tag);
            continue;
        }

        ByteCount = Components * BytesPerFormat[Format];

        if (ByteCount > 4){
            OffsetVal = Get32u(DirEntry+8);
            // If its bigger than 4 bytes, the dir entry contains an offset.
            if (OffsetVal+ByteCount > ExifLength || OffsetVal < 0 || OffsetVal > 65536){
                // Bogus pointer offset and / or bytecount value
                ErrNonfatal("Illegal value pointer for tag %04x in Exif", Tag,0);
                continue;
            }
            ValuePtr = OffsetBase+OffsetVal;

            if (OffsetVal > ImageInfo.LargestExifOffset){
                ImageInfo.LargestExifOffset = OffsetVal;
            }

            if (DumpExifMap){
                printf("Map: %05u-%05u:   Data for tag %04x\n",OffsetVal, OffsetVal+ByteCount, Tag);
            }
        }else{
            // 4 bytes or less and value is in the dir entry itself
            ValuePtr = DirEntry+8;
        }

        if (Tag == TAG_MAKER_NOTE){
            if (ShowTags){
                printf("%s    Maker note: ",IndentString);
            }
            ProcessMakerNote(ValuePtr, ByteCount, OffsetBase, ExifLength);
            continue;
        }

        if (ShowTags){
            // Show tag name
            for (a=0;;a++){
                if (a >= TAG_TABLE_SIZE){
                    printf("%s    Unknown Tag %04x Value = ", IndentString, Tag);
                    break;
                }
                if (TagTable[a].Tag == Tag){
                    printf("%s    %s = ",IndentString, TagTable[a].Desc);
                    break;
                }
            }

            // Show tag value.
            switch(Format){
                case FMT_BYTE:
                    if(ByteCount>1){
                        for (a=0;a<ByteCount;a+=2){
                            int cv = *(char *)(ValuePtr+a)+(*(char *)(ValuePtr+a+1)<<8);
                            // Note that after getting the 16-bit char, putchar truncates it back
                            // down to 8 bit.  Unicoe and linus console is something I don't understand.
                            putchar(cv);
                        }
                        putchar('\n');
                    }else{
                        PrintFormatNumber(ValuePtr, Format, ByteCount);
                        printf("\n");
                    }
                    break;

                case FMT_UNDEFINED:
                    // Undefined is typically an ascii string.

                case FMT_STRING:
                    // String arrays printed without function call (different from int arrays)
                    {
                        int NoPrint = 0;
                        printf("\"");
                        for (a=0;a<ByteCount;a++){
                            if (ValuePtr[a] >= 32){
                                putchar(ValuePtr[a]);
                                NoPrint = 0;
                            }else{
                                // Avoiding indicating too many unprintable characters of proprietary
                                // bits of binary information this program may not know how to parse.
                                if (!NoPrint && a != ByteCount-1){
                                    putchar('?');
                                    NoPrint = 1;
                                }
                            }
                        }
                        printf("\"\n");
                    }
                    break;

                default:
                    // Handle arrays of numbers later (will there ever be?)
                    PrintFormatNumber(ValuePtr, Format, ByteCount);
                    printf("\n");
            }
        }

        // Extract useful components of tag
        switch(Tag){

            case TAG_MAKE:
                strncpy(ImageInfo.CameraMake, (char *)ValuePtr, ByteCount < 31 ? ByteCount : 31);
                break;

            case TAG_MODEL:
                strncpy(ImageInfo.CameraModel, (char *)ValuePtr, ByteCount < 39 ? ByteCount : 39);
                break;

            case TAG_DATETIME_ORIGINAL:
            case TAG_DATETIME_DIGITIZED:
            case TAG_DATETIME:
                if (ValuePtr+19 >= OffsetBase+ExifLength){
                    ErrNonfatal("Incomplete time",0,0);
                    continue;
                }
                
                if (Tag == TAG_DATETIME_ORIGINAL || !isdigit(ImageInfo.DateTime[0])){
                    // If we don't already have a DATETIME_ORIGINAL, use whatever
                    // time fields we may have.  But if ORIGINAL tag comes later, use that one.
                    strncpy(ImageInfo.DateTime, (char *)ValuePtr, 19);
                }

                if (ImageInfo.numDateTimeTags >= MAX_DATE_COPIES){
                    ErrNonfatal("More than %d date fields in Exif.  This is nuts", MAX_DATE_COPIES, 0);
                    break;
                }
                ImageInfo.DateTimeOffsets[ImageInfo.numDateTimeTags++] = 
                    (char *)ValuePtr - (char *)OffsetBase;
                break;

            case TAG_USERCOMMENT:
                if (ImageInfo.Comments[0]){ // We already have a jpeg comment.
                    // Already have a comment (probably windows comment), skip this one.
                    if (ShowTags) printf("Multiple comments in exif header\n");
                    break; // Already have a windows comment, skip this one.
                }

                // Comment is often padded with trailing spaces.  Remove these first.
                for (a=ByteCount;;){
                    a--;
                    if ((ValuePtr)[a] == ' '){
                        (ValuePtr)[a] = '\0';
                    }else{
                        break;
                    }
                    if (a == 0) break;
                }

                // Copy the comment
                {
                    int msiz = ExifLength - (ValuePtr-OffsetBase);
                    if (msiz > ByteCount) msiz = ByteCount;
                    if (msiz > MAX_COMMENT_SIZE-1) msiz = MAX_COMMENT_SIZE-1;
                    if (msiz > 5 && memcmp(ValuePtr, "ASCII",5) == 0){
                        for (a=5;a<10 && a < msiz;a++){
                            int c = (ValuePtr)[a];
                            if (c != '\0' && c != ' '){
                                strncpy(ImageInfo.Comments, (char *)ValuePtr+a, msiz-a);
                                break;
                            }
                        }
                    }else{
                        strncpy(ImageInfo.Comments, (char *)ValuePtr, msiz);
                    }
                }
                break;

            case TAG_FNUMBER:
                // Simplest way of expressing aperture, so I trust it the most.
                // (overwrite previously computd value if there is one)
                ImageInfo.ApertureFNumber = (float)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_APERTURE:
            case TAG_MAXAPERTURE:
                // More relevant info always comes earlier, so only use this field if we don't 
                // have appropriate aperture information yet.
                if (ImageInfo.ApertureFNumber == 0){
                    ImageInfo.ApertureFNumber 
                        = (float)exp(ConvertAnyFormat(ValuePtr, Format)*log(2)*0.5);
                }
                break;

            case TAG_FOCALLENGTH:
                // Nice digital cameras actually save the focal length as a function
                // of how farthey are zoomed in.
                ImageInfo.FocalLength = (float)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_SUBJECT_DISTANCE:
                // Inidcates the distacne the autofocus camera is focused to.
                // Tends to be less accurate as distance increases.
                ImageInfo.Distance = (float)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_EXPOSURETIME:
                // Simplest way of expressing exposure time, so I trust it most.
                // (overwrite previously computd value if there is one)
                ImageInfo.ExposureTime = (float)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_SHUTTERSPEED:
                // More complicated way of expressing exposure time, so only use
                // this value if we don't already have it from somewhere else.
                if (ImageInfo.ExposureTime == 0){
                    ImageInfo.ExposureTime 
                        = (float)(1/exp(ConvertAnyFormat(ValuePtr, Format)*log(2)));
                }
                break;


            case TAG_FLASH:
                ImageInfo.FlashUsed=(int)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_ORIENTATION:
                if (NumOrientations >= 2){
                    // Can have another orientation tag for the thumbnail, but if there's
                    // a third one, things are strange.
                    ErrNonfatal("More than two orientation in Exif",0,0);
                    break;
                }
                OrientationPtr[NumOrientations] = ValuePtr;
                OrientationNumFormat[NumOrientations] = Format;
                if (NumOrientations == 0){
                    ImageInfo.Orientation = (int)ConvertAnyFormat(ValuePtr, Format);
                }
                if (ImageInfo.Orientation < 0 || ImageInfo.Orientation > 8){
                    ErrNonfatal("Undefined rotation value %d in Exif", ImageInfo.Orientation, 0);
                }
                NumOrientations += 1;
                break;

            case TAG_PIXEL_Y_DIMENSION:
            case TAG_PIXEL_X_DIMENSION:
                // Use largest of height and width to deal with images that have been
                // rotated to portrait format.
                a = (int)ConvertAnyFormat(ValuePtr, Format);
                if (ExifImageWidth < a) ExifImageWidth = a;
                break;

            case TAG_FOCAL_PLANE_XRES:
                FocalplaneXRes = ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_FOCAL_PLANE_UNITS:
                switch((int)ConvertAnyFormat(ValuePtr, Format)){
                    case 1: FocalplaneUnits = 25.4; break; // inch
                    case 2: 
                        // According to the information I was using, 2 means meters.
                        // But looking at the Cannon powershot's files, inches is the only
                        // sensible value.
                        FocalplaneUnits = 25.4;
                        break;

                    case 3: FocalplaneUnits = 10;   break;  // centimeter
                    case 4: FocalplaneUnits = 1;    break;  // millimeter
                    case 5: FocalplaneUnits = .001; break;  // micrometer
                }
                break;

            case TAG_EXPOSURE_BIAS:
                ImageInfo.ExposureBias = (float)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_WHITEBALANCE:
                ImageInfo.Whitebalance = (int)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_LIGHT_SOURCE:
                ImageInfo.LightSource = (int)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_METERING_MODE:
                ImageInfo.MeteringMode = (int)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_EXPOSURE_PROGRAM:
                ImageInfo.ExposureProgram = (int)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_EXPOSURE_INDEX:
                if (ImageInfo.ISOequivalent == 0){
                    // Exposure index and ISO equivalent are often used interchangeably,
                    // so we will do the same in jhead.
                    // http://photography.about.com/library/glossary/bldef_ei.htm
                    ImageInfo.ISOequivalent = (int)ConvertAnyFormat(ValuePtr, Format);
                }
                break;

            case TAG_EXPOSURE_MODE:
                ImageInfo.ExposureMode = (int)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_ISO_EQUIVALENT:
                ImageInfo.ISOequivalent = (int)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_DIGITALZOOMRATIO:
                ImageInfo.DigitalZoomRatio = (float)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_THUMBNAIL_OFFSET:
                ThumbnailOffset = (unsigned)ConvertAnyFormat(ValuePtr, Format);
                DirWithThumbnailPtrs = DirStart;
                break;

            case TAG_THUMBNAIL_LENGTH:
                ThumbnailSize = (unsigned)ConvertAnyFormat(ValuePtr, Format);
                ImageInfo.ThumbnailSizeOffset = ValuePtr-OffsetBase;
                break;

            case TAG_EXIF_OFFSET:
                if (ShowTags) printf("%s    Exif Dir:",IndentString);

            case TAG_INTEROP_OFFSET:
                if (Tag == TAG_INTEROP_OFFSET && ShowTags) printf("%s    Interop Dir:",IndentString);
                {
                    unsigned char * SubdirStart;
                    SubdirStart = OffsetBase + Get32u(ValuePtr);
                    if (SubdirStart < OffsetBase || SubdirStart > OffsetBase+ExifLength){
                        ErrNonfatal("Illegal Exif or interop ofset directory link",0,0);
                    }else{
                        ProcessExifDir(SubdirStart, OffsetBase, ExifLength, NestingLevel+1);
                    }
                    continue;
                }
                break;

            case TAG_GPSINFO:
                if (ShowTags) printf("%s    GPS info dir:",IndentString);
                {
                    unsigned char * SubdirStart;
                    SubdirStart = OffsetBase + Get32u(ValuePtr);
                    if (SubdirStart < OffsetBase || SubdirStart > OffsetBase+ExifLength){
                        ErrNonfatal("Illegal GPS directory link in Exif",0,0);
                    }else{
                        ProcessGpsInfo(SubdirStart, OffsetBase, ExifLength);
                    }
                    continue;
                }
                break;

            case TAG_FOCALLENGTH_35MM:
                // The focal length equivalent 35 mm is a 2.2 tag (defined as of April 2002)
                // if its present, use it to compute equivalent focal length instead of 
                // computing it from sensor geometry and actual focal length.
                ImageInfo.FocalLength35mmEquiv = (unsigned)ConvertAnyFormat(ValuePtr, Format);
                break;

            case TAG_DISTANCE_RANGE:
                // Three possible standard values:
                //   1 = macro, 2 = close, 3 = distant
                ImageInfo.DistanceRange = (int)ConvertAnyFormat(ValuePtr, Format);
                break;



            case TAG_X_RESOLUTION:
                if (NestingLevel==0) {// Only use the values from the top level directory
                    ImageInfo.xResolution = (float)ConvertAnyFormat(ValuePtr,Format);
                    // if yResolution has not been set, use the value of xResolution
                    if (ImageInfo.yResolution == 0.0) ImageInfo.yResolution = ImageInfo.xResolution;
                }
                break;

            case TAG_Y_RESOLUTION:
                if (NestingLevel==0) {// Only use the values from the top level directory
                    ImageInfo.yResolution = (float)ConvertAnyFormat(ValuePtr,Format);
                    // if xResolution has not been set, use the value of yResolution
                    if (ImageInfo.xResolution == 0.0) ImageInfo.xResolution = ImageInfo.yResolution;
                }
                break;

            case TAG_RESOLUTION_UNIT:
                if (NestingLevel==0) {// Only use the values from the top level directory
                    ImageInfo.ResolutionUnit = (int) ConvertAnyFormat(ValuePtr,Format);
                }
                break;

        }
    }


    {
        // In addition to linking to subdirectories via exif tags, 
        // there's also a potential link to another directory at the end of each
        // directory.  this has got to be the result of a committee!
        unsigned char * SubdirStart;
        int Offset;

        if (DIR_ENTRY_ADDR(DirStart, NumDirEntries) + 4 <= OffsetBase+ExifLength){
            Offset = Get32u(DirStart+2+12*NumDirEntries);
            if (Offset){
                SubdirStart = OffsetBase + Offset;
                if (SubdirStart > OffsetBase+ExifLength || SubdirStart < OffsetBase){
                    if (SubdirStart > OffsetBase && SubdirStart < OffsetBase+ExifLength+20){
                        // Jhead 1.3 or earlier would crop the whole directory!
                        // As Jhead produces this form of format incorrectness, 
                        // I'll just let it pass silently
                        if (ShowTags) printf("Thumbnail removed with Jhead 1.3 or earlier\n");
                    }else{
                        ErrNonfatal("Illegal subdirectory link in Exif header",0,0);
                    }
                }else{
                    if (SubdirStart+2 <= OffsetBase+ExifLength){
                        if (ShowTags) printf("%s    Continued directory ",IndentString);
                        ProcessExifDir(SubdirStart, OffsetBase, ExifLength, NestingLevel+1);
                    }
                }
                if (Offset > ImageInfo.LargestExifOffset){
                    ImageInfo.LargestExifOffset = Offset;
                }
            }
        }else{
            // The exif header ends before the last next directory pointer.
        }
    }

    if (ThumbnailOffset){
        ImageInfo.ThumbnailAtEnd = FALSE;

        if (DumpExifMap){
            printf("Map: %05d-%05d: Thumbnail\n",ThumbnailOffset, ThumbnailOffset+ThumbnailSize);
        }

        if (ThumbnailOffset <= ExifLength){
            if (ThumbnailSize > ExifLength-ThumbnailOffset){
                // If thumbnail extends past exif header, only save the part that
                // actually exists.  Canon's EOS viewer utility will do this - the
                // thumbnail extracts ok with this hack.
                ThumbnailSize = ExifLength-ThumbnailOffset;
                if (ShowTags) printf("Thumbnail incorrectly placed in header\n");

            }
            // The thumbnail pointer appears to be valid.  Store it.
            ImageInfo.ThumbnailOffset = ThumbnailOffset;
            ImageInfo.ThumbnailSize = ThumbnailSize;

            if (ShowTags){
                printf("Thumbnail size: %u bytes\n",ThumbnailSize);
            }
        }
    }
}