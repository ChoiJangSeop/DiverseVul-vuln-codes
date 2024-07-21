GBool PSOutputDev::tilingPatternFill(GfxState *state, Object *str,
				     int paintType, Dict *resDict,
				     double *mat, double *bbox,
				     int x0, int y0, int x1, int y1,
				     double xStep, double yStep) {
  PDFRectangle box;
  Gfx *gfx;

  // define a Type 3 font
  writePS("8 dict begin\n");
  writePS("/FontType 3 def\n");
  writePS("/FontMatrix [1 0 0 1 0 0] def\n");
  writePSFmt("/FontBBox [{0:.6g} {1:.6g} {2:.6g} {3:.6g}] def\n",
	     bbox[0], bbox[1], bbox[2], bbox[3]);
  writePS("/Encoding 256 array def\n");
  writePS("  0 1 255 { Encoding exch /.notdef put } for\n");
  writePS("  Encoding 120 /x put\n");
  writePS("/BuildGlyph {\n");
  writePS("  exch /CharProcs get exch\n");
  writePS("  2 copy known not { pop /.notdef } if\n");
  writePS("  get exec\n");
  writePS("} bind def\n");
  writePS("/BuildChar {\n");
  writePS("  1 index /Encoding get exch get\n");
  writePS("  1 index /BuildGlyph get exec\n");
  writePS("} bind def\n");
  writePS("/CharProcs 1 dict def\n");
  writePS("CharProcs begin\n");
  box.x1 = bbox[0];
  box.y1 = bbox[1];
  box.x2 = bbox[2];
  box.y2 = bbox[3];
  gfx = new Gfx(xref, this, resDict, m_catalog, &box, NULL);
  writePS("/x {\n");
  if (paintType == 2) {
    writePSFmt("{0:.6g} 0 {1:.6g} {2:.6g} {3:.6g} {4:.6g} setcachedevice\n",
	       xStep, bbox[0], bbox[1], bbox[2], bbox[3]);
  } else
  {
    if (x1 - 1 <= x0) {
      writePS("1 0 setcharwidth\n");
    } else {
      writePSFmt("{0:.6g} 0 setcharwidth\n", xStep);
    }
  }
  inType3Char = gTrue;
  ++numTilingPatterns;
  gfx->display(str);
  --numTilingPatterns;
  inType3Char = gFalse;
  writePS("} def\n");
  delete gfx;
  writePS("end\n");
  writePS("currentdict end\n");
  writePSFmt("/xpdfTile{0:d} exch definefont pop\n", numTilingPatterns);

  // draw the tiles
  writePSFmt("/xpdfTile{0:d} findfont setfont\n", numTilingPatterns);
  writePSFmt("gsave [{0:.6g} {1:.6g} {2:.6g} {3:.6g} {4:.6g} {5:.6g}] concat\n",
	     mat[0], mat[1], mat[2], mat[3], mat[4], mat[5]);
  writePSFmt("{0:d} 1 {1:d} {{ {2:.6g} exch {3:.6g} mul m {4:d} 1 {5:d} {{ pop (x) show }} for }} for\n",
	     y0, y1 - 1, x0 * xStep, yStep, x0, x1 - 1);
  writePS("grestore\n");

  return gTrue;
}