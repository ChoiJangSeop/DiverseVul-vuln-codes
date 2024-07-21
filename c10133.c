static Quantum ApplyFunction(Quantum pixel,const MagickFunction function,
  const size_t number_parameters,const double *parameters,
  ExceptionInfo *exception)
{
  MagickRealType
    result;

  ssize_t
    i;

  (void) exception;
  result=0.0;
  switch (function)
  {
    case PolynomialFunction:
    {
      /*
       * Polynomial
       * Parameters:   polynomial constants,  highest to lowest order
       *   For example:      c0*x^3 + c1*x^2 + c2*x  + c3
       */
      result=0.0;
      for (i=0; i < (ssize_t) number_parameters; i++)
        result=result*QuantumScale*pixel + parameters[i];
      result*=QuantumRange;
      break;
    }
    case SinusoidFunction:
    {
      /* Sinusoid Function
       * Parameters:   Freq, Phase, Ampl, bias
       */
      double  freq,phase,ampl,bias;
      freq  = ( number_parameters >= 1 ) ? parameters[0] : 1.0;
      phase = ( number_parameters >= 2 ) ? parameters[1] : 0.0;
      ampl  = ( number_parameters >= 3 ) ? parameters[2] : 0.5;
      bias  = ( number_parameters >= 4 ) ? parameters[3] : 0.5;
      result=(MagickRealType) (QuantumRange*(ampl*sin((double) (2.0*MagickPI*
        (freq*QuantumScale*pixel + phase/360.0) )) + bias ) );
      break;
    }
    case ArcsinFunction:
    {
      /* Arcsin Function  (peged at range limits for invalid results)
       * Parameters:   Width, Center, Range, Bias
       */
      double  width,range,center,bias;
      width  = ( number_parameters >= 1 ) ? parameters[0] : 1.0;
      center = ( number_parameters >= 2 ) ? parameters[1] : 0.5;
      range  = ( number_parameters >= 3 ) ? parameters[2] : 1.0;
      bias   = ( number_parameters >= 4 ) ? parameters[3] : 0.5;
      result = 2.0/width*(QuantumScale*pixel - center);
      if ( result <= -1.0 )
        result = bias - range/2.0;
      else if ( result >= 1.0 )
        result = bias + range/2.0;
      else
        result=(MagickRealType) (range/MagickPI*asin((double) result)+bias);
      result *= QuantumRange;
      break;
    }
    case ArctanFunction:
    {
      /* Arctan Function
       * Parameters:   Slope, Center, Range, Bias
       */
      double  slope,range,center,bias;
      slope  = ( number_parameters >= 1 ) ? parameters[0] : 1.0;
      center = ( number_parameters >= 2 ) ? parameters[1] : 0.5;
      range  = ( number_parameters >= 3 ) ? parameters[2] : 1.0;
      bias   = ( number_parameters >= 4 ) ? parameters[3] : 0.5;
      result=(MagickRealType) (MagickPI*slope*(QuantumScale*pixel-center));
      result=(MagickRealType) (QuantumRange*(range/MagickPI*atan((double)
                  result) + bias ) );
      break;
    }
    case UndefinedFunction:
      break;
  }
  return(ClampToQuantum(result));
}