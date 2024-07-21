SYSCALL_DEFINE5(prctl, int, option, unsigned long, arg2, unsigned long, arg3,
		unsigned long, arg4, unsigned long, arg5)
{
	struct task_struct *me = current;
	unsigned char comm[sizeof(me->comm)];
	long error;

	error = security_task_prctl(option, arg2, arg3, arg4, arg5);
	if (error != -ENOSYS)
		return error;

	error = 0;
	switch (option) {
		case PR_SET_PDEATHSIG:
			if (!valid_signal(arg2)) {
				error = -EINVAL;
				break;
			}
			me->pdeath_signal = arg2;
			error = 0;
			break;
		case PR_GET_PDEATHSIG:
			error = put_user(me->pdeath_signal, (int __user *)arg2);
			break;
		case PR_GET_DUMPABLE:
			error = get_dumpable(me->mm);
			break;
		case PR_SET_DUMPABLE:
			if (arg2 < 0 || arg2 > 1) {
				error = -EINVAL;
				break;
			}
			set_dumpable(me->mm, arg2);
			error = 0;
			break;

		case PR_SET_UNALIGN:
			error = SET_UNALIGN_CTL(me, arg2);
			break;
		case PR_GET_UNALIGN:
			error = GET_UNALIGN_CTL(me, arg2);
			break;
		case PR_SET_FPEMU:
			error = SET_FPEMU_CTL(me, arg2);
			break;
		case PR_GET_FPEMU:
			error = GET_FPEMU_CTL(me, arg2);
			break;
		case PR_SET_FPEXC:
			error = SET_FPEXC_CTL(me, arg2);
			break;
		case PR_GET_FPEXC:
			error = GET_FPEXC_CTL(me, arg2);
			break;
		case PR_GET_TIMING:
			error = PR_TIMING_STATISTICAL;
			break;
		case PR_SET_TIMING:
			if (arg2 != PR_TIMING_STATISTICAL)
				error = -EINVAL;
			else
				error = 0;
			break;

		case PR_SET_NAME:
			comm[sizeof(me->comm)-1] = 0;
			if (strncpy_from_user(comm, (char __user *)arg2,
					      sizeof(me->comm) - 1) < 0)
				return -EFAULT;
			set_task_comm(me, comm);
			proc_comm_connector(me);
			return 0;
		case PR_GET_NAME:
			get_task_comm(comm, me);
			if (copy_to_user((char __user *)arg2, comm,
					 sizeof(comm)))
				return -EFAULT;
			return 0;
		case PR_GET_ENDIAN:
			error = GET_ENDIAN(me, arg2);
			break;
		case PR_SET_ENDIAN:
			error = SET_ENDIAN(me, arg2);
			break;

		case PR_GET_SECCOMP:
			error = prctl_get_seccomp();
			break;
		case PR_SET_SECCOMP:
			error = prctl_set_seccomp(arg2);
			break;
		case PR_GET_TSC:
			error = GET_TSC_CTL(arg2);
			break;
		case PR_SET_TSC:
			error = SET_TSC_CTL(arg2);
			break;
		case PR_TASK_PERF_EVENTS_DISABLE:
			error = perf_event_task_disable();
			break;
		case PR_TASK_PERF_EVENTS_ENABLE:
			error = perf_event_task_enable();
			break;
		case PR_GET_TIMERSLACK:
			error = current->timer_slack_ns;
			break;
		case PR_SET_TIMERSLACK:
			if (arg2 <= 0)
				current->timer_slack_ns =
					current->default_timer_slack_ns;
			else
				current->timer_slack_ns = arg2;
			error = 0;
			break;
		case PR_MCE_KILL:
			if (arg4 | arg5)
				return -EINVAL;
			switch (arg2) {
			case PR_MCE_KILL_CLEAR:
				if (arg3 != 0)
					return -EINVAL;
				current->flags &= ~PF_MCE_PROCESS;
				break;
			case PR_MCE_KILL_SET:
				current->flags |= PF_MCE_PROCESS;
				if (arg3 == PR_MCE_KILL_EARLY)
					current->flags |= PF_MCE_EARLY;
				else if (arg3 == PR_MCE_KILL_LATE)
					current->flags &= ~PF_MCE_EARLY;
				else if (arg3 == PR_MCE_KILL_DEFAULT)
					current->flags &=
						~(PF_MCE_EARLY|PF_MCE_PROCESS);
				else
					return -EINVAL;
				break;
			default:
				return -EINVAL;
			}
			error = 0;
			break;
		case PR_MCE_KILL_GET:
			if (arg2 | arg3 | arg4 | arg5)
				return -EINVAL;
			if (current->flags & PF_MCE_PROCESS)
				error = (current->flags & PF_MCE_EARLY) ?
					PR_MCE_KILL_EARLY : PR_MCE_KILL_LATE;
			else
				error = PR_MCE_KILL_DEFAULT;
			break;
		case PR_SET_MM:
			error = prctl_set_mm(arg2, arg3, arg4, arg5);
			break;
		case PR_SET_CHILD_SUBREAPER:
			me->signal->is_child_subreaper = !!arg2;
			error = 0;
			break;
		case PR_GET_CHILD_SUBREAPER:
			error = put_user(me->signal->is_child_subreaper,
					 (int __user *) arg2);
			break;
		default:
			error = -EINVAL;
			break;
	}
	return error;
}