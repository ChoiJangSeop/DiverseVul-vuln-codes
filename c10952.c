slapi_pblock_clone(Slapi_PBlock *pb)
{
    /*
     * This is used only in psearch, with an access pattern of
     * ['13:3', '191:28', '47:18', '2001:1', '115:3', '503:3', '196:10', '52:18', '1930:3', '133:189', '112:13', '57:1', '214:91', '70:93', '193:14', '49:10', '403:6', '117:2', '1001:1', '130:161', '109:2', '198:36', '510:27', '1945:3', '114:16', '4:11', '216:91', '712:11', '195:19', '51:26', '140:10', '2005:18', '9:2', '132:334', '111:1', '1160:1', '410:54', '48:1', '2002:156', '116:3', '1000:22', '53:27', '9999:1', '113:13', '3:132', '590:3', '215:91', '194:20', '118:1', '131:30', '860:2', '110:14', ]
     */
    Slapi_PBlock *new_pb = slapi_pblock_new();
    new_pb->pb_backend = pb->pb_backend;
    new_pb->pb_conn = pb->pb_conn;
    new_pb->pb_op = pb->pb_op;
    new_pb->pb_plugin = pb->pb_plugin;
    /* Perform a shallow copy. */
    if (pb->pb_dse != NULL) {
        _pblock_assert_pb_dse(new_pb);
        *(new_pb->pb_dse) = *(pb->pb_dse);
    }
    if (pb->pb_task != NULL) {
        _pblock_assert_pb_task(new_pb);
        *(new_pb->pb_task) = *(pb->pb_task);
    }
    if (pb->pb_mr != NULL) {
        _pblock_assert_pb_mr(new_pb);
        *(new_pb->pb_mr) = *(pb->pb_mr);
    }
    if (pb->pb_misc != NULL) {
        _pblock_assert_pb_misc(new_pb);
        *(new_pb->pb_misc) = *(pb->pb_misc);
    }
    if (pb->pb_intop != NULL) {
        _pblock_assert_pb_intop(new_pb);
        *(new_pb->pb_intop) = *(pb->pb_intop);
        /* set pwdpolicy to NULL so this clone allocates its own policy */
        new_pb->pb_intop->pwdpolicy = NULL;
    }
    if (pb->pb_intplugin != NULL) {
        _pblock_assert_pb_intplugin(new_pb);
        *(new_pb->pb_intplugin) = *(pb->pb_intplugin);
    }
    if (pb->pb_deprecated != NULL) {
        _pblock_assert_pb_deprecated(new_pb);
        *(new_pb->pb_deprecated) = *(pb->pb_deprecated);
    }
#ifdef PBLOCK_ANALYTICS
    new_pb->analytics = NULL;
    pblock_analytics_init(new_pb);
#endif
    return new_pb;
}