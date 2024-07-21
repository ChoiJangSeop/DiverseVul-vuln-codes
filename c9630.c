static double OptimalTau(const ssize_t *histogram,const double max_tau,
  const double min_tau,const double delta_tau,const double smooth_threshold,
  short *extrema)
{
  IntervalTree
    **list,
    *node,
    *root;

  MagickBooleanType
    peak;

  double
    average_tau,
    *derivative,
    *second_derivative,
    tau,
    value;

  register ssize_t
    i,
    x;

  size_t
    count,
    number_crossings;

  ssize_t
    index,
    j,
    k,
    number_nodes;

  ZeroCrossing
    *zero_crossing;

  /*
    Allocate interval tree.
  */
  list=(IntervalTree **) AcquireQuantumMemory((size_t) TreeLength,
    sizeof(*list));
  if (list == (IntervalTree **) NULL)
    return(0.0);
  /*
    Allocate zero crossing list.
  */
  count=(size_t) ((max_tau-min_tau)/delta_tau)+2;
  zero_crossing=(ZeroCrossing *) AcquireQuantumMemory((size_t) count,
    sizeof(*zero_crossing));
  if (zero_crossing == (ZeroCrossing *) NULL)
    {
      list=(IntervalTree **) RelinquishMagickMemory(list);
      return(0.0);
    }
  for (i=0; i < (ssize_t) count; i++)
    zero_crossing[i].tau=(-1.0);
  /*
    Initialize zero crossing list.
  */
  derivative=(double *) AcquireCriticalMemory(256*sizeof(*derivative));
  second_derivative=(double *) AcquireCriticalMemory(256*
    sizeof(*second_derivative));
  i=0;
  for (tau=max_tau; tau >= min_tau; tau-=delta_tau)
  {
    zero_crossing[i].tau=tau;
    ScaleSpace(histogram,tau,zero_crossing[i].histogram);
    DerivativeHistogram(zero_crossing[i].histogram,derivative);
    DerivativeHistogram(derivative,second_derivative);
    ZeroCrossHistogram(second_derivative,smooth_threshold,
      zero_crossing[i].crossings);
    i++;
  }
  /*
    Add an entry for the original histogram.
  */
  zero_crossing[i].tau=0.0;
  for (j=0; j <= 255; j++)
    zero_crossing[i].histogram[j]=(double) histogram[j];
  DerivativeHistogram(zero_crossing[i].histogram,derivative);
  DerivativeHistogram(derivative,second_derivative);
  ZeroCrossHistogram(second_derivative,smooth_threshold,
    zero_crossing[i].crossings);
  number_crossings=(size_t) i;
  derivative=(double *) RelinquishMagickMemory(derivative);
  second_derivative=(double *) RelinquishMagickMemory(second_derivative);
  /*
    Ensure the scale-space fingerprints form lines in scale-space, not loops.
  */
  ConsolidateCrossings(zero_crossing,number_crossings);
  /*
    Force endpoints to be included in the interval.
  */
  for (i=0; i <= (ssize_t) number_crossings; i++)
  {
    for (j=0; j < 255; j++)
      if (zero_crossing[i].crossings[j] != 0)
        break;
    zero_crossing[i].crossings[0]=(-zero_crossing[i].crossings[j]);
    for (j=255; j > 0; j--)
      if (zero_crossing[i].crossings[j] != 0)
        break;
    zero_crossing[i].crossings[255]=(-zero_crossing[i].crossings[j]);
  }
  /*
    Initialize interval tree.
  */
  root=InitializeIntervalTree(zero_crossing,number_crossings);
  if (root == (IntervalTree *) NULL)
    {
      zero_crossing=(ZeroCrossing *) RelinquishMagickMemory(zero_crossing);
      list=(IntervalTree **) RelinquishMagickMemory(list);
      return(0.0);
    }
  /*
    Find active nodes:  stability is greater (or equal) to the mean stability of
    its children.
  */
  number_nodes=0;
  ActiveNodes(list,&number_nodes,root->child);
  /*
    Initialize extrema.
  */
  for (i=0; i <= 255; i++)
    extrema[i]=0;
  for (i=0; i < number_nodes; i++)
  {
    /*
      Find this tau in zero crossings list.
    */
    k=0;
    node=list[i];
    for (j=0; j <= (ssize_t) number_crossings; j++)
      if (zero_crossing[j].tau == node->tau)
        k=j;
    /*
      Find the value of the peak.
    */
    peak=zero_crossing[k].crossings[node->right] == -1 ? MagickTrue :
      MagickFalse;
    index=node->left;
    value=zero_crossing[k].histogram[index];
    for (x=node->left; x <= node->right; x++)
    {
      if (peak != MagickFalse)
        {
          if (zero_crossing[k].histogram[x] > value)
            {
              value=zero_crossing[k].histogram[x];
              index=x;
            }
        }
      else
        if (zero_crossing[k].histogram[x] < value)
          {
            value=zero_crossing[k].histogram[x];
            index=x;
          }
    }
    for (x=node->left; x <= node->right; x++)
    {
      if (index == 0)
        index=256;
      if (peak != MagickFalse)
        extrema[x]=(short) index;
      else
        extrema[x]=(short) (-index);
    }
  }
  /*
    Determine the average tau.
  */
  average_tau=0.0;
  for (i=0; i < number_nodes; i++)
    average_tau+=list[i]->tau;
  average_tau/=(double) number_nodes;
  /*
    Relinquish resources.
  */
  FreeNodes(root);
  zero_crossing=(ZeroCrossing *) RelinquishMagickMemory(zero_crossing);
  list=(IntervalTree **) RelinquishMagickMemory(list);
  return(average_tau);
}