void WavOutFile::write(const short *buffer, int numElems)
{
    int res;

    // 16 bit samples
    if (numElems < 1) return;   // nothing to do

    switch (header.format.bits_per_sample)
    {
        case 8:
        {
            int i;
            unsigned char *temp = (unsigned char *)getConvBuffer(numElems);
            // convert from 16bit format to 8bit format
            for (i = 0; i < numElems; i ++)
            {
                temp[i] = (unsigned char)(buffer[i] / 256 + 128);
            }
            // write in 8bit format
            write(temp, numElems);
            break;
        }

        case 16:
        {
            // 16bit format

            // use temp buffer to swap byte order if necessary
            short *pTemp = (short *)getConvBuffer(numElems * sizeof(short));
            memcpy(pTemp, buffer, numElems * 2);
            _swap16Buffer(pTemp, numElems);

            res = (int)fwrite(pTemp, 2, numElems, fptr);

            if (res != numElems) 
            {
                ST_THROW_RT_ERROR("Error while writing to a wav file.");
            }
            bytesWritten += 2 * numElems;
            break;
        }

        default:
        {
            stringstream ss;
            ss << "\nOnly 8/16 bit sample WAV files supported in integer compilation. Can't open WAV file with ";
            ss << (int)header.format.bits_per_sample;
            ss << " bit sample format. ";
            ST_THROW_RT_ERROR(ss.str().c_str());
        }
    }
}