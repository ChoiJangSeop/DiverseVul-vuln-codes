static netdev_features_t bnx2x_features_check(struct sk_buff *skb,
					      struct net_device *dev,
					      netdev_features_t features)
{
	features = vlan_features_check(skb, features);
	return vxlan_features_check(skb, features);
}