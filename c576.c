processBatchMultiRuleset(batch_t *pBatch)
{
	ruleset_t *currRuleset;
	batch_t snglRuleBatch;
	int i;
	int iStart;	/* start index of partial batch */
	int iNew;	/* index for new (temporary) batch */
	DEFiRet;

	CHKiRet(batchInit(&snglRuleBatch, pBatch->nElem));
	snglRuleBatch.pbShutdownImmediate = pBatch->pbShutdownImmediate;

	while(1) { /* loop broken inside */
		/* search for first unprocessed element */
		for(iStart = 0 ; iStart < pBatch->nElem && pBatch->pElem[iStart].state == BATCH_STATE_DISC ; ++iStart)
			/* just search, no action */;

		if(iStart == pBatch->nElem)
			FINALIZE; /* everything processed */

		/* prepare temporary batch */
		currRuleset = batchElemGetRuleset(pBatch, iStart);
		iNew = 0;
		for(i = iStart ; i < pBatch->nElem ; ++i) {
			if(batchElemGetRuleset(pBatch, i) == currRuleset) {
				batchCopyElem(&(snglRuleBatch.pElem[iNew++]), &(pBatch->pElem[i]));
				/* We indicate the element also as done, so it will not be processed again */
				pBatch->pElem[i].state = BATCH_STATE_DISC;
			}
		}
		snglRuleBatch.nElem = iNew; /* was left just right by the for loop */
		batchSetSingleRuleset(&snglRuleBatch, 1);
		/* process temp batch */
		processBatch(&snglRuleBatch);
	}
	batchFree(&snglRuleBatch);

finalize_it:
	RETiRet;
}