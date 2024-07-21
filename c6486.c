int WavInFile::readHeaderBlock()
{
    char label[5];
    string sLabel;

    // lead label string
    if (fread(label, 1, 4, fptr) !=4) return -1;
    label[4] = 0;

    if (isAlphaStr(label) == 0) return -1;    // not a valid label

    // Decode blocks according to their label
    if (strcmp(label, fmtStr) == 0)
    {
        int nLen, nDump;

        // 'fmt ' block 
        memcpy(header.format.fmt, fmtStr, 4);

        // read length of the format field
        if (fread(&nLen, sizeof(int), 1, fptr) != 1) return -1;
        // swap byte order if necessary
        _swap32(nLen); // int format_len;
        header.format.format_len = nLen;

        // calculate how much length differs from expected
        nDump = nLen - ((int)sizeof(header.format) - 8);

        // if format_len is larger than expected, read only as much data as we've space for
        if (nDump > 0)
        {
            nLen = sizeof(header.format) - 8;
        }

        // read data
        if (fread(&(header.format.fixed), nLen, 1, fptr) != 1) return -1;

        // swap byte order if necessary
        _swap16((short &)header.format.fixed);            // short int fixed;
        _swap16((short &)header.format.channel_number);   // short int channel_number;
        _swap32((int &)header.format.sample_rate);        // int sample_rate;
        _swap32((int &)header.format.byte_rate);          // int byte_rate;
        _swap16((short &)header.format.byte_per_sample);  // short int byte_per_sample;
        _swap16((short &)header.format.bits_per_sample);  // short int bits_per_sample;

        // if format_len is larger than expected, skip the extra data
        if (nDump > 0)
        {
            fseek(fptr, nDump, SEEK_CUR);
        }

        return 0;
    }
    else if (strcmp(label, factStr) == 0)
    {
        int nLen, nDump;

        // 'fact' block 
        memcpy(header.fact.fact_field, factStr, 4);

        // read length of the fact field
        if (fread(&nLen, sizeof(int), 1, fptr) != 1) return -1;
        // swap byte order if necessary
        _swap32(nLen); // int fact_len;
        header.fact.fact_len = nLen;

        // calculate how much length differs from expected
        nDump = nLen - ((int)sizeof(header.fact) - 8);

        // if format_len is larger than expected, read only as much data as we've space for
        if (nDump > 0)
        {
            nLen = sizeof(header.fact) - 8;
        }

        // read data
        if (fread(&(header.fact.fact_sample_len), nLen, 1, fptr) != 1) return -1;

        // swap byte order if necessary
        _swap32((int &)header.fact.fact_sample_len);    // int sample_length;

        // if fact_len is larger than expected, skip the extra data
        if (nDump > 0)
        {
            fseek(fptr, nDump, SEEK_CUR);
        }

        return 0;
    }
    else if (strcmp(label, dataStr) == 0)
    {
        // 'data' block
        memcpy(header.data.data_field, dataStr, 4);
        if (fread(&(header.data.data_len), sizeof(uint), 1, fptr) != 1) return -1;

        // swap byte order if necessary
        _swap32((int &)header.data.data_len);

        return 1;
    }
    else
    {
        uint len, i;
        uint temp;
        // unknown block

        // read length
        if (fread(&len, sizeof(len), 1, fptr) != 1) return -1;
        // scan through the block
        for (i = 0; i < len; i ++)
        {
            if (fread(&temp, 1, 1, fptr) != 1) return -1;
            if (feof(fptr)) return -1;   // unexpected eof
        }
    }
    return 0;
}