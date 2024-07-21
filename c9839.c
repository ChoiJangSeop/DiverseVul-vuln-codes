static void encode_help_display(void)
{
    fprintf(stdout,
            "\nThis is the opj_compress utility from the OpenJPEG project.\n"
            "It compresses various image formats with the JPEG 2000 algorithm.\n"
            "It has been compiled against openjp2 library v%s.\n\n", opj_version());

    fprintf(stdout, "Default encoding options:\n");
    fprintf(stdout, "-------------------------\n");
    fprintf(stdout, "\n");
    fprintf(stdout, " * Lossless\n");
    fprintf(stdout, " * 1 tile\n");
    fprintf(stdout, " * RGB->YCC conversion if at least 3 components\n");
    fprintf(stdout, " * Size of precinct : 2^15 x 2^15 (means 1 precinct)\n");
    fprintf(stdout, " * Size of code-block : 64 x 64\n");
    fprintf(stdout, " * Number of resolutions: 6\n");
    fprintf(stdout, " * No SOP marker in the codestream\n");
    fprintf(stdout, " * No EPH marker in the codestream\n");
    fprintf(stdout, " * No sub-sampling in x or y direction\n");
    fprintf(stdout, " * No mode switch activated\n");
    fprintf(stdout, " * Progression order: LRCP\n");
#ifdef FIXME_INDEX
    fprintf(stdout, " * No index file\n");
#endif /* FIXME_INDEX */
    fprintf(stdout, " * No ROI upshifted\n");
    fprintf(stdout, " * No offset of the origin of the image\n");
    fprintf(stdout, " * No offset of the origin of the tiles\n");
    fprintf(stdout, " * Reversible DWT 5-3\n");
    /* UniPG>> */
#ifdef USE_JPWL
    fprintf(stdout, " * No JPWL protection\n");
#endif /* USE_JPWL */
    /* <<UniPG */
    fprintf(stdout, "\n");

    fprintf(stdout, "Note:\n");
    fprintf(stdout, "-----\n");
    fprintf(stdout, "\n");
    fprintf(stdout,
            "The markers written to the main_header are : SOC SIZ COD QCD COM.\n");
    fprintf(stdout, "COD and QCD never appear in the tile_header.\n");
    fprintf(stdout, "\n");

    fprintf(stdout, "Parameters:\n");
    fprintf(stdout, "-----------\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "Required Parameters (except with -h):\n");
    fprintf(stdout, "One of the two options -ImgDir or -i must be used\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "-i <file>\n");
    fprintf(stdout, "    Input file\n");
    fprintf(stdout,
            "    Known extensions are <PBM|PGM|PPM|PNM|PAM|PGX|PNG|BMP|TIF|RAW|RAWL|TGA>\n");
    fprintf(stdout, "    If used, '-o <file>' must be provided\n");
    fprintf(stdout, "-o <compressed file>\n");
    fprintf(stdout, "    Output file (accepted extensions are j2k or jp2).\n");
    fprintf(stdout, "-ImgDir <dir>\n");
    fprintf(stdout, "    Image file Directory path (example ../Images) \n");
    fprintf(stdout, "    When using this option -OutFor must be used\n");
    fprintf(stdout, "-OutFor <J2K|J2C|JP2>\n");
    fprintf(stdout, "    Output format for compressed files.\n");
    fprintf(stdout, "    Required only if -ImgDir is used\n");
    fprintf(stdout,
            "-F <width>,<height>,<ncomp>,<bitdepth>,{s,u}@<dx1>x<dy1>:...:<dxn>x<dyn>\n");
    fprintf(stdout, "    Characteristics of the raw input image\n");
    fprintf(stdout,
            "    If subsampling is omitted, 1x1 is assumed for all components\n");
    fprintf(stdout, "      Example: -F 512,512,3,8,u@1x1:2x2:2x2\n");
    fprintf(stdout,
            "               for raw 512x512 image with 4:2:0 subsampling\n");
    fprintf(stdout, "    Required only if RAW or RAWL input file is provided.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "Optional Parameters:\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "-h\n");
    fprintf(stdout, "    Display the help information.\n");
    fprintf(stdout, "-r <compression ratio>,<compression ratio>,...\n");
    fprintf(stdout, "    Different compression ratios for successive layers.\n");
    fprintf(stdout,
            "    The rate specified for each quality level is the desired\n");
    fprintf(stdout, "    compression factor (use 1 for lossless)\n");
    fprintf(stdout, "    Decreasing ratios required.\n");
    fprintf(stdout, "      Example: -r 20,10,1 means \n");
    fprintf(stdout, "            quality layer 1: compress 20x, \n");
    fprintf(stdout, "            quality layer 2: compress 10x \n");
    fprintf(stdout, "            quality layer 3: compress lossless\n");
    fprintf(stdout, "    Options -r and -q cannot be used together.\n");
    fprintf(stdout, "-q <psnr value>,<psnr value>,<psnr value>,...\n");
    fprintf(stdout, "    Different psnr for successive layers (-q 30,40,50).\n");
    fprintf(stdout, "    Increasing PSNR values required, except 0 which can\n");
    fprintf(stdout, "    be used for the last layer to indicate it is lossless.\n");
    fprintf(stdout, "    Options -r and -q cannot be used together.\n");
    fprintf(stdout, "-n <number of resolutions>\n");
    fprintf(stdout, "    Number of resolutions.\n");
    fprintf(stdout,
            "    It corresponds to the number of DWT decompositions +1. \n");
    fprintf(stdout, "    Default: 6.\n");
    fprintf(stdout, "-b <cblk width>,<cblk height>\n");
    fprintf(stdout,
            "    Code-block size. The dimension must respect the constraint \n");
    fprintf(stdout,
            "    defined in the JPEG-2000 standard (no dimension smaller than 4 \n");
    fprintf(stdout,
            "    or greater than 1024, no code-block with more than 4096 coefficients).\n");
    fprintf(stdout, "    The maximum value authorized is 64x64. \n");
    fprintf(stdout, "    Default: 64x64.\n");
    fprintf(stdout,
            "-c [<prec width>,<prec height>],[<prec width>,<prec height>],...\n");
    fprintf(stdout, "    Precinct size. Values specified must be power of 2. \n");
    fprintf(stdout,
            "    Multiple records may be supplied, in which case the first record refers\n");
    fprintf(stdout,
            "    to the highest resolution level and subsequent records to lower \n");
    fprintf(stdout,
            "    resolution levels. The last specified record is halved successively for each \n");
    fprintf(stdout, "    remaining lower resolution levels.\n");
    fprintf(stdout, "    Default: 2^15x2^15 at each resolution.\n");
    fprintf(stdout, "-t <tile width>,<tile height>\n");
    fprintf(stdout, "    Tile size.\n");
    fprintf(stdout,
            "    Default: the dimension of the whole image, thus only one tile.\n");
    fprintf(stdout, "-p <LRCP|RLCP|RPCL|PCRL|CPRL>\n");
    fprintf(stdout, "    Progression order.\n");
    fprintf(stdout, "    Default: LRCP.\n");
    fprintf(stdout, "-s  <subX,subY>\n");
    fprintf(stdout, "    Subsampling factor.\n");
    fprintf(stdout, "    Subsampling bigger than 2 can produce error\n");
    fprintf(stdout, "    Default: no subsampling.\n");
    fprintf(stdout,
            "-POC <progression order change>/<progression order change>/...\n");
    fprintf(stdout, "    Progression order change.\n");
    fprintf(stdout,
            "    The syntax of a progression order change is the following:\n");
    fprintf(stdout,
            "    T<tile>=<resStart>,<compStart>,<layerEnd>,<resEnd>,<compEnd>,<progOrder>\n");
    fprintf(stdout, "      Example: -POC T1=0,0,1,5,3,CPRL/T1=5,0,1,6,3,CPRL\n");
    fprintf(stdout, "-SOP\n");
    fprintf(stdout, "    Write SOP marker before each packet.\n");
    fprintf(stdout, "-EPH\n");
    fprintf(stdout, "    Write EPH marker after each header packet.\n");
    fprintf(stdout, "-M <key value>\n");
    fprintf(stdout, "    Mode switch.\n");
    fprintf(stdout, "    [1=BYPASS(LAZY) 2=RESET 4=RESTART(TERMALL)\n");
    fprintf(stdout, "    8=VSC 16=ERTERM(SEGTERM) 32=SEGMARK(SEGSYM)]\n");
    fprintf(stdout, "    Indicate multiple modes by adding their values.\n");
    fprintf(stdout,
            "      Example: RESTART(4) + RESET(2) + SEGMARK(32) => -M 38\n");
    fprintf(stdout, "-TP <R|L|C>\n");
    fprintf(stdout, "    Divide packets of every tile into tile-parts.\n");
    fprintf(stdout,
            "    Division is made by grouping Resolutions (R), Layers (L)\n");
    fprintf(stdout, "    or Components (C).\n");
#ifdef FIXME_INDEX
    fprintf(stdout, "-x  <index file>\n");
    fprintf(stdout, "    Create an index file.\n");
#endif /*FIXME_INDEX*/
    fprintf(stdout, "-ROI c=<component index>,U=<upshifting value>\n");
    fprintf(stdout, "    Quantization indices upshifted for a component. \n");
    fprintf(stdout,
            "    Warning: This option does not implement the usual ROI (Region of Interest).\n");
    fprintf(stdout,
            "    It should be understood as a 'Component of Interest'. It offers the \n");
    fprintf(stdout,
            "    possibility to upshift the value of a component during quantization step.\n");
    fprintf(stdout,
            "    The value after c= is the component number [0, 1, 2, ...] and the value \n");
    fprintf(stdout,
            "    after U= is the value of upshifting. U must be in the range [0, 37].\n");
    fprintf(stdout, "-d <image offset X,image offset Y>\n");
    fprintf(stdout, "    Offset of the origin of the image.\n");
    fprintf(stdout, "-T <tile offset X,tile offset Y>\n");
    fprintf(stdout, "    Offset of the origin of the tiles.\n");
    fprintf(stdout, "-I\n");
    fprintf(stdout, "    Use the irreversible DWT 9-7.\n");
    fprintf(stdout, "-mct <0|1|2>\n");
    fprintf(stdout,
            "    Explicitly specifies if a Multiple Component Transform has to be used.\n");
    fprintf(stdout, "    0: no MCT ; 1: RGB->YCC conversion ; 2: custom MCT.\n");
    fprintf(stdout,
            "    If custom MCT, \"-m\" option has to be used (see hereunder).\n");
    fprintf(stdout,
            "    By default, RGB->YCC conversion is used if there are 3 components or more,\n");
    fprintf(stdout, "    no conversion otherwise.\n");
    fprintf(stdout, "-m <file>\n");
    fprintf(stdout,
            "    Use array-based MCT, values are coma separated, line by line\n");
    fprintf(stdout,
            "    No specific separators between lines, no space allowed between values.\n");
    fprintf(stdout,
            "    If this option is used, it automatically sets \"-mct\" option to 2.\n");
    fprintf(stdout, "-cinema2K <24|48>\n");
    fprintf(stdout, "    Digital Cinema 2K profile compliant codestream.\n");
    fprintf(stdout,
            "	Need to specify the frames per second for a 2K resolution.\n");
    fprintf(stdout, "    Only 24 or 48 fps are currently allowed.\n");
    fprintf(stdout, "-cinema4K\n");
    fprintf(stdout, "    Digital Cinema 4K profile compliant codestream.\n");
    fprintf(stdout, "	Frames per second not required. Default value is 24fps.\n");
    fprintf(stdout, "-IMF <PROFILE>[,mainlevel=X][,sublevel=Y][,framerate=FPS]\n");
    fprintf(stdout, "    Interoperable Master Format compliant codestream.\n");
    fprintf(stdout, "    <PROFILE>=2K, 4K, 8K, 2K_R, 4K_R or 8K_R.\n");
    fprintf(stdout, "    X >= 0 and X <= 11.\n");
    fprintf(stdout, "    Y >= 0 and Y <= 9.\n");
    fprintf(stdout,
            "    framerate > 0 may be specified to enhance checks and set maximum bit rate when Y > 0.\n");
    fprintf(stdout, "-jpip\n");
    fprintf(stdout, "    Write jpip codestream index box in JP2 output file.\n");
    fprintf(stdout, "    Currently supports only RPCL order.\n");
    fprintf(stdout, "-C <comment>\n");
    fprintf(stdout, "    Add <comment> in the comment marker segment.\n");
    /* UniPG>> */
#ifdef USE_JPWL
    fprintf(stdout, "-W <params>\n");
    fprintf(stdout, "    Adoption of JPWL (Part 11) capabilities (-W params)\n");
    fprintf(stdout,
            "    The <params> field can be written and repeated in any order:\n");
    fprintf(stdout, "    [h<tilepart><=type>,s<tilepart><=method>,a=<addr>,...\n");
    fprintf(stdout, "    ...,z=<size>,g=<range>,p<tilepart:pack><=type>]\n");
    fprintf(stdout,
            "     h selects the header error protection (EPB): 'type' can be\n");
    fprintf(stdout,
            "       [0=none 1,absent=predefined 16=CRC-16 32=CRC-32 37-128=RS]\n");
    fprintf(stdout,
            "       if 'tilepart' is absent, it is for main and tile headers\n");
    fprintf(stdout, "       if 'tilepart' is present, it applies from that tile\n");
    fprintf(stdout,
            "         onwards, up to the next h<> spec, or to the last tilepart\n");
    fprintf(stdout, "         in the codestream (max. %d specs)\n",
            JPWL_MAX_NO_TILESPECS);
    fprintf(stdout,
            "     p selects the packet error protection (EEP/UEP with EPBs)\n");
    fprintf(stdout, "      to be applied to raw data: 'type' can be\n");
    fprintf(stdout,
            "       [0=none 1,absent=predefined 16=CRC-16 32=CRC-32 37-128=RS]\n");
    fprintf(stdout,
            "       if 'tilepart:pack' is absent, it is from tile 0, packet 0\n");
    fprintf(stdout,
            "       if 'tilepart:pack' is present, it applies from that tile\n");
    fprintf(stdout,
            "         and that packet onwards, up to the next packet spec\n");
    fprintf(stdout,
            "         or to the last packet in the last tilepart in the stream\n");
    fprintf(stdout, "         (max. %d specs)\n", JPWL_MAX_NO_PACKSPECS);
    fprintf(stdout,
            "     s enables sensitivity data insertion (ESD): 'method' can be\n");
    fprintf(stdout,
            "       [-1=NO ESD 0=RELATIVE ERROR 1=MSE 2=MSE REDUCTION 3=PSNR\n");
    fprintf(stdout, "        4=PSNR INCREMENT 5=MAXERR 6=TSE 7=RESERVED]\n");
    fprintf(stdout, "       if 'tilepart' is absent, it is for main header only\n");
    fprintf(stdout, "       if 'tilepart' is present, it applies from that tile\n");
    fprintf(stdout,
            "         onwards, up to the next s<> spec, or to the last tilepart\n");
    fprintf(stdout, "         in the codestream (max. %d specs)\n",
            JPWL_MAX_NO_TILESPECS);
    fprintf(stdout, "     g determines the addressing mode: <range> can be\n");
    fprintf(stdout, "       [0=PACKET 1=BYTE RANGE 2=PACKET RANGE]\n");
    fprintf(stdout,
            "     a determines the size of data addressing: <addr> can be\n");
    fprintf(stdout,
            "       2/4 bytes (small/large codestreams). If not set, auto-mode\n");
    fprintf(stdout,
            "     z determines the size of sensitivity values: <size> can be\n");
    fprintf(stdout,
            "       1/2 bytes, for the transformed pseudo-floating point value\n");
    fprintf(stdout, "     ex.:\n");
    fprintf(stdout,
            "       h,h0=64,h3=16,h5=32,p0=78,p0:24=56,p1,p3:0=0,p3:20=32,s=0,\n");
    fprintf(stdout, "         s0=6,s3=-1,a=0,g=1,z=1\n");
    fprintf(stdout, "     means\n");
    fprintf(stdout,
            "       predefined EPB in MH, rs(64,32) from TPH 0 to TPH 2,\n");
    fprintf(stdout,
            "       CRC-16 in TPH 3 and TPH 4, CRC-32 in remaining TPHs,\n");
    fprintf(stdout, "       UEP rs(78,32) for packets 0 to 23 of tile 0,\n");
    fprintf(stdout,
            "       UEP rs(56,32) for packs. 24 to the last of tilepart 0,\n");
    fprintf(stdout, "       UEP rs default for packets of tilepart 1,\n");
    fprintf(stdout, "       no UEP for packets 0 to 19 of tilepart 3,\n");
    fprintf(stdout,
            "       UEP CRC-32 for packs. 20 of tilepart 3 to last tilepart,\n");
    fprintf(stdout, "       relative sensitivity ESD for MH,\n");
    fprintf(stdout,
            "       TSE ESD from TPH 0 to TPH 2, byte range with automatic\n");
    fprintf(stdout,
            "       size of addresses and 1 byte for each sensitivity value\n");
    fprintf(stdout, "     ex.:\n");
    fprintf(stdout, "           h,s,p\n");
    fprintf(stdout, "     means\n");
    fprintf(stdout,
            "       default protection to headers (MH and TPHs) as well as\n");
    fprintf(stdout, "       data packets, one ESD in MH\n");
    fprintf(stdout,
            "     N.B.: use the following recommendations when specifying\n");
    fprintf(stdout, "           the JPWL parameters list\n");
    fprintf(stdout,
            "       - when you use UEP, always pair the 'p' option with 'h'\n");
#endif /* USE_JPWL */
    /* <<UniPG */
    fprintf(stdout, "\n");
#ifdef FIXME_INDEX
    fprintf(stdout, "Index structure:\n");
    fprintf(stdout, "----------------\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "Image_height Image_width\n");
    fprintf(stdout, "progression order\n");
    fprintf(stdout, "Tiles_size_X Tiles_size_Y\n");
    fprintf(stdout, "Tiles_nb_X Tiles_nb_Y\n");
    fprintf(stdout, "Components_nb\n");
    fprintf(stdout, "Layers_nb\n");
    fprintf(stdout, "decomposition_levels\n");
    fprintf(stdout, "[Precincts_size_X_res_Nr Precincts_size_Y_res_Nr]...\n");
    fprintf(stdout, "   [Precincts_size_X_res_0 Precincts_size_Y_res_0]\n");
    fprintf(stdout, "Main_header_start_position\n");
    fprintf(stdout, "Main_header_end_position\n");
    fprintf(stdout, "Codestream_size\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "INFO ON TILES\n");
    fprintf(stdout,
            "tileno start_pos end_hd end_tile nbparts disto nbpix disto/nbpix\n");
    fprintf(stdout,
            "Tile_0 start_pos end_Theader end_pos NumParts TotalDisto NumPix MaxMSE\n");
    fprintf(stdout,
            "Tile_1   ''           ''        ''        ''       ''    ''      ''\n");
    fprintf(stdout, "...\n");
    fprintf(stdout,
            "Tile_Nt   ''           ''        ''        ''       ''    ''     ''\n");
    fprintf(stdout, "...\n");
    fprintf(stdout, "TILE 0 DETAILS\n");
    fprintf(stdout, "part_nb tileno num_packs start_pos end_tph_pos end_pos\n");
    fprintf(stdout, "...\n");
    fprintf(stdout, "Progression_string\n");
    fprintf(stdout,
            "pack_nb tileno layno resno compno precno start_pos end_ph_pos end_pos disto\n");
    fprintf(stdout,
            "Tpacket_0 Tile layer res. comp. prec. start_pos end_pos disto\n");
    fprintf(stdout, "...\n");
    fprintf(stdout,
            "Tpacket_Np ''   ''    ''   ''    ''       ''       ''     ''\n");
    fprintf(stdout, "MaxDisto\n");
    fprintf(stdout, "TotalDisto\n\n");
#endif /*FIXME_INDEX*/
}