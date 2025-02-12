static MagickBooleanType DecodeImage(Image *image,unsigned char *luma,
  unsigned char *chroma1,unsigned char *chroma2,ExceptionInfo *exception)
{
#define IsSync(sum)  ((sum & 0xffffff00UL) == 0xfffffe00UL)
#define PCDGetBits(n) \
{  \
  sum=(sum << n) & 0xffffffff; \
  bits-=n; \
  while (bits <= 24) \
  { \
    if (p >= (buffer+0x800)) \
      { \
        count=ReadBlob(image,0x800,buffer); \
        p=buffer; \
      } \
    sum|=((unsigned int) (*p) << (24-bits)); \
    bits+=8; \
    p++; \
  } \
}

  typedef struct PCDTable
  {
    unsigned int
      length,
      sequence;

    MagickStatusType
      mask;

    unsigned char
      key;
  } PCDTable;

  PCDTable
    *pcd_table[3];

  register ssize_t
    i,
    j;

  register PCDTable
    *r;

  register unsigned char
    *p,
    *q;

  size_t
    bits,
    length,
    plane,
    pcd_length[3],
    row,
    sum;

  ssize_t
    count,
    quantum;

  unsigned char
    *buffer;

  /*
    Initialize Huffman tables.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(luma != (unsigned char *) NULL);
  assert(chroma1 != (unsigned char *) NULL);
  assert(chroma2 != (unsigned char *) NULL);
  buffer=(unsigned char *) AcquireQuantumMemory(0x800,sizeof(*buffer));
  if (buffer == (unsigned char *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  sum=0;
  bits=32;
  p=buffer+0x800;
  for (i=0; i < 3; i++)
  {
    pcd_table[i]=(PCDTable *) NULL;
    pcd_length[i]=0;
  }
  for (i=0; i < (image->columns > 1536 ? 3 : 1); i++)
  {
    PCDGetBits(8);
    length=(sum & 0xff)+1;
    pcd_table[i]=(PCDTable *) AcquireQuantumMemory(length,
      sizeof(*pcd_table[i]));
    if (pcd_table[i] == (PCDTable *) NULL)
      {
        buffer=(unsigned char *) RelinquishMagickMemory(buffer);
        ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
          image->filename);
      }
    r=pcd_table[i];
    for (j=0; j < (ssize_t) length; j++)
    {
      PCDGetBits(8);
      r->length=(unsigned int) (sum & 0xff)+1;
      if (r->length > 16)
        {
          buffer=(unsigned char *) RelinquishMagickMemory(buffer);
          return(MagickFalse);
        }
      PCDGetBits(16);
      r->sequence=(unsigned int) (sum & 0xffff) << 16;
      PCDGetBits(8);
      r->key=(unsigned char) (sum & 0xff);
      r->mask=(~((1U << (32-r->length))-1));
      r++;
    }
    pcd_length[i]=(size_t) length;
  }
  /*
    Search for Sync byte.
  */
  for (i=0; i < 1; i++)
    PCDGetBits(16);
  for (i=0; i < 1; i++)
    PCDGetBits(16);
  while ((sum & 0x00fff000UL) != 0x00fff000UL)
    PCDGetBits(8);
  while (IsSync(sum) == 0)
    PCDGetBits(1);
  /*
    Recover the Huffman encoded luminance and chrominance deltas.
  */
  count=0;
  length=0;
  plane=0;
  row=0;
  q=luma;
  for ( ; ; )
  {
    if (IsSync(sum) != 0)
      {
        /*
          Determine plane and row number.
        */
        PCDGetBits(16);
        row=((sum >> 9) & 0x1fff);
        if (row == image->rows)
          break;
        PCDGetBits(8);
        plane=sum >> 30;
        PCDGetBits(16);
        switch (plane)
        {
          case 0:
          {
            q=luma+row*image->columns;
            count=(ssize_t) image->columns;
            break;
          }
          case 2:
          {
            q=chroma1+(row >> 1)*image->columns;
            count=(ssize_t) (image->columns >> 1);
            plane--;
            break;
          }
          case 3:
          {
            q=chroma2+(row >> 1)*image->columns;
            count=(ssize_t) (image->columns >> 1);
            plane--;
            break;
          }
          default:
          {
            for (i=0; i < (image->columns > 1536 ? 3 : 1); i++)
              pcd_table[i]=(PCDTable *) RelinquishMagickMemory(pcd_table[i]);
            buffer=(unsigned char *) RelinquishMagickMemory(buffer);
            ThrowBinaryException(CorruptImageError,"CorruptImage",
              image->filename);
          }
        }
        length=pcd_length[plane];
        continue;
      }
    /*
      Decode luminance or chrominance deltas.
    */
    r=pcd_table[plane];
    for (i=0; ((i < (ssize_t) length) && ((sum & r->mask) != r->sequence)); i++)
      r++;
    if ((row > image->rows) || (r == (PCDTable *) NULL))
      {
        (void) ThrowMagickException(exception,GetMagickModule(),
          CorruptImageWarning,"SkipToSyncByte","`%s'",image->filename);
        while ((sum & 0x00fff000) != 0x00fff000)
          PCDGetBits(8);
        while (IsSync(sum) == 0)
          PCDGetBits(1);
        continue;
      }
    if (r->key < 128)
      quantum=(ssize_t) (*q)+r->key;
    else
      quantum=(ssize_t) (*q)+r->key-256;
    *q=(unsigned char) ((quantum < 0) ? 0 : (quantum > 255) ? 255 : quantum);
    q++;
    PCDGetBits(r->length);
    count--;
  }
  /*
    Relinquish resources.
  */
  for (i=0; i < (image->columns > 1536 ? 3 : 1); i++)
    pcd_table[i]=(PCDTable *) RelinquishMagickMemory(pcd_table[i]);
  buffer=(unsigned char *) RelinquishMagickMemory(buffer);
  return(MagickTrue);
}