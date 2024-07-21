NOEXPORT int parse_socket_error(CLI *c, const char *text) {
    switch(get_last_socket_error()) {
        /* http://tangentsoft.net/wskfaq/articles/bsd-compatibility.html */
    case 0: /* close on read, or close on write on WIN32 */
#ifndef USE_WIN32
    case EPIPE: /* close on write on Unix */
#endif
    case S_ECONNABORTED:
        s_log(LOG_INFO, "%s: Socket is closed", text);
        return 0;
    case S_EINTR:
        s_log(LOG_DEBUG, "%s: Interrupted by a signal: retrying", text);
        return 1;
    case S_EWOULDBLOCK:
        s_log(LOG_NOTICE, "%s: Would block: retrying", text);
        s_poll_sleep(1, 0); /* Microsoft bug KB177346 */
        return 1;
#if S_EAGAIN!=S_EWOULDBLOCK
    case S_EAGAIN:
        s_log(LOG_DEBUG,
            "%s: Temporary lack of resources: retrying", text);
        return 1;
#endif
#ifdef USE_WIN32
    case S_ECONNRESET:
        /* dying "exec" processes on Win32 cause reset instead of close */
        if(c->opt->exec_name) {
            s_log(LOG_INFO, "%s: Socket is closed (exec)", text);
            return 0;
        }
        /* fall through */
#endif
    default:
        sockerror(text);
        throw_exception(c, 1);
        return -1; /* some C compilers require a return value */
    }
}