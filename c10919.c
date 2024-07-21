static bool bpf_map_is_rdonly(const struct bpf_map *map)
{
	return (map->map_flags & BPF_F_RDONLY_PROG) && map->frozen;
}