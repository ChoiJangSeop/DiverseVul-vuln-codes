void php_wddx_packet_end(wddx_packet *packet)
{
	php_wddx_add_chunk_static(packet, WDDX_DATA_E);
	php_wddx_add_chunk_static(packet, WDDX_PACKET_E);	
}