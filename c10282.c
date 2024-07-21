MagickExport MagickBooleanType PasskeyEncipherImage(Image *image,
  const StringInfo *passkey,ExceptionInfo *exception)
{
#define EncipherImageTag  "Encipher/Image "

  AESInfo
    *aes_info;

  CacheView
    *image_view;

  char
    *signature;

  const unsigned char
    *digest;

  MagickBooleanType
    proceed;

  MagickSizeType
    extent;

  QuantumInfo
    *quantum_info;

  QuantumType
    quantum_type;

  unsigned char
    *p;

  SignatureInfo
    *signature_info;

  size_t
    length;

  ssize_t
    y;

  StringInfo
    *key,
    *nonce;

  unsigned char
    input_block[AESBlocksize],
    output_block[AESBlocksize],
    *pixels;

  /*
    Generate encipher key and nonce.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (passkey == (const StringInfo *) NULL)
    return(MagickTrue);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    return(MagickFalse);
  aes_info=AcquireAESInfo();
  key=CloneStringInfo(passkey);
  if (key == (StringInfo *) NULL)
    {
      aes_info=DestroyAESInfo(aes_info);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  nonce=SplitStringInfo(key,GetStringInfoLength(key)/2);
  if (nonce == (StringInfo *) NULL)
    {
      key=DestroyStringInfo(key);
      aes_info=DestroyAESInfo(aes_info);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  SetAESKey(aes_info,key);
  key=DestroyStringInfo(key);
  signature_info=AcquireSignatureInfo();
  UpdateSignature(signature_info,nonce);
  extent=(MagickSizeType) image->columns*image->rows;
  SetStringInfoLength(nonce,sizeof(extent));
  SetStringInfoDatum(nonce,(const unsigned char *) &extent);
  UpdateSignature(signature_info,nonce);
  nonce=DestroyStringInfo(nonce);
  FinalizeSignature(signature_info);
  signature=StringInfoToHexString(GetSignatureDigest(signature_info));
  (void) SetImageProperty(image,"cipher:type","AES",exception);
  (void) SetImageProperty(image,"cipher:mode","CTR",exception);
  (void) SetImageProperty(image,"cipher:nonce",signature,exception);
  signature=DestroyString(signature);
  (void) memset(input_block,0,sizeof(input_block));
  digest=GetStringInfoDatum(GetSignatureDigest(signature_info));
  (void) memcpy(input_block,digest,MagickMin(AESBlocksize,
    GetSignatureDigestsize(signature_info))*sizeof(*input_block));
  signature_info=DestroySignatureInfo(signature_info);
  /*
    Convert plain pixels to cipher pixels.
  */
  quantum_info=AcquireQuantumInfo((const ImageInfo *) NULL,image);
  if (quantum_info == (QuantumInfo *) NULL)
    {
      aes_info=DestroyAESInfo(aes_info);
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image->filename);
    }
  quantum_type=GetQuantumType(image,exception);
  pixels=(unsigned char *) GetQuantumPixels(quantum_info);
  image_view=AcquireAuthenticCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    ssize_t
      i,
      x;

    Quantum
      *magick_restrict q;

    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      break;
    length=ExportQuantumPixels(image,image_view,quantum_info,quantum_type,
      pixels,exception);
    p=pixels;
    for (x=0; x < (ssize_t) length; x+=AESBlocksize)
    {
      (void) memcpy(output_block,input_block,AESBlocksize*
        sizeof(*output_block));
      IncrementCipherNonce(AESBlocksize,input_block);
      EncipherAESBlock(aes_info,output_block,output_block);
      for (i=0; i < AESBlocksize; i++)
        p[i]^=output_block[i];
      p+=AESBlocksize;
    }
    (void) memcpy(output_block,input_block,AESBlocksize*
      sizeof(*output_block));
    EncipherAESBlock(aes_info,output_block,output_block);
    for (i=0; x < (ssize_t) length; x++)
    {
      p[i]^=output_block[i];
      i++;
    }
    (void) ImportQuantumPixels(image,image_view,quantum_info,quantum_type,
      pixels,exception);
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      break;
    proceed=SetImageProgress(image,EncipherImageTag,(MagickOffsetType) y,
      image->rows);
    if (proceed == MagickFalse)
      break;
  }
  image_view=DestroyCacheView(image_view);
  image->taint=MagickFalse;
  /*
    Free resources.
  */
  quantum_info=DestroyQuantumInfo(quantum_info);
  aes_info=DestroyAESInfo(aes_info);
  (void) memset(input_block,0,sizeof(input_block));
  (void) memset(output_block,0,sizeof(output_block));
  return(y == (ssize_t) image->rows ? MagickTrue : MagickFalse);
}