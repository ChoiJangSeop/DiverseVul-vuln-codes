FORCE_INLINE int LZ4_decompress_generic(
                 const char* source,
                 char* dest,
                 int inputSize,
                 int outputSize,         /* If endOnInput==endOnInputSize, this value is the max size of Output Buffer. */

                 int endOnInput,         /* endOnOutputSize, endOnInputSize */
                 int partialDecoding,    /* full, partial */
                 int targetOutputSize,   /* only used if partialDecoding==partial */
                 int dict,               /* noDict, withPrefix64k, usingExtDict */
                 const char* dictStart,  /* only if dict==usingExtDict */
                 int dictSize            /* note : = 0 if noDict */
                 )
{
    /* Local Variables */
    const BYTE* restrict ip = (const BYTE*) source;
    const BYTE* ref;
    const BYTE* const iend = ip + inputSize;

    BYTE* op = (BYTE*) dest;
    BYTE* const oend = op + outputSize;
    BYTE* cpy;
    BYTE* oexit = op + targetOutputSize;
    const BYTE* const lowLimit = (const BYTE*)dest - dictSize;

    const BYTE* const dictEnd = (const BYTE*)dictStart + dictSize;
//#define OLD
#ifdef OLD
    const size_t dec32table[] = {0, 3, 2, 3, 0, 0, 0, 0};   /* static reduces speed for LZ4_decompress_safe() on GCC64 */
#else
    const size_t dec32table[] = {4-0, 4-3, 4-2, 4-3, 4-0, 4-0, 4-0, 4-0};   /* static reduces speed for LZ4_decompress_safe() on GCC64 */
#endif
    static const size_t dec64table[] = {0, 0, 0, (size_t)-1, 0, 1, 2, 3};

    const int checkOffset = (endOnInput) && (dictSize < (int)(64 KB));


    /* Special cases */
    if ((partialDecoding) && (oexit> oend-MFLIMIT)) oexit = oend-MFLIMIT;                        /* targetOutputSize too high => decode everything */
    if ((endOnInput) && (unlikely(outputSize==0))) return ((inputSize==1) && (*ip==0)) ? 0 : -1;   /* Empty output buffer */
    if ((!endOnInput) && (unlikely(outputSize==0))) return (*ip==0?1:-1);


    /* Main Loop */
    while (1)
    {
        unsigned token;
        size_t length;

        /* get runlength */
        token = *ip++;
        if ((length=(token>>ML_BITS)) == RUN_MASK)
        {
            unsigned s=255;
            while (((endOnInput)?ip<iend:1) && (s==255))
            {
                s = *ip++;
                length += s;
            }
        }

        /* copy literals */
        cpy = op+length;
        if (((endOnInput) && ((cpy>(partialDecoding?oexit:oend-MFLIMIT)) || (ip+length>iend-(2+1+LASTLITERALS))) )
            || ((!endOnInput) && (cpy>oend-COPYLENGTH)))
        {
            if (partialDecoding)
            {
                if (cpy > oend) goto _output_error;                           /* Error : write attempt beyond end of output buffer */
                if ((endOnInput) && (ip+length > iend)) goto _output_error;   /* Error : read attempt beyond end of input buffer */
            }
            else
            {
                if ((!endOnInput) && (cpy != oend)) goto _output_error;       /* Error : block decoding must stop exactly there */
                if ((endOnInput) && ((ip+length != iend) || (cpy > oend))) goto _output_error;   /* Error : input must be consumed */
            }
            memcpy(op, ip, length);
            ip += length;
            op += length;
            break;                                       /* Necessarily EOF, due to parsing restrictions */
        }
        LZ4_WILDCOPY(op, ip, cpy); ip -= (op-cpy); op = cpy;

        /* get offset */
        LZ4_READ_LITTLEENDIAN_16(ref,cpy,ip); ip+=2;
        if ((checkOffset) && (unlikely(ref < lowLimit))) goto _output_error;   /* Error : offset outside destination buffer */

        /* get matchlength */
        if ((length=(token&ML_MASK)) == ML_MASK)
        {
            unsigned s;
            do
            {
                if (endOnInput && (ip > iend-LASTLITERALS)) goto _output_error;
                s = *ip++;
                length += s;
            } while (s==255);
        }

        /* check external dictionary */
        if ((dict==usingExtDict) && (ref < (BYTE* const)dest))
        {
            if (unlikely(op+length+MINMATCH > oend-LASTLITERALS)) goto _output_error;

            if (length+MINMATCH <= (size_t)(dest-(char*)ref))
            {
                ref = dictEnd - (dest-(char*)ref);
                memcpy(op, ref, length+MINMATCH);
                op += length+MINMATCH;
            }
            else
            {
                size_t copySize = (size_t)(dest-(char*)ref);
                memcpy(op, dictEnd - copySize, copySize);
                op += copySize;
                copySize = length+MINMATCH - copySize;
                if (copySize > (size_t)((char*)op-dest))   /* overlap */
                {
                    BYTE* const cpy = op + copySize;
                    const BYTE* ref = (BYTE*)dest;
                    while (op < cpy) *op++ = *ref++;
                }
                else
                {
                    memcpy(op, dest, copySize);
                    op += copySize;
                }
            }
            continue;
        }

        /* copy repeated sequence */
        if (unlikely((op-ref)<(int)STEPSIZE))
        {
            const size_t dec64 = dec64table[(sizeof(void*)==4) ? 0 : op-ref];
            op[0] = ref[0];
            op[1] = ref[1];
            op[2] = ref[2];
            op[3] = ref[3];
#ifdef OLD
            op += 4, ref += 4; ref -= dec32table[op-ref];
            A32(op) = A32(ref);
            op += STEPSIZE-4; ref -= dec64;
#else
            ref += dec32table[op-ref];
            A32(op+4) = A32(ref);
            op += STEPSIZE; ref -= dec64;
#endif
        } else { LZ4_COPYSTEP(op,ref); }
        cpy = op + length - (STEPSIZE-4);

        if (unlikely(cpy>oend-COPYLENGTH-(STEPSIZE-4)))
        {
            if (cpy > oend-LASTLITERALS) goto _output_error;    /* Error : last 5 bytes must be literals */
            if (op<oend-COPYLENGTH) LZ4_WILDCOPY(op, ref, (oend-COPYLENGTH));
            while(op<cpy) *op++=*ref++;
            op=cpy;
            continue;
        }
        LZ4_WILDCOPY(op, ref, cpy);
        op=cpy;   /* correction */
    }

    /* end of decoding */
    if (endOnInput)
       return (int) (((char*)op)-dest);     /* Nb of output bytes decoded */
    else
       return (int) (((char*)ip)-source);   /* Nb of input bytes read */

    /* Overflow error detected */
_output_error:
    return (int) (-(((char*)ip)-source))-1;
}