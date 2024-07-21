destroyPresentationContextList(LST_HEAD ** lst)
{
    DUL_PRESENTATIONCONTEXT *pc;
    DUL_TRANSFERSYNTAX *ts;

    if ((lst == NULL) || (*lst == NULL))
        return;
    while ((pc = (DUL_PRESENTATIONCONTEXT*) LST_Dequeue(lst)) != NULL) {
        if (pc->proposedTransferSyntax != NULL) {
            while ((ts = (DUL_TRANSFERSYNTAX*) LST_Dequeue(&pc->proposedTransferSyntax)) != NULL) {
                free(ts);
            }
            LST_Destroy(&pc->proposedTransferSyntax);
        }
        free(pc);
    }
    LST_Destroy(lst);
}