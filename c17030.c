void Gfx::doTilingPatternFill(GfxTilingPattern *tPat,
			      GBool stroke, GBool eoFill) {
  GfxPatternColorSpace *patCS;
  GfxColorSpace *cs;
  GfxColor color;
  GfxPath *savedPath;
  double xMin, yMin, xMax, yMax, x, y, x1, y1;
  double cxMin, cyMin, cxMax, cyMax;
  int xi0, yi0, xi1, yi1, xi, yi;
  double *ctm, *btm, *ptm;
  double m[6], ictm[6], m1[6], imb[6];
  double det;
  double xstep, ystep;
  int i;

  // get color space
  patCS = (GfxPatternColorSpace *)(stroke ? state->getStrokeColorSpace()
				          : state->getFillColorSpace());

  // construct a (pattern space) -> (current space) transform matrix
  ctm = state->getCTM();
  btm = baseMatrix;
  ptm = tPat->getMatrix();
  // iCTM = invert CTM
  det = 1 / (ctm[0] * ctm[3] - ctm[1] * ctm[2]);
  ictm[0] = ctm[3] * det;
  ictm[1] = -ctm[1] * det;
  ictm[2] = -ctm[2] * det;
  ictm[3] = ctm[0] * det;
  ictm[4] = (ctm[2] * ctm[5] - ctm[3] * ctm[4]) * det;
  ictm[5] = (ctm[1] * ctm[4] - ctm[0] * ctm[5]) * det;
  // m1 = PTM * BTM = PTM * base transform matrix
  m1[0] = ptm[0] * btm[0] + ptm[1] * btm[2];
  m1[1] = ptm[0] * btm[1] + ptm[1] * btm[3];
  m1[2] = ptm[2] * btm[0] + ptm[3] * btm[2];
  m1[3] = ptm[2] * btm[1] + ptm[3] * btm[3];
  m1[4] = ptm[4] * btm[0] + ptm[5] * btm[2] + btm[4];
  m1[5] = ptm[4] * btm[1] + ptm[5] * btm[3] + btm[5];
  // m = m1 * iCTM = (PTM * BTM) * (iCTM)
  m[0] = m1[0] * ictm[0] + m1[1] * ictm[2];
  m[1] = m1[0] * ictm[1] + m1[1] * ictm[3];
  m[2] = m1[2] * ictm[0] + m1[3] * ictm[2];
  m[3] = m1[2] * ictm[1] + m1[3] * ictm[3];
  m[4] = m1[4] * ictm[0] + m1[5] * ictm[2] + ictm[4];
  m[5] = m1[4] * ictm[1] + m1[5] * ictm[3] + ictm[5];

  // construct a (device space) -> (pattern space) transform matrix
  det = 1 / (m1[0] * m1[3] - m1[1] * m1[2]);
  imb[0] = m1[3] * det;
  imb[1] = -m1[1] * det;
  imb[2] = -m1[2] * det;
  imb[3] = m1[0] * det;
  imb[4] = (m1[2] * m1[5] - m1[3] * m1[4]) * det;
  imb[5] = (m1[1] * m1[4] - m1[0] * m1[5]) * det;

  // save current graphics state
  savedPath = state->getPath()->copy();
  saveState();

  // set underlying color space (for uncolored tiling patterns); set
  // various other parameters (stroke color, line width) to match
  // Adobe's behavior
  if (tPat->getPaintType() == 2 && (cs = patCS->getUnder())) {
    state->setFillColorSpace(cs->copy());
    out->updateFillColorSpace(state);
    state->setStrokeColorSpace(cs->copy());
    out->updateStrokeColorSpace(state);
    if (stroke) {
	state->setFillColor(state->getStrokeColor());
    } else {
	state->setStrokeColor(state->getFillColor());
    }
  } else {
    cs = new GfxDeviceGrayColorSpace();
    state->setFillColorSpace(cs);
    cs->getDefaultColor(&color);
    state->setFillColor(&color);
    out->updateFillColorSpace(state);
    state->setStrokeColorSpace(new GfxDeviceGrayColorSpace());
    state->setStrokeColor(&color);
    out->updateStrokeColorSpace(state);
  }
  state->setFillPattern(NULL);
  out->updateFillColor(state);
  state->setStrokePattern(NULL);
  out->updateStrokeColor(state);

  // clip to current path
  if (stroke) {
    state->clipToStrokePath();
    out->clipToStrokePath(state);
  } else if (!textHaveCSPattern && !maskHaveCSPattern) {
    state->clip();
    if (eoFill) {
      out->eoClip(state);
    } else {
      out->clip(state);
    }
  }
  state->clearPath();
  state->setLineWidth(0);
  out->updateLineWidth(state);

  // get the clip region, check for empty
  state->getClipBBox(&cxMin, &cyMin, &cxMax, &cyMax);
  if (cxMin > cxMax || cyMin > cyMax) {
    goto restore;
  }

  // transform clip region bbox to pattern space
  xMin = xMax = cxMin * imb[0] + cyMin * imb[2] + imb[4];
  yMin = yMax = cxMin * imb[1] + cyMin * imb[3] + imb[5];
  x1 = cxMin * imb[0] + cyMax * imb[2] + imb[4];
  y1 = cxMin * imb[1] + cyMax * imb[3] + imb[5];
  if (x1 < xMin) {
    xMin = x1;
  } else if (x1 > xMax) {
    xMax = x1;
  }
  if (y1 < yMin) {
    yMin = y1;
  } else if (y1 > yMax) {
    yMax = y1;
  }
  x1 = cxMax * imb[0] + cyMin * imb[2] + imb[4];
  y1 = cxMax * imb[1] + cyMin * imb[3] + imb[5];
  if (x1 < xMin) {
    xMin = x1;
  } else if (x1 > xMax) {
    xMax = x1;
  }
  if (y1 < yMin) {
    yMin = y1;
  } else if (y1 > yMax) {
    yMax = y1;
  }
  x1 = cxMax * imb[0] + cyMax * imb[2] + imb[4];
  y1 = cxMax * imb[1] + cyMax * imb[3] + imb[5];
  if (x1 < xMin) {
    xMin = x1;
  } else if (x1 > xMax) {
    xMax = x1;
  }
  if (y1 < yMin) {
    yMin = y1;
  } else if (y1 > yMax) {
    yMax = y1;
  }

  // draw the pattern
  //~ this should treat negative steps differently -- start at right/top
  //~ edge instead of left/bottom (?)
  xstep = fabs(tPat->getXStep());
  ystep = fabs(tPat->getYStep());
  xi0 = (int)ceil((xMin - tPat->getBBox()[2]) / xstep);
  xi1 = (int)floor((xMax - tPat->getBBox()[0]) / xstep) + 1;
  yi0 = (int)ceil((yMin - tPat->getBBox()[3]) / ystep);
  yi1 = (int)floor((yMax - tPat->getBBox()[1]) / ystep) + 1;
  for (i = 0; i < 4; ++i) {
    m1[i] = m[i];
  }
  if (!contentIsHidden()) {
    m1[4] = m[4];
    m1[5] = m[5];
    if (out->useTilingPatternFill() &&
	out->tilingPatternFill(state, tPat->getContentStream(),
			       tPat->getPaintType(), tPat->getResDict(),
			       m1, tPat->getBBox(),
			       xi0, yi0, xi1, yi1, xstep, ystep)) {
	    goto restore;
    } else {
      for (yi = yi0; yi < yi1; ++yi) {
        for (xi = xi0; xi < xi1; ++xi) {
	  x = xi * xstep;
	  y = yi * ystep;
	  m1[4] = x * m[0] + y * m[2] + m[4];
	  m1[5] = x * m[1] + y * m[3] + m[5];
	  doForm1(tPat->getContentStream(), tPat->getResDict(),
		  m1, tPat->getBBox());
	}
      }
    }
  }

  // restore graphics state
 restore:
  restoreState();
  state->setPath(savedPath);
}