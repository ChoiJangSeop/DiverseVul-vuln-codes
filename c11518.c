void delete_run_files(pid_t pid) {
	delete_bandwidth_run_file(pid);
	delete_network_run_file(pid);
	delete_name_run_file(pid);
	delete_x11_run_file(pid);
	delete_profile_run_file(pid);
}