int enter_svm_guest_mode(struct vcpu_svm *svm, u64 vmcb12_gpa,
			 struct vmcb *vmcb12)
{
	int ret;

	svm->nested.vmcb12_gpa = vmcb12_gpa;
	load_nested_vmcb_control(svm, &vmcb12->control);
	nested_prepare_vmcb_save(svm, vmcb12);
	nested_prepare_vmcb_control(svm);

	ret = nested_svm_load_cr3(&svm->vcpu, vmcb12->save.cr3,
				  nested_npt_enabled(svm));
	if (ret)
		return ret;

	svm_set_gif(svm, true);

	return 0;
}