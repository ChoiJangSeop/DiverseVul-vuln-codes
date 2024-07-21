static int lsig_eval(cli_ctx *ctx, struct cli_matcher *root, struct cli_ac_data *acdata, struct cli_target_info *target_info, const char *hash, uint32_t lsid)
{
    unsigned evalcnt = 0;
    uint64_t evalids = 0;
    fmap_t *map = *ctx->fmap;
    struct cli_ac_lsig *ac_lsig = root->ac_lsigtable[lsid];
    char * exp = ac_lsig->u.logic;
    char* exp_end = exp + strlen(exp);
    int rc;

    rc = cli_ac_chkmacro(root, acdata, lsid);
    if (rc != CL_SUCCESS)
        return rc;
    if (cli_ac_chklsig(exp, exp_end, acdata->lsigcnt[lsid], &evalcnt, &evalids, 0) == 1) {
        if(ac_lsig->tdb.container && ac_lsig->tdb.container[0] != cli_get_container_type(ctx, -1))
            return CL_CLEAN;
        if(ac_lsig->tdb.intermediates && !intermediates_eval(ctx, ac_lsig))
            return CL_CLEAN;
        if(ac_lsig->tdb.filesize && (ac_lsig->tdb.filesize[0] > map->len || ac_lsig->tdb.filesize[1] < map->len))
            return CL_CLEAN;

        if(ac_lsig->tdb.ep || ac_lsig->tdb.nos) {
            if(!target_info || target_info->status != 1)
                return CL_CLEAN;
            if(ac_lsig->tdb.ep && (ac_lsig->tdb.ep[0] > target_info->exeinfo.ep || ac_lsig->tdb.ep[1] < target_info->exeinfo.ep))
                return CL_CLEAN;
            if(ac_lsig->tdb.nos && (ac_lsig->tdb.nos[0] > target_info->exeinfo.nsections || ac_lsig->tdb.nos[1] < target_info->exeinfo.nsections))
                return CL_CLEAN;
        }

        if(hash && ac_lsig->tdb.handlertype) {
            if(memcmp(ctx->handlertype_hash, hash, 16)) {
                ctx->recursion++;
                memcpy(ctx->handlertype_hash, hash, 16);
                if(cli_magic_scandesc_type(ctx, ac_lsig->tdb.handlertype[0]) == CL_VIRUS) {
                    ctx->recursion--;
                    return CL_VIRUS;
                }
                ctx->recursion--;
                return CL_CLEAN;
            }
        }
        
        if(ac_lsig->tdb.icongrp1 || ac_lsig->tdb.icongrp2) {
            if(!target_info || target_info->status != 1)
                return CL_CLEAN;
            if(matchicon(ctx, &target_info->exeinfo, ac_lsig->tdb.icongrp1, ac_lsig->tdb.icongrp2) == CL_VIRUS) {
                if(!ac_lsig->bc_idx) {
                    rc = cli_append_virus(ctx, ac_lsig->virname);
                    if (rc != CL_CLEAN)
                        return rc;
                } else if(cli_bytecode_runlsig(ctx, target_info, &ctx->engine->bcs, ac_lsig->bc_idx, acdata->lsigcnt[lsid], acdata->lsigsuboff_first[lsid], map) == CL_VIRUS) {
                    return CL_VIRUS;
                }
            }
            return CL_CLEAN;
        }
        if(!ac_lsig->bc_idx) {
            rc = cli_append_virus(ctx, ac_lsig->virname);
            if (rc != CL_CLEAN)
                return rc;
        }
        if(cli_bytecode_runlsig(ctx, target_info, &ctx->engine->bcs, ac_lsig->bc_idx, acdata->lsigcnt[lsid], acdata->lsigsuboff_first[lsid], map) == CL_VIRUS) {
            return CL_VIRUS;
        }
    }
    
    return CL_CLEAN;
}