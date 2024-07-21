static MagickBooleanType Classify(Image *image,short **extrema,
  const double cluster_threshold,
  const double weighting_exponent,const MagickBooleanType verbose,
  ExceptionInfo *exception)
{
#define SegmentImageTag  "Segment/Image"
#define ThrowClassifyException(severity,tag,label) \
{\
  for (cluster=head; cluster != (Cluster *) NULL; cluster=next_cluster) \
  { \
    next_cluster=cluster->next; \
    cluster=(Cluster *) RelinquishMagickMemory(cluster); \
  } \
  if (squares != (double *) NULL) \
    { \
      squares-=255; \
      free_squares=squares; \
      free_squares=(double *) RelinquishMagickMemory(free_squares); \
    } \
  ThrowBinaryException(severity,tag,label); \
}

  CacheView
    *image_view;

  Cluster
    *cluster,
    *head,
    *last_cluster,
    *next_cluster;

  ExtentPacket
    blue,
    green,
    red;

  MagickOffsetType
    progress;

  double
    *free_squares;

  MagickStatusType
    status;

  register ssize_t
    i;

  register double
    *squares;

  size_t
    number_clusters;

  ssize_t
    count,
    y;

  /*
    Form clusters.
  */
  cluster=(Cluster *) NULL;
  head=(Cluster *) NULL;
  squares=(double *) NULL;
  (void) memset(&red,0,sizeof(red));
  (void) memset(&green,0,sizeof(green));
  (void) memset(&blue,0,sizeof(blue));
  while (DefineRegion(extrema[Red],&red) != 0)
  {
    green.index=0;
    while (DefineRegion(extrema[Green],&green) != 0)
    {
      blue.index=0;
      while (DefineRegion(extrema[Blue],&blue) != 0)
      {
        /*
          Allocate a new class.
        */
        if (head != (Cluster *) NULL)
          {
            cluster->next=(Cluster *) AcquireMagickMemory(
              sizeof(*cluster->next));
            cluster=cluster->next;
          }
        else
          {
            cluster=(Cluster *) AcquireMagickMemory(sizeof(*cluster));
            head=cluster;
          }
        if (cluster == (Cluster *) NULL)
          ThrowClassifyException(ResourceLimitError,"MemoryAllocationFailed",
            image->filename);
        /*
          Initialize a new class.
        */
        cluster->count=0;
        cluster->red=red;
        cluster->green=green;
        cluster->blue=blue;
        cluster->next=(Cluster *) NULL;
      }
    }
  }
  if (head == (Cluster *) NULL)
    {
      /*
        No classes were identified-- create one.
      */
      cluster=(Cluster *) AcquireMagickMemory(sizeof(*cluster));
      if (cluster == (Cluster *) NULL)
        ThrowClassifyException(ResourceLimitError,"MemoryAllocationFailed",
          image->filename);
      /*
        Initialize a new class.
      */
      cluster->count=0;
      cluster->red=red;
      cluster->green=green;
      cluster->blue=blue;
      cluster->next=(Cluster *) NULL;
      head=cluster;
    }
  /*
    Count the pixels for each cluster.
  */
  status=MagickTrue;
  count=0;
  progress=0;
  image_view=AcquireVirtualCacheView(image,exception);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    register const Quantum
      *p;

    register ssize_t
      x;

    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    if (p == (const Quantum *) NULL)
      break;
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      for (cluster=head; cluster != (Cluster *) NULL; cluster=cluster->next)
        if (((ssize_t) ScaleQuantumToChar(GetPixelRed(image,p)) >=
             (cluster->red.left-SafeMargin)) &&
            ((ssize_t) ScaleQuantumToChar(GetPixelRed(image,p)) <=
             (cluster->red.right+SafeMargin)) &&
            ((ssize_t) ScaleQuantumToChar(GetPixelGreen(image,p)) >=
             (cluster->green.left-SafeMargin)) &&
            ((ssize_t) ScaleQuantumToChar(GetPixelGreen(image,p)) <=
             (cluster->green.right+SafeMargin)) &&
            ((ssize_t) ScaleQuantumToChar(GetPixelBlue(image,p)) >=
             (cluster->blue.left-SafeMargin)) &&
            ((ssize_t) ScaleQuantumToChar(GetPixelBlue(image,p)) <=
             (cluster->blue.right+SafeMargin)))
          {
            /*
              Count this pixel.
            */
            count++;
            cluster->red.center+=(double) ScaleQuantumToChar(
              GetPixelRed(image,p));
            cluster->green.center+=(double) ScaleQuantumToChar(
              GetPixelGreen(image,p));
            cluster->blue.center+=(double) ScaleQuantumToChar(
              GetPixelBlue(image,p));
            cluster->count++;
            break;
          }
      p+=GetPixelChannels(image);
    }
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,SegmentImageTag,progress,2*image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  /*
    Remove clusters that do not meet minimum cluster threshold.
  */
  count=0;
  last_cluster=head;
  next_cluster=head;
  for (cluster=head; cluster != (Cluster *) NULL; cluster=next_cluster)
  {
    next_cluster=cluster->next;
    if ((cluster->count > 0) &&
        (cluster->count >= (count*cluster_threshold/100.0)))
      {
        /*
          Initialize cluster.
        */
        cluster->id=count;
        cluster->red.center/=cluster->count;
        cluster->green.center/=cluster->count;
        cluster->blue.center/=cluster->count;
        count++;
        last_cluster=cluster;
        continue;
      }
    /*
      Delete cluster.
    */
    if (cluster == head)
      head=next_cluster;
    else
      last_cluster->next=next_cluster;
    cluster=(Cluster *) RelinquishMagickMemory(cluster);
  }
  number_clusters=(size_t) count;
  if (verbose != MagickFalse)
    {
      /*
        Print cluster statistics.
      */
      (void) FormatLocaleFile(stdout,"Fuzzy C-means Statistics\n");
      (void) FormatLocaleFile(stdout,"===================\n\n");
      (void) FormatLocaleFile(stdout,"\tCluster Threshold = %g\n",(double)
        cluster_threshold);
      (void) FormatLocaleFile(stdout,"\tWeighting Exponent = %g\n",(double)
        weighting_exponent);
      (void) FormatLocaleFile(stdout,"\tTotal Number of Clusters = %.20g\n\n",
        (double) number_clusters);
      /*
        Print the total number of points per cluster.
      */
      (void) FormatLocaleFile(stdout,"\n\nNumber of Vectors Per Cluster\n");
      (void) FormatLocaleFile(stdout,"=============================\n\n");
      for (cluster=head; cluster != (Cluster *) NULL; cluster=cluster->next)
        (void) FormatLocaleFile(stdout,"Cluster #%.20g = %.20g\n",(double)
          cluster->id,(double) cluster->count);
      /*
        Print the cluster extents.
      */
      (void) FormatLocaleFile(stdout,
        "\n\n\nCluster Extents:        (Vector Size: %d)\n",MaxDimension);
      (void) FormatLocaleFile(stdout,"================");
      for (cluster=head; cluster != (Cluster *) NULL; cluster=cluster->next)
      {
        (void) FormatLocaleFile(stdout,"\n\nCluster #%.20g\n\n",(double)
          cluster->id);
        (void) FormatLocaleFile(stdout,
          "%.20g-%.20g  %.20g-%.20g  %.20g-%.20g\n",(double)
          cluster->red.left,(double) cluster->red.right,(double)
          cluster->green.left,(double) cluster->green.right,(double)
          cluster->blue.left,(double) cluster->blue.right);
      }
      /*
        Print the cluster center values.
      */
      (void) FormatLocaleFile(stdout,
        "\n\n\nCluster Center Values:        (Vector Size: %d)\n",MaxDimension);
      (void) FormatLocaleFile(stdout,"=====================");
      for (cluster=head; cluster != (Cluster *) NULL; cluster=cluster->next)
      {
        (void) FormatLocaleFile(stdout,"\n\nCluster #%.20g\n\n",(double)
          cluster->id);
        (void) FormatLocaleFile(stdout,"%g  %g  %g\n",(double)
          cluster->red.center,(double) cluster->green.center,(double)
          cluster->blue.center);
      }
      (void) FormatLocaleFile(stdout,"\n");
    }
  if (number_clusters > 256)
    ThrowClassifyException(ImageError,"TooManyClusters",image->filename);
  /*
    Speed up distance calculations.
  */
  squares=(double *) AcquireQuantumMemory(513UL,sizeof(*squares));
  if (squares == (double *) NULL)
    ThrowClassifyException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  squares+=255;
  for (i=(-255); i <= 255; i++)
    squares[i]=(double) i*(double) i;
  /*
    Allocate image colormap.
  */
  if (AcquireImageColormap(image,number_clusters,exception) == MagickFalse)
    ThrowClassifyException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  i=0;
  for (cluster=head; cluster != (Cluster *) NULL; cluster=cluster->next)
  {
    image->colormap[i].red=(double) ScaleCharToQuantum((unsigned char)
      (cluster->red.center+0.5));
    image->colormap[i].green=(double) ScaleCharToQuantum((unsigned char)
      (cluster->green.center+0.5));
    image->colormap[i].blue=(double) ScaleCharToQuantum((unsigned char)
      (cluster->blue.center+0.5));
    i++;
  }
  /*
    Do course grain classes.
  */
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    Cluster
      *clust;

    register const PixelInfo
      *magick_restrict p;

    register ssize_t
      x;

    register Quantum
      *magick_restrict q;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      SetPixelIndex(image,0,q);
      for (clust=head; clust != (Cluster *) NULL; clust=clust->next)
      {
        if (((ssize_t) ScaleQuantumToChar(GetPixelRed(image,q)) >=
             (clust->red.left-SafeMargin)) &&
            ((ssize_t) ScaleQuantumToChar(GetPixelRed(image,q)) <=
             (clust->red.right+SafeMargin)) &&
            ((ssize_t) ScaleQuantumToChar(GetPixelGreen(image,q)) >=
             (clust->green.left-SafeMargin)) &&
            ((ssize_t) ScaleQuantumToChar(GetPixelGreen(image,q)) <=
             (clust->green.right+SafeMargin)) &&
            ((ssize_t) ScaleQuantumToChar(GetPixelBlue(image,q)) >=
             (clust->blue.left-SafeMargin)) &&
            ((ssize_t) ScaleQuantumToChar(GetPixelBlue(image,q)) <=
             (clust->blue.right+SafeMargin)))
          {
            /*
              Classify this pixel.
            */
            SetPixelIndex(image,(Quantum) clust->id,q);
            break;
          }
      }
      if (clust == (Cluster *) NULL)
        {
          double
            distance_squared,
            local_minima,
            numerator,
            ratio,
            sum;

          register ssize_t
            j,
            k;

          /*
            Compute fuzzy membership.
          */
          local_minima=0.0;
          for (j=0; j < (ssize_t) image->colors; j++)
          {
            sum=0.0;
            p=image->colormap+j;
            distance_squared=squares[(ssize_t) ScaleQuantumToChar(
              GetPixelRed(image,q))-(ssize_t)
              ScaleQuantumToChar(ClampToQuantum(p->red))]+squares[(ssize_t)
              ScaleQuantumToChar(GetPixelGreen(image,q))-(ssize_t)
              ScaleQuantumToChar(ClampToQuantum(p->green))]+squares[(ssize_t)
              ScaleQuantumToChar(GetPixelBlue(image,q))-(ssize_t)
              ScaleQuantumToChar(ClampToQuantum(p->blue))];
            numerator=distance_squared;
            for (k=0; k < (ssize_t) image->colors; k++)
            {
              p=image->colormap+k;
                distance_squared=squares[(ssize_t) ScaleQuantumToChar(
                  GetPixelRed(image,q))-(ssize_t)
                  ScaleQuantumToChar(ClampToQuantum(p->red))]+squares[
                  (ssize_t) ScaleQuantumToChar(GetPixelGreen(image,q))-(ssize_t)
                  ScaleQuantumToChar(ClampToQuantum(p->green))]+squares[
                  (ssize_t) ScaleQuantumToChar(GetPixelBlue(image,q))-(ssize_t)
                  ScaleQuantumToChar(ClampToQuantum(p->blue))];
              ratio=numerator/distance_squared;
              sum+=SegmentPower(ratio);
            }
            if ((sum != 0.0) && ((1.0/sum) > local_minima))
              {
                /*
                  Classify this pixel.
                */
                local_minima=1.0/sum;
                SetPixelIndex(image,(Quantum) j,q);
              }
          }
        }
      q+=GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,SegmentImageTag,progress,2*image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  status&=SyncImage(image,exception);
  /*
    Relinquish resources.
  */
  for (cluster=head; cluster != (Cluster *) NULL; cluster=next_cluster)
  {
    next_cluster=cluster->next;
    cluster=(Cluster *) RelinquishMagickMemory(cluster);
  }
  squares-=255;
  free_squares=squares;
  free_squares=(double *) RelinquishMagickMemory(free_squares);
  return(MagickTrue);
}