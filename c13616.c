void PrintFormatNumber(void * ValuePtr, int Format, int ByteCount)
{
    int s,n;

    for(n=0;n<16;n++){
        switch(Format){
            case FMT_SBYTE:
            case FMT_BYTE:      printf("%02x",*(uchar *)ValuePtr); s=1;  break;
            case FMT_USHORT:    printf("%d",Get16u(ValuePtr)); s=2;      break;
            case FMT_ULONG:     
            case FMT_SLONG:     printf("%d",Get32s(ValuePtr)); s=4;      break;
            case FMT_SSHORT:    printf("%hd",(signed short)Get16u(ValuePtr)); s=2; break;
            case FMT_URATIONAL:
                printf("%u/%u",Get32s(ValuePtr), Get32s(4+(char *)ValuePtr)); 
                s = 8;
                break;

            case FMT_SRATIONAL: 
                printf("%d/%d",Get32s(ValuePtr), Get32s(4+(char *)ValuePtr)); 
                s = 8;
                break;

            case FMT_SINGLE:    printf("%f",(double)*(float *)ValuePtr); s=8; break;
            case FMT_DOUBLE:    printf("%f",*(double *)ValuePtr);        s=8; break;
            default: 
                printf("Unknown format %d:", Format);
                return;
        }
        ByteCount -= s;
        if (ByteCount <= 0) break;
        printf(", ");
        ValuePtr = (void *)((char *)ValuePtr + s);

    }
    if (n >= 16) printf("...");
}