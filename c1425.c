cmsToneCurve* AllocateToneCurveStruct(cmsContext ContextID, cmsInt32Number nEntries,
                                      cmsInt32Number nSegments, const cmsCurveSegment* Segments,
                                      const cmsUInt16Number* Values)
{
    cmsToneCurve* p;
    int i;

    // We allow huge tables, which are then restricted for smoothing operations
    if (nEntries > 65530 || nEntries < 0) {
        cmsSignalError(ContextID, cmsERROR_RANGE, "Couldn't create tone curve of more than 65530 entries");
        return NULL;
    }

    if (nEntries <= 0 && nSegments <= 0) {
        cmsSignalError(ContextID, cmsERROR_RANGE, "Couldn't create tone curve with zero segments and no table");
        return NULL;
    }

    // Allocate all required pointers, etc.
    p = (cmsToneCurve*) _cmsMallocZero(ContextID, sizeof(cmsToneCurve));
    if (!p) return NULL;

    // In this case, there are no segments
    if (nSegments <= 0) {
        p ->Segments = NULL;
        p ->Evals = NULL;
    }
    else {
        p ->Segments = (cmsCurveSegment*) _cmsCalloc(ContextID, nSegments, sizeof(cmsCurveSegment));
        if (p ->Segments == NULL) goto Error;

        p ->Evals    = (cmsParametricCurveEvaluator*) _cmsCalloc(ContextID, nSegments, sizeof(cmsParametricCurveEvaluator));
        if (p ->Evals == NULL) goto Error;
    }

    p -> nSegments = nSegments;

    // This 16-bit table contains a limited precision representation of the whole curve and is kept for
    // increasing xput on certain operations.
    if (nEntries <= 0) {
        p ->Table16 = NULL;
    }
    else {
       p ->Table16 = (cmsUInt16Number*)  _cmsCalloc(ContextID, nEntries, sizeof(cmsUInt16Number));
       if (p ->Table16 == NULL) goto Error;
    }

    p -> nEntries  = nEntries;

    // Initialize members if requested
    if (Values != NULL && (nEntries > 0)) {

        for (i=0; i < nEntries; i++)
            p ->Table16[i] = Values[i];
    }

    // Initialize the segments stuff. The evaluator for each segment is located and a pointer to it
    // is placed in advance to maximize performance.
    if (Segments != NULL && (nSegments > 0)) {

        _cmsParametricCurvesCollection *c;

        p ->SegInterp = (cmsInterpParams**) _cmsCalloc(ContextID, nSegments, sizeof(cmsInterpParams*));
        if (p ->SegInterp == NULL) goto Error;

        for (i=0; i< nSegments; i++) {

            // Type 0 is a special marker for table-based curves
            if (Segments[i].Type == 0)
                p ->SegInterp[i] = _cmsComputeInterpParams(ContextID, Segments[i].nGridPoints, 1, 1, NULL, CMS_LERP_FLAGS_FLOAT);

            memmove(&p ->Segments[i], &Segments[i], sizeof(cmsCurveSegment));

            if (Segments[i].Type == 0 && Segments[i].SampledPoints != NULL)
                p ->Segments[i].SampledPoints = (cmsFloat32Number*) _cmsDupMem(ContextID, Segments[i].SampledPoints, sizeof(cmsFloat32Number) * Segments[i].nGridPoints);
            else
                p ->Segments[i].SampledPoints = NULL;


            c = GetParametricCurveByType(Segments[i].Type, NULL);
            if (c != NULL)
                    p ->Evals[i] = c ->Evaluator;
        }
    }

    p ->InterpParams = _cmsComputeInterpParams(ContextID, p ->nEntries, 1, 1, p->Table16, CMS_LERP_FLAGS_16BITS);
    return p;

Error:
    if (p -> Segments) _cmsFree(ContextID, p ->Segments);
    if (p -> Evals) _cmsFree(ContextID, p -> Evals);
    if (p ->Table16) _cmsFree(ContextID, p ->Table16);
    _cmsFree(ContextID, p);
    return NULL;
}