destroyUserInformationLists(DUL_USERINFO * userInfo)
{
    PRV_SCUSCPROLE
    * role;

    role = (PRV_SCUSCPROLE*)LST_Dequeue(&userInfo->SCUSCPRoleList);
    while (role != NULL) {
        free(role);
        role = (PRV_SCUSCPROLE*)LST_Dequeue(&userInfo->SCUSCPRoleList);
    }
    LST_Destroy(&userInfo->SCUSCPRoleList);

    /* extended negotiation */
    delete userInfo->extNegList; userInfo->extNegList = NULL;

    /* user identity negotiation */
    delete userInfo->usrIdent; userInfo->usrIdent = NULL;
}