void ring_buffer_reset_online_cpus(struct trace_buffer *buffer)
{
	struct ring_buffer_per_cpu *cpu_buffer;
	int cpu;

	for_each_online_buffer_cpu(buffer, cpu) {
		cpu_buffer = buffer->buffers[cpu];

		atomic_inc(&cpu_buffer->resize_disabled);
		atomic_inc(&cpu_buffer->record_disabled);
	}

	/* Make sure all commits have finished */
	synchronize_rcu();

	for_each_online_buffer_cpu(buffer, cpu) {
		cpu_buffer = buffer->buffers[cpu];

		reset_disabled_cpu_buffer(cpu_buffer);

		atomic_dec(&cpu_buffer->record_disabled);
		atomic_dec(&cpu_buffer->resize_disabled);
	}
}