static int selinux_msg_queue_msgrcv(struct kern_ipc_perm *msq, struct msg_msg *msg,
				    struct task_struct *target,
				    long type, int mode)
{
	struct ipc_security_struct *isec;
	struct msg_security_struct *msec;
	struct common_audit_data ad;
	u32 sid = task_sid_subj(target);
	int rc;

	isec = selinux_ipc(msq);
	msec = selinux_msg_msg(msg);

	ad.type = LSM_AUDIT_DATA_IPC;
	ad.u.ipc_id = msq->key;

	rc = avc_has_perm(&selinux_state,
			  sid, isec->sid,
			  SECCLASS_MSGQ, MSGQ__READ, &ad);
	if (!rc)
		rc = avc_has_perm(&selinux_state,
				  sid, msec->sid,
				  SECCLASS_MSG, MSG__RECEIVE, &ad);
	return rc;
}