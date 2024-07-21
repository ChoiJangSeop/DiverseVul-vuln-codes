int fullSpeedBench(char** fileNamesTable, int nbFiles)
{
  int fileIdx=0;
  char* orig_buff;
# define NB_COMPRESSION_ALGORITHMS 13
# define MINCOMPRESSIONCHAR '0'
  double totalCTime[NB_COMPRESSION_ALGORITHMS+1] = {0};
  double totalCSize[NB_COMPRESSION_ALGORITHMS+1] = {0};
# define NB_DECOMPRESSION_ALGORITHMS 7
# define MINDECOMPRESSIONCHAR '0'
# define MAXDECOMPRESSIONCHAR (MINDECOMPRESSIONCHAR + NB_DECOMPRESSION_ALGORITHMS)
  static char* decompressionNames[] = { "LZ4_decompress_fast", "LZ4_decompress_fast_withPrefix64k", "LZ4_decompress_fast_usingDict",
                                        "LZ4_decompress_safe", "LZ4_decompress_safe_withPrefix64k", "LZ4_decompress_safe_usingDict", "LZ4_decompress_safe_partial" };
  double totalDTime[NB_DECOMPRESSION_ALGORITHMS+1] = {0};

  U64 totals = 0;


  // Loop for each file
  while (fileIdx<nbFiles)
  {
      FILE* inFile;
      char* inFileName;
      U64   inFileSize;
      size_t benchedSize;
      int nbChunks;
      int maxCompressedChunkSize;
      struct chunkParameters* chunkP;
      size_t readSize;
      char* compressed_buff; int compressedBuffSize;
      U32 crcOriginal;


      // Init
      stateLZ4   = malloc(LZ4_sizeofState());
      stateLZ4HC = malloc(LZ4_sizeofStateHC());

      // Check file existence
      inFileName = fileNamesTable[fileIdx++];
      inFile = fopen( inFileName, "rb" );
      if (inFile==NULL)
      {
        DISPLAY( "Pb opening %s\n", inFileName);
        return 11;
      }

      // Memory allocation & restrictions
      inFileSize = BMK_GetFileSize(inFileName);
      benchedSize = (size_t) BMK_findMaxMem(inFileSize) / 2;
      if ((U64)benchedSize > inFileSize) benchedSize = (size_t)inFileSize;
      if (benchedSize < inFileSize)
      {
          DISPLAY("Not enough memory for '%s' full size; testing %i MB only...\n", inFileName, (int)(benchedSize>>20));
      }

      // Alloc
      chunkP = (struct chunkParameters*) malloc(((benchedSize / (size_t)chunkSize)+1) * sizeof(struct chunkParameters));
      orig_buff = (char*) malloc((size_t)benchedSize);
      nbChunks = (int) ((int)benchedSize / chunkSize) + 1;
      maxCompressedChunkSize = LZ4_compressBound(chunkSize);
      compressedBuffSize = nbChunks * maxCompressedChunkSize;
      compressed_buff = (char*)malloc((size_t)compressedBuffSize);


      if(!orig_buff || !compressed_buff)
      {
        DISPLAY("\nError: not enough memory!\n");
        free(orig_buff);
        free(compressed_buff);
        free(chunkP);
        fclose(inFile);
        return 12;
      }

      // Init chunks data
      {
          int i;
          size_t remaining = benchedSize;
          char* in = orig_buff;
          char* out = compressed_buff;
          for (i=0; i<nbChunks; i++)
          {
              chunkP[i].id = i;
              chunkP[i].origBuffer = in; in += chunkSize;
              if ((int)remaining > chunkSize) { chunkP[i].origSize = chunkSize; remaining -= chunkSize; } else { chunkP[i].origSize = (int)remaining; remaining = 0; }
              chunkP[i].compressedBuffer = out; out += maxCompressedChunkSize;
              chunkP[i].compressedSize = 0;
          }
      }

      // Fill input buffer
      DISPLAY("Loading %s...       \r", inFileName);
      readSize = fread(orig_buff, 1, benchedSize, inFile);
      fclose(inFile);

      if(readSize != benchedSize)
      {
        DISPLAY("\nError: problem reading file '%s' !!    \n", inFileName);
        free(orig_buff);
        free(compressed_buff);
        free(chunkP);
        return 13;
      }

      // Calculating input Checksum
      crcOriginal = XXH32(orig_buff, (unsigned int)benchedSize,0);


      // Bench
      {
        int loopNb, nb_loops, chunkNb, cAlgNb, dAlgNb;
        size_t cSize=0;
        double ratio=0.;

        DISPLAY("\r%79s\r", "");
        DISPLAY(" %s : \n", inFileName);

        // Compression Algorithms
        for (cAlgNb=1; (cAlgNb <= NB_COMPRESSION_ALGORITHMS) && (compressionTest); cAlgNb++)
        {
            char* compressorName;
            int (*compressionFunction)(const char*, char*, int);
            void* (*initFunction)(const char*) = NULL;
            double bestTime = 100000000.;

            if ((compressionAlgo != ALL_COMPRESSORS) && (compressionAlgo != cAlgNb)) continue;

            switch(cAlgNb)
            {
            case 1 : compressionFunction = LZ4_compress; compressorName = "LZ4_compress"; break;
            case 2 : compressionFunction = local_LZ4_compress_limitedOutput; compressorName = "LZ4_compress_limitedOutput"; break;
            case 3 : compressionFunction = local_LZ4_compress_withState; compressorName = "LZ4_compress_withState"; break;
            case 4 : compressionFunction = local_LZ4_compress_limitedOutput_withState; compressorName = "LZ4_compress_limitedOutput_withState"; break;
            case 5 : compressionFunction = local_LZ4_compress_continue; initFunction = LZ4_create; compressorName = "LZ4_compress_continue"; break;
            case 6 : compressionFunction = local_LZ4_compress_limitedOutput_continue; initFunction = LZ4_create; compressorName = "LZ4_compress_limitedOutput_continue"; break;
            case 7 : compressionFunction = LZ4_compressHC; compressorName = "LZ4_compressHC"; break;
            case 8 : compressionFunction = local_LZ4_compressHC_limitedOutput; compressorName = "LZ4_compressHC_limitedOutput"; break;
            case 9 : compressionFunction = local_LZ4_compressHC_withStateHC; compressorName = "LZ4_compressHC_withStateHC"; break;
            case 10: compressionFunction = local_LZ4_compressHC_limitedOutput_withStateHC; compressorName = "LZ4_compressHC_limitedOutput_withStateHC"; break;
            case 11: compressionFunction = local_LZ4_compressHC_continue; initFunction = LZ4_createHC; compressorName = "LZ4_compressHC_continue"; break;
            case 12: compressionFunction = local_LZ4_compressHC_limitedOutput_continue; initFunction = LZ4_createHC; compressorName = "LZ4_compressHC_limitedOutput_continue"; break;
            case 13: compressionFunction = local_LZ4_compress_forceDict; initFunction = local_LZ4_resetDictT; compressorName = "LZ4_compress_forceDict"; break;
            default : DISPLAY("ERROR ! Bad algorithm Id !! \n"); free(chunkP); return 1;
            }

            for (loopNb = 1; loopNb <= nbIterations; loopNb++)
            {
                double averageTime;
                int milliTime;

                PROGRESS("%1i-%-26.26s : %9i ->\r", loopNb, compressorName, (int)benchedSize);
                { size_t i; for (i=0; i<benchedSize; i++) compressed_buff[i]=(char)i; }     // warmimg up memory

                nb_loops = 0;
                milliTime = BMK_GetMilliStart();
                while(BMK_GetMilliStart() == milliTime);
                milliTime = BMK_GetMilliStart();
                while(BMK_GetMilliSpan(milliTime) < TIMELOOP)
                {
                    if (initFunction!=NULL) ctx = initFunction(chunkP[0].origBuffer);
                    for (chunkNb=0; chunkNb<nbChunks; chunkNb++)
                    {
                        chunkP[chunkNb].compressedSize = compressionFunction(chunkP[chunkNb].origBuffer, chunkP[chunkNb].compressedBuffer, chunkP[chunkNb].origSize);
                        if (chunkP[chunkNb].compressedSize==0) DISPLAY("ERROR ! %s() = 0 !! \n", compressorName), exit(1);
                    }
                    if (initFunction!=NULL) free(ctx);
                    nb_loops++;
                }
                milliTime = BMK_GetMilliSpan(milliTime);

                averageTime = (double)milliTime / nb_loops;
                if (averageTime < bestTime) bestTime = averageTime;
                cSize=0; for (chunkNb=0; chunkNb<nbChunks; chunkNb++) cSize += chunkP[chunkNb].compressedSize;
                ratio = (double)cSize/(double)benchedSize*100.;
                PROGRESS("%1i-%-26.26s : %9i -> %9i (%5.2f%%),%7.1f MB/s\r", loopNb, compressorName, (int)benchedSize, (int)cSize, ratio, (double)benchedSize / bestTime / 1000.);
            }

            if (ratio<100.)
                DISPLAY("%-28.28s : %9i -> %9i (%5.2f%%),%7.1f MB/s\n", compressorName, (int)benchedSize, (int)cSize, ratio, (double)benchedSize / bestTime / 1000.);
            else
                DISPLAY("%-28.28s : %9i -> %9i (%5.1f%%),%7.1f MB/s\n", compressorName, (int)benchedSize, (int)cSize, ratio, (double)benchedSize / bestTime / 1000.);

            totalCTime[cAlgNb] += bestTime;
            totalCSize[cAlgNb] += cSize;
        }

        // Prepare layout for decompression
        for (chunkNb=0; chunkNb<nbChunks; chunkNb++)
        {
            chunkP[chunkNb].compressedSize = LZ4_compress(chunkP[chunkNb].origBuffer, chunkP[chunkNb].compressedBuffer, chunkP[chunkNb].origSize);
            if (chunkP[chunkNb].compressedSize==0) DISPLAY("ERROR ! %s() = 0 !! \n", "LZ4_compress"), exit(1);
        }
        { size_t i; for (i=0; i<benchedSize; i++) orig_buff[i]=0; }     // zeroing source area, for CRC checking

        // Decompression Algorithms
        for (dAlgNb=0; (dAlgNb < NB_DECOMPRESSION_ALGORITHMS) && (decompressionTest); dAlgNb++)
        {
            char* dName = decompressionNames[dAlgNb];
            int (*decompressionFunction)(const char*, char*, int, int);
            double bestTime = 100000000.;

            if ((decompressionAlgo != ALL_DECOMPRESSORS) && (decompressionAlgo != dAlgNb+1)) continue;

            switch(dAlgNb)
            {
            case 0: decompressionFunction = local_LZ4_decompress_fast; break;
            case 1: decompressionFunction = local_LZ4_decompress_fast_withPrefix64k; break;
            case 2: decompressionFunction = local_LZ4_decompress_fast_usingDict; break;
            case 3: decompressionFunction = LZ4_decompress_safe; break;
            case 4: decompressionFunction = LZ4_decompress_safe_withPrefix64k; break;
            case 5: decompressionFunction = local_LZ4_decompress_safe_usingDict; break;
            case 6: decompressionFunction = local_LZ4_decompress_safe_partial; break;
            default : DISPLAY("ERROR ! Bad decompression algorithm Id !! \n"); free(chunkP); return 1;
            }

            for (loopNb = 1; loopNb <= nbIterations; loopNb++)
            {
                double averageTime;
                int milliTime;
                U32 crcDecoded;

                PROGRESS("%1i-%-29.29s :%10i ->\r", loopNb, dName, (int)benchedSize);

                nb_loops = 0;
                milliTime = BMK_GetMilliStart();
                while(BMK_GetMilliStart() == milliTime);
                milliTime = BMK_GetMilliStart();
                while(BMK_GetMilliSpan(milliTime) < TIMELOOP)
                {
                    for (chunkNb=0; chunkNb<nbChunks; chunkNb++)
                    {
                        int decodedSize = decompressionFunction(chunkP[chunkNb].compressedBuffer, chunkP[chunkNb].origBuffer, chunkP[chunkNb].compressedSize, chunkP[chunkNb].origSize);
                        if (chunkP[chunkNb].origSize != decodedSize) DISPLAY("ERROR ! %s() == %i != %i !! \n", dName, decodedSize, chunkP[chunkNb].origSize), exit(1);
                    }
                    nb_loops++;
                }
                milliTime = BMK_GetMilliSpan(milliTime);

                averageTime = (double)milliTime / nb_loops;
                if (averageTime < bestTime) bestTime = averageTime;

                PROGRESS("%1i-%-29.29s :%10i -> %7.1f MB/s\r", loopNb, dName, (int)benchedSize, (double)benchedSize / bestTime / 1000.);

                // CRC Checking
                crcDecoded = XXH32(orig_buff, (int)benchedSize, 0);
                if (crcOriginal!=crcDecoded) { DISPLAY("\n!!! WARNING !!! %14s : Invalid Checksum : %x != %x\n", inFileName, (unsigned)crcOriginal, (unsigned)crcDecoded); exit(1); }
            }

            DISPLAY("%-31.31s :%10i -> %7.1f MB/s\n", dName, (int)benchedSize, (double)benchedSize / bestTime / 1000.);

            totalDTime[dAlgNb] += bestTime;
        }

        totals += benchedSize;
      }

      free(orig_buff);
      free(compressed_buff);
      free(chunkP);
  }

  if (BMK_pause) { printf("press enter...\n"); getchar(); }

  return 0;
}