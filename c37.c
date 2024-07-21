int is_bad_inode(struct inode * inode) 
{
	return (inode->i_op == &bad_inode_ops);	
}