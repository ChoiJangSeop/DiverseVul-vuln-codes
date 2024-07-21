int sqlite3ResolveExprNames( 
  NameContext *pNC,       /* Namespace to resolve expressions in. */
  Expr *pExpr             /* The expression to be analyzed. */
){
  u16 savedHasAgg;
  Walker w;

  if( pExpr==0 ) return SQLITE_OK;
  savedHasAgg = pNC->ncFlags & (NC_HasAgg|NC_MinMaxAgg);
  pNC->ncFlags &= ~(NC_HasAgg|NC_MinMaxAgg);
  w.pParse = pNC->pParse;
  w.xExprCallback = resolveExprStep;
  w.xSelectCallback = resolveSelectStep;
  w.xSelectCallback2 = 0;
  w.u.pNC = pNC;
#if SQLITE_MAX_EXPR_DEPTH>0
  w.pParse->nHeight += pExpr->nHeight;
  if( sqlite3ExprCheckHeight(w.pParse, w.pParse->nHeight) ){
    return SQLITE_ERROR;
  }
#endif
  sqlite3WalkExpr(&w, pExpr);
#if SQLITE_MAX_EXPR_DEPTH>0
  w.pParse->nHeight -= pExpr->nHeight;
#endif
  if( pNC->ncFlags & NC_HasAgg ){
    ExprSetProperty(pExpr, EP_Agg);
  }
  pNC->ncFlags |= savedHasAgg;
  return pNC->nErr>0 || w.pParse->nErr>0;
}