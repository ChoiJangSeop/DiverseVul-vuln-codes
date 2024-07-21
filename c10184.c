writeRead (const std::string &tempDir,
           const Array2D<unsigned int> &pi,
           const Array2D<half> &ph,
           const Array2D<float> &pf,
           int W,
           int H,
           LineOrder lorder,
           Compression comp,
	   LevelRoundingMode rmode,
           int dx, int dy,
           int xSize, int ySize)
{
    std::string filename = tempDir + "imf_test_scanline_api.exr";

    writeRead (pi, ph, pf, filename.c_str(), lorder, W, H,
               xSize, ySize, dx, dy, comp, ONE_LEVEL, rmode);
    writeRead (pi, ph, pf, filename.c_str(), lorder, W, H,
               xSize, ySize, dx, dy, comp, MIPMAP_LEVELS, rmode);
    writeRead (pi, ph, pf, filename.c_str(), lorder, W, H,
               xSize, ySize, dx, dy, comp, RIPMAP_LEVELS, rmode);
}