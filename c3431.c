proto_config(
	int	item,
	u_long	value,
	double	dvalue,
	sockaddr_u *svalue
	)
{
	/*
	 * Figure out what he wants to change, then do it
	 */
	DPRINTF(2, ("proto_config: code %d value %lu dvalue %lf\n",
		    item, value, dvalue));

	switch (item) {

	/*
	 * enable and disable commands - arguments are Boolean.
	 */
	case PROTO_AUTHENTICATE: /* authentication (auth) */
		sys_authenticate = value;
		break;

	case PROTO_BROADCLIENT: /* broadcast client (bclient) */
		sys_bclient = (int)value;
		if (sys_bclient == 0)
			io_unsetbclient();
		else
			io_setbclient();
		break;

#ifdef REFCLOCK
	case PROTO_CAL:		/* refclock calibrate (calibrate) */
		cal_enable = value;
		break;
#endif /* REFCLOCK */

	case PROTO_KERNEL:	/* kernel discipline (kernel) */
		select_loop(value);
		break;

	case PROTO_MONITOR:	/* monitoring (monitor) */
		if (value)
			mon_start(MON_ON);
		else {
			mon_stop(MON_ON);
			if (mon_enabled)
				msyslog(LOG_WARNING,
					"restrict: 'monitor' cannot be disabled while 'limited' is enabled");
		}
		break;

	case PROTO_NTP:		/* NTP discipline (ntp) */
		ntp_enable = value;
		break;

	case PROTO_MODE7:	/* mode7 management (ntpdc) */
		ntp_mode7 = value;
		break;

	case PROTO_PPS:		/* PPS discipline (pps) */
		hardpps_enable = value;
		break;

	case PROTO_FILEGEN:	/* statistics (stats) */
		stats_control = value;
		break;

	/*
	 * tos command - arguments are double, sometimes cast to int
	 */
	case PROTO_BEACON:	/* manycast beacon (beacon) */
		sys_beacon = (int)dvalue;
		break;

	case PROTO_BROADDELAY:	/* default broadcast delay (bdelay) */
		sys_bdelay = dvalue;
		break;

	case PROTO_CEILING:	/* stratum ceiling (ceiling) */
		sys_ceiling = (int)dvalue;
		break;

	case PROTO_COHORT:	/* cohort switch (cohort) */
		sys_cohort = (int)dvalue;
		break;

	case PROTO_FLOOR:	/* stratum floor (floor) */
		sys_floor = (int)dvalue;
		break;

	case PROTO_MAXCLOCK:	/* maximum candidates (maxclock) */
		sys_maxclock = (int)dvalue;
		break;

	case PROTO_MAXDIST:	/* select threshold (maxdist) */
		sys_maxdist = dvalue;
		break;

	case PROTO_CALLDELAY:	/* modem call delay (mdelay) */
		break;		/* NOT USED */

	case PROTO_MINCLOCK:	/* minimum candidates (minclock) */
		sys_minclock = (int)dvalue;
		break;

	case PROTO_MINDISP:	/* minimum distance (mindist) */
		sys_mindisp = dvalue;
		break;

	case PROTO_MINSANE:	/* minimum survivors (minsane) */
		sys_minsane = (int)dvalue;
		break;

	case PROTO_ORPHAN:	/* orphan stratum (orphan) */
		sys_orphan = (int)dvalue;
		break;

	case PROTO_ORPHWAIT:	/* orphan wait (orphwait) */
		orphwait -= sys_orphwait;
		sys_orphwait = (int)dvalue;
		orphwait += sys_orphwait;
		break;

	/*
	 * Miscellaneous commands
	 */
	case PROTO_MULTICAST_ADD: /* add group address */
		if (svalue != NULL)
			io_multicast_add(svalue);
		sys_bclient = 1;
		break;

	case PROTO_MULTICAST_DEL: /* delete group address */
		if (svalue != NULL)
			io_multicast_del(svalue);
		break;

	default:
		msyslog(LOG_NOTICE,
		    "proto: unsupported option %d", item);
	}
}