ASC_destroyAssociation(T_ASC_Association ** association)
{
    OFCondition cond = EC_Normal;

    /* don't worry if already destroyed */
    if (association == NULL) return EC_Normal;
    if (*association == NULL) return EC_Normal;

    if ((*association)->DULassociation != NULL) {
        ASC_dropAssociation(*association);
    }

    if ((*association)->params != NULL) {
        cond = ASC_destroyAssociationParameters(&(*association)->params);
        if (cond.bad()) return cond;
    }

    if ((*association)->sendPDVBuffer != NULL)
        free((*association)->sendPDVBuffer);

    free(*association);
    *association = NULL;

    return EC_Normal;
}