void derive_boundaryStrength(de265_image* img, bool vertical, int yStart,int yEnd,
                             int xStart,int xEnd)
{
  int xIncr = vertical ? 2 : 1;
  int yIncr = vertical ? 1 : 2;
  int xOffs = vertical ? 1 : 0;
  int yOffs = vertical ? 0 : 1;
  int edgeMask = vertical ?
    (DEBLOCK_FLAG_VERTI | DEBLOCK_PB_EDGE_VERTI) :
    (DEBLOCK_FLAG_HORIZ | DEBLOCK_PB_EDGE_HORIZ);
  int transformEdgeMask = vertical ? DEBLOCK_FLAG_VERTI : DEBLOCK_FLAG_HORIZ;

  xEnd = libde265_min(xEnd,img->get_deblk_width());
  yEnd = libde265_min(yEnd,img->get_deblk_height());

  int TUShift = img->get_sps().Log2MinTrafoSize;
  int TUStride= img->get_sps().PicWidthInTbsY;

  for (int y=yStart;y<yEnd;y+=yIncr)
    for (int x=xStart;x<xEnd;x+=xIncr) {
      int xDi = x<<2;
      int yDi = y<<2;

      logtrace(LogDeblock,"%d %d %s = %s\n",xDi,yDi, vertical?"Vertical":"Horizontal",
               (img->get_deblk_flags(xDi,yDi) & edgeMask) ? "edge" : "...");

      uint8_t edgeFlags = img->get_deblk_flags(xDi,yDi);

      if (edgeFlags & edgeMask) {
        bool p_is_intra_pred = (img->get_pred_mode(xDi-xOffs, yDi-yOffs) == MODE_INTRA);
        bool q_is_intra_pred = (img->get_pred_mode(xDi,       yDi      ) == MODE_INTRA);

        int bS;

        if (p_is_intra_pred || q_is_intra_pred) {
          bS = 2;
        }
        else {
          // opposing site
          int xDiOpp = xDi-xOffs;
          int yDiOpp = yDi-yOffs;

          if ((edgeFlags & transformEdgeMask) &&
              (img->get_nonzero_coefficient(xDi   ,yDi) ||
               img->get_nonzero_coefficient(xDiOpp,yDiOpp))) {
            bS = 1;
          }
          else {

            bS = 0;

            const PBMotion& mviP = img->get_mv_info(xDiOpp,yDiOpp);
            const PBMotion& mviQ = img->get_mv_info(xDi   ,yDi);

            slice_segment_header* shdrP = img->get_SliceHeader(xDiOpp,yDiOpp);
            slice_segment_header* shdrQ = img->get_SliceHeader(xDi   ,yDi);

            int refPicP0 = mviP.predFlag[0] ? shdrP->RefPicList[0][ mviP.refIdx[0] ] : -1;
            int refPicP1 = mviP.predFlag[1] ? shdrP->RefPicList[1][ mviP.refIdx[1] ] : -1;
            int refPicQ0 = mviQ.predFlag[0] ? shdrQ->RefPicList[0][ mviQ.refIdx[0] ] : -1;
            int refPicQ1 = mviQ.predFlag[1] ? shdrQ->RefPicList[1][ mviQ.refIdx[1] ] : -1;

            bool samePics = ((refPicP0==refPicQ0 && refPicP1==refPicQ1) ||
                             (refPicP0==refPicQ1 && refPicP1==refPicQ0));

            if (!samePics) {
              bS = 1;
            }
            else {
              MotionVector mvP0 = mviP.mv[0]; if (!mviP.predFlag[0]) { mvP0.x=mvP0.y=0; }
              MotionVector mvP1 = mviP.mv[1]; if (!mviP.predFlag[1]) { mvP1.x=mvP1.y=0; }
              MotionVector mvQ0 = mviQ.mv[0]; if (!mviQ.predFlag[0]) { mvQ0.x=mvQ0.y=0; }
              MotionVector mvQ1 = mviQ.mv[1]; if (!mviQ.predFlag[1]) { mvQ1.x=mvQ1.y=0; }

              int numMV_P = mviP.predFlag[0] + mviP.predFlag[1];
              int numMV_Q = mviQ.predFlag[0] + mviQ.predFlag[1];

              if (numMV_P!=numMV_Q) {
                img->decctx->add_warning(DE265_WARNING_NUMMVP_NOT_EQUAL_TO_NUMMVQ, false);
                img->integrity = INTEGRITY_DECODING_ERRORS;
              }

              // two different reference pictures or only one reference picture
              if (refPicP0 != refPicP1) {

                if (refPicP0 == refPicQ0) {
                  if (abs_value(mvP0.x-mvQ0.x) >= 4 ||
                      abs_value(mvP0.y-mvQ0.y) >= 4 ||
                      abs_value(mvP1.x-mvQ1.x) >= 4 ||
                      abs_value(mvP1.y-mvQ1.y) >= 4) {
                    bS = 1;
                  }
                }
                else {
                  if (abs_value(mvP0.x-mvQ1.x) >= 4 ||
                      abs_value(mvP0.y-mvQ1.y) >= 4 ||
                      abs_value(mvP1.x-mvQ0.x) >= 4 ||
                      abs_value(mvP1.y-mvQ0.y) >= 4) {
                    bS = 1;
                  }
                }
              }
              else {
                assert(refPicQ0==refPicQ1);

                if ((abs_value(mvP0.x-mvQ0.x) >= 4 ||
                     abs_value(mvP0.y-mvQ0.y) >= 4 ||
                     abs_value(mvP1.x-mvQ1.x) >= 4 ||
                     abs_value(mvP1.y-mvQ1.y) >= 4)
                    &&
                    (abs_value(mvP0.x-mvQ1.x) >= 4 ||
                     abs_value(mvP0.y-mvQ1.y) >= 4 ||
                     abs_value(mvP1.x-mvQ0.x) >= 4 ||
                     abs_value(mvP1.y-mvQ0.y) >= 4)) {
                  bS = 1;
                }
              }
            }

            /*
              printf("unimplemented deblocking code for CU at %d;%d\n",xDi,yDi);

              logerror(LogDeblock, "unimplemented code reached (file %s, line %d)\n",
              __FILE__, __LINE__);
            */
          }
        }

        img->set_deblk_bS(xDi,yDi, bS);
      }
      else {
        img->set_deblk_bS(xDi,yDi, 0);
      }
    }
}