static Quantum ApplyFunction(Quantum pixel,const MagickFunction function,
  const size_t number_parameters,const double *parameters,
  ExceptionInfo *exception)
{
  double
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
        Polynomial: polynomial constants, highest to lowest order (e.g. c0*x^3+
        c1*x^2+c2*x+c3).
      */
      result=0.0;
      for (i=0; i < (ssize_t) number_parameters; i++)
        result=result*QuantumScale*pixel+parameters[i];
      result*=QuantumRange;
      break;
    }
    case SinusoidFunction:
    {
      double
        amplitude,
        bias,
        frequency,
        phase;

      /*
        Sinusoid: frequency, phase, amplitude, bias.
      */
      frequency=(number_parameters >= 1) ? parameters[0] : 1.0;
      phase=(number_parameters >= 2) ? parameters[1] : 0.0;
      amplitude=(number_parameters >= 3) ? parameters[2] : 0.5;
      bias=(number_parameters >= 4) ? parameters[3] : 0.5;
      result=(double) (QuantumRange*(amplitude*sin((double) (2.0*
        MagickPI*(frequency*QuantumScale*pixel+phase/360.0)))+bias));
      break;
    }
    case ArcsinFunction:
    {
      double
        bias,
        center,
        range,
        width;

      /*
        Arcsin (peged at range limits for invalid results): width, center,
        range, and bias.
      */
      width=(number_parameters >= 1) ? parameters[0] : 1.0;
      center=(number_parameters >= 2) ? parameters[1] : 0.5;
      range=(number_parameters >= 3) ? parameters[2] : 1.0;
      bias=(number_parameters >= 4) ? parameters[3] : 0.5;
      result=2.0/width*(QuantumScale*pixel-center);
      if ( result <= -1.0 )
        result=bias-range/2.0;
      else
        if (result >= 1.0)
          result=bias+range/2.0;
        else
          result=(double) (range/MagickPI*asin((double) result)+bias);
      result*=QuantumRange;
      break;
    }
    case ArctanFunction:
    {
      double
        center,
        bias,
        range,
        slope;

      /*
        Arctan: slope, center, range, and bias.
      */
      slope=(number_parameters >= 1) ? parameters[0] : 1.0;
      center=(number_parameters >= 2) ? parameters[1] : 0.5;
      range=(number_parameters >= 3) ? parameters[2] : 1.0;
      bias=(number_parameters >= 4) ? parameters[3] : 0.5;
      result=(double) (MagickPI*slope*(QuantumScale*pixel-center));
      result=(double) (QuantumRange*(range/MagickPI*atan((double)
        result)+bias));
      break;
    }
    case UndefinedFunction:
      break;
  }
  return(ClampToQuantum(result));
}