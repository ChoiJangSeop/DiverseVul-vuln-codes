cmsBool  PatchLUT(cmsStage* CLUT, cmsUInt16Number At[], cmsUInt16Number Value[],
                  int nChannelsOut, int nChannelsIn)
{
    _cmsStageCLutData* Grid = (_cmsStageCLutData*) CLUT ->Data;
    cmsInterpParams* p16  = Grid ->Params;
    cmsFloat64Number px, py, pz, pw;
    int        x0, y0, z0, w0;
    int        i, index;

    if (CLUT -> Type != cmsSigCLutElemType) {
        cmsSignalError(CLUT->ContextID, cmsERROR_INTERNAL, "(internal) Attempt to PatchLUT on non-lut MPE");
        return FALSE;
    }

    if (nChannelsIn == 4) {

        px = ((cmsFloat64Number) At[0] * (p16->Domain[0])) / 65535.0;
        py = ((cmsFloat64Number) At[1] * (p16->Domain[1])) / 65535.0;
        pz = ((cmsFloat64Number) At[2] * (p16->Domain[2])) / 65535.0;
        pw = ((cmsFloat64Number) At[3] * (p16->Domain[3])) / 65535.0;

        x0 = (int) floor(px);
        y0 = (int) floor(py);
        z0 = (int) floor(pz);
        w0 = (int) floor(pw);

        if (((px - x0) != 0) ||
            ((py - y0) != 0) ||
            ((pz - z0) != 0) ||
            ((pw - w0) != 0)) return FALSE; // Not on exact node

        index = p16 -> opta[3] * x0 +
                p16 -> opta[2] * y0 +
                p16 -> opta[1] * z0 +
                p16 -> opta[0] * w0;
    }
    else
        if (nChannelsIn == 3) {

            px = ((cmsFloat64Number) At[0] * (p16->Domain[0])) / 65535.0;
            py = ((cmsFloat64Number) At[1] * (p16->Domain[1])) / 65535.0;
            pz = ((cmsFloat64Number) At[2] * (p16->Domain[2])) / 65535.0;
           
            x0 = (int) floor(px);
            y0 = (int) floor(py);
            z0 = (int) floor(pz);
           
            if (((px - x0) != 0) ||
                ((py - y0) != 0) ||
                ((pz - z0) != 0)) return FALSE;  // Not on exact node

            index = p16 -> opta[2] * x0 +
                    p16 -> opta[1] * y0 +
                    p16 -> opta[0] * z0;
        }
        else
            if (nChannelsIn == 1) {

                px = ((cmsFloat64Number) At[0] * (p16->Domain[0])) / 65535.0;
                
                x0 = (int) floor(px);
                
                if (((px - x0) != 0)) return FALSE; // Not on exact node

                index = p16 -> opta[0] * x0;
            }
            else {
                cmsSignalError(CLUT->ContextID, cmsERROR_INTERNAL, "(internal) %d Channels are not supported on PatchLUT", nChannelsIn);
                return FALSE;
            }

            for (i=0; i < nChannelsOut; i++)
                Grid -> Tab.T[index + i] = Value[i];

            return TRUE;
}