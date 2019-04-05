/*
 * aQuantia Corporation Network Driver
 * Copyright (C) 2014-2017 aQuantia Corporation. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

/* File aq_main.c: Main file for aQuantia Linux driver. */

#include "aq_main.h"
#include "aq_nic.h"
#include "aq_pci_func.h"
#include "aq_ethtool.h"
#include "aq_drvinfo.h"
#ifndef __VMKLNX__
#include "aq_ptp.h"
#endif
#include "aq_filters.h"

#include <linux/netdevice.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/stat.h>
#include <linux/string.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
#include <uapi/linux/stat.h>
#endif
#include <linux/ip.h>
#include <linux/udp.h>

MODULE_LICENSE("GPL v2");
MODULE_VERSION(AQ_CFG_DRV_VERSION);
MODULE_AUTHOR(AQ_CFG_DRV_AUTHOR);
MODULE_DESCRIPTION(AQ_CFG_DRV_DESC);

const char aq_ndev_driver_name[] = AQ_CFG_DRV_NAME;

#ifdef __VMKLNX__
static int aq_ndev_open(struct net_device *ndev);
static int aq_ndev_close(struct net_device *ndev);
static int aq_ndev_start_xmit(struct sk_buff *skb, struct net_device *ndev);
static void aq_ndev_set_multicast_settings(struct net_device *ndev);
static int aq_ndev_change_mtu(struct net_device *ndev, int new_mtu);
static int aq_ndev_set_mac_address(struct net_device *ndev, void *addr);
static void aq_ndev_tx_timeout(struct net_device *ndev);
static void aq_ndo_vmk_vlan_rx_add_vid(struct net_device *ndev, u16 vid);
static void aq_ndo_vmk_vlan_rx_kill_vid(struct net_device *ndev, u16 vid);
#else
static const struct net_device_ops aq_ndev_ops;
#endif


static struct workqueue_struct *aq_ndev_wq;

void aq_ndev_service_event_schedule(struct aq_nic_s *aq_nic)
{
	queue_work(aq_ndev_wq, &aq_nic->service_task);
}

struct net_device *aq_ndev_alloc(void)
{
	struct net_device *ndev = NULL;
	struct aq_nic_s *aq_nic = NULL;

	ndev = alloc_etherdev_mq(sizeof(struct aq_nic_s), AQ_CFG_VECS_MAX);
	if (!ndev)
		return NULL;

	aq_nic = netdev_priv(ndev);
	aq_nic->ndev = ndev;
#ifdef __VMKLNX__
	ndev->open = aq_ndev_open;
	ndev->stop = aq_ndev_close;
	ndev->hard_start_xmit = aq_ndev_start_xmit;
	ndev->set_multicast_list = aq_ndev_set_multicast_settings;
	ndev->change_mtu = aq_ndev_change_mtu;
	ndev->set_mac_address = aq_ndev_set_mac_address;
	ndev->tx_timeout = aq_ndev_tx_timeout;
	ndev->vlan_rx_add_vid = aq_ndo_vmk_vlan_rx_add_vid;
	ndev->vlan_rx_kill_vid = aq_ndo_vmk_vlan_rx_kill_vid;
#else
	ndev->netdev_ops = &aq_ndev_ops;
#endif
	ndev->ethtool_ops = &aq_ethtool_ops;

	return ndev;
}

static int aq_ndev_open(struct net_device *ndev)
{
	int err = 0;
	struct aq_nic_s *aq_nic = netdev_priv(ndev);

	aq_drvinfo_init(ndev);

	err = aq_nic_init(aq_nic);
	if (err < 0)
		goto err_exit;
	err = aq_nic_start(aq_nic);
	if (err < 0)
		goto err_exit;

err_exit:
	if (err < 0)
		aq_nic_deinit(aq_nic);
	return err;
}

static int aq_ndev_close(struct net_device *ndev)
{
	int err = 0;
	struct aq_nic_s *aq_nic = netdev_priv(ndev);

	aq_drvinfo_exit(ndev);


	err = aq_nic_stop(aq_nic);
	if (err < 0)
		goto err_exit;
	aq_nic_deinit(aq_nic);

err_exit:
	return err;
}

static int aq_ndev_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct aq_nic_s *aq_nic = netdev_priv(ndev);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)) ||\
    (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7, 2))
	if (unlikely(aq_utils_obj_test(&aq_nic->flags, AQ_NIC_PTP_DPATH_UP))) {
		/* Hardware adds the Timestamp for PTPv2 802.AS1 and PTPv2 IPv4 UDP. */
		if (unlikely((ip_hdr(skb)->version == 4) &&
					(ip_hdr(skb)->protocol == IPPROTO_UDP) &&
					((udp_hdr(skb)->dest == htons(319)) || (udp_hdr(skb)->dest == htons(320))))) {
			return aq_ptp_xmit(aq_nic, skb);
		}
		if (unlikely(eth_hdr(skb)->h_proto == htons(ETH_P_1588))) {
			return aq_ptp_xmit(aq_nic, skb);
		}
	}
#endif

#ifndef __VMKLNX__
	skb_tx_timestamp(skb);
#endif
	return aq_nic_xmit(aq_nic, skb);
}

static int aq_ndev_change_mtu(struct net_device *ndev, int new_mtu)
{
	struct aq_nic_s *aq_nic = netdev_priv(ndev);
	int err = aq_nic_set_mtu(aq_nic, new_mtu + ETH_HLEN);

	if (err < 0)
		goto err_exit;
	ndev->mtu = new_mtu;

err_exit:
	return err;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
static int aq_ndev_set_features(struct net_device *ndev,
				netdev_features_t features)
{
	bool is_vlan_rx_strip = !!(features & NETIF_F_HW_VLAN_CTAG_RX);
	bool is_vlan_tx_insert = !!(features & NETIF_F_HW_VLAN_CTAG_TX);
	struct aq_nic_s *aq_nic = netdev_priv(ndev);
	struct aq_nic_cfg_s *aq_cfg;
	bool need_ndev_restart = false;
	bool is_lro = false;
	int err = 0;

	aq_cfg = aq_nic_get_cfg(aq_nic);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34)
	if (!(features & NETIF_F_NTUPLE)) {
		if (aq_nic->ndev->features & NETIF_F_NTUPLE) {
			err = aq_clear_rxnfc_all_rules(aq_nic);
			if (unlikely(err))
				goto err_exit;
		}
	}
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
	if (features & NETIF_F_HW_VLAN_CTAG_FILTER) {
		if (!(aq_nic->ndev->features & NETIF_F_HW_VLAN_CTAG_FILTER)) {
			err = aq_filters_vlans_on(aq_nic);
			if (unlikely(err))
				goto err_exit;
		}
	} else {
		if (aq_nic->ndev->features & NETIF_F_HW_VLAN_CTAG_FILTER) {
			err = aq_filters_vlans_off(aq_nic);
			if (unlikely(err))
				goto err_exit;
		}
	}
#endif

	aq_cfg->features = features;

	if (aq_cfg->aq_hw_caps->hw_features & NETIF_F_LRO) {
		is_lro = features & NETIF_F_LRO;

		if (aq_cfg->is_lro != is_lro) {
			aq_cfg->is_lro = is_lro;
			need_ndev_restart = true;
		}
	}
	if ((aq_nic->ndev->features ^ features) & NETIF_F_RXCSUM) {
		err = aq_nic->aq_hw_ops->hw_set_offload(aq_nic->aq_hw,
							aq_cfg);
		if (unlikely(err))
			goto err_exit;
	}

	if (aq_cfg->is_vlan_rx_strip != is_vlan_rx_strip) {
		aq_cfg->is_vlan_rx_strip = is_vlan_rx_strip;
		need_ndev_restart = true;
	}
	if (aq_cfg->is_vlan_tx_insert != is_vlan_tx_insert) {
		aq_cfg->is_vlan_tx_insert = is_vlan_tx_insert;
		need_ndev_restart = true;
	}

	if (need_ndev_restart && netif_running(ndev)) {
		aq_ndev_close(ndev);
		aq_ndev_open(ndev);
	}
err_exit:
	return err;
}
#endif

static int aq_ndev_set_mac_address(struct net_device *ndev, void *addr)
{
	struct aq_nic_s *aq_nic = netdev_priv(ndev);
	int err = 0;

	err = eth_mac_addr(ndev, addr);
	if (err < 0)
		goto err_exit;
	err = aq_nic_set_mac(aq_nic, ndev);
	if (err < 0)
		goto err_exit;

err_exit:
	return err;
}

static void aq_ndev_set_multicast_settings(struct net_device *ndev)
{
	struct aq_nic_s *aq_nic = netdev_priv(ndev);
	int err = 0;

	err = aq_nic_set_packet_filter(aq_nic, ndev->flags);
	if (err < 0)
		return;

	err = aq_nic_set_multicast_list(aq_nic, ndev);
	if (err < 0)
		return;
}

#ifndef __VMKLNX__
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)) ||\
    (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7, 2))
static int aq_ndev_config_hwtstamp(struct aq_nic_s *aq_nic, struct hwtstamp_config *config)
{
	if (config->flags)
		return -EINVAL;

	switch (config->tx_type) {
		case HWTSTAMP_TX_OFF:
		case HWTSTAMP_TX_ON:
			break;
		default:
			return -ERANGE;
	}

	switch (config->rx_filter) {
		case HWTSTAMP_FILTER_PTP_V2_L4_EVENT:
		case HWTSTAMP_FILTER_PTP_V2_L4_SYNC:
		case HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ:
		case HWTSTAMP_FILTER_PTP_V2_L2_EVENT:
		case HWTSTAMP_FILTER_PTP_V2_L2_SYNC:
		case HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ:
		case HWTSTAMP_FILTER_PTP_V2_SYNC:
		case HWTSTAMP_FILTER_PTP_V2_DELAY_REQ:
			config->rx_filter = HWTSTAMP_FILTER_PTP_V2_EVENT;
			break;
		case HWTSTAMP_FILTER_PTP_V2_EVENT:
		case HWTSTAMP_FILTER_NONE:
			break;
		default:
			return -ERANGE;
	}

	return aq_ptp_hwtstamp_config_set(aq_nic->aq_ptp, config);
}
#endif

static int aq_ndev_hwtstamp_set(struct aq_nic_s *aq_nic, struct ifreq *ifr)
{
	struct hwtstamp_config config;
	int ret_val;

	if (!aq_nic->aq_ptp)
		return -EOPNOTSUPP;

	if (copy_from_user(&config, ifr->ifr_data, sizeof(config)))
		return -EFAULT;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)) ||\
    (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7, 2))
	ret_val = aq_ndev_config_hwtstamp(aq_nic, &config);
	if (ret_val)
		return ret_val;
#endif

	return copy_to_user(ifr->ifr_data, &config, sizeof(config)) ? -EFAULT : 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)) ||\
    (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7, 2))
static int aq_ndev_hwtstamp_get(struct aq_nic_s *aq_nic, struct ifreq *ifr)
{
	struct hwtstamp_config config;

	if (!aq_nic->aq_ptp)
		return -EOPNOTSUPP;

	aq_ptp_hwtstamp_config_get(aq_nic->aq_ptp, &config);
	return copy_to_user(ifr->ifr_data, &config, sizeof(config)) ? -EFAULT : 0;
}
#endif

static int aq_ndev_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
{
	struct aq_nic_s *aq_nic = netdev_priv(netdev);

	switch (cmd) {
		case SIOCSHWTSTAMP:
			return aq_ndev_hwtstamp_set(aq_nic, ifr);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)) ||\
    (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7, 2))

		case SIOCGHWTSTAMP:
			return aq_ndev_hwtstamp_get(aq_nic, ifr);

#endif
	}

	return -EOPNOTSUPP;
}
#endif /*__VMKLNX__*/

static int aq_ndo_vlan_rx_add_vid(struct net_device *ndev, __be16 proto,
				  u16 vid)

{
	struct aq_nic_s *aq_nic = netdev_priv(ndev);

	if (!aq_nic->aq_hw_ops->hw_filter_vlan_set)
		return -EOPNOTSUPP;

	set_bit(vid, aq_nic->active_vlans);

	return aq_filters_vlans_update(aq_nic);
}

#ifdef __VMKLNX__
static void aq_ndev_tx_timeout(struct net_device *ndev)
{
	/*TO DO probably really need to reset TX
	 * we need this callback since ESXi may
	 * crash it if NULL in net_device structure
	 */
}

static void aq_ndo_vmk_vlan_rx_add_vid(struct net_device *ndev, u16 vid)
{
	aq_ndo_vlan_rx_add_vid(ndev, 0, vid);
}
#endif

static int aq_ndo_vlan_rx_kill_vid(struct net_device *ndev, __be16 proto,
				   u16 vid)
{
	struct aq_nic_s *aq_nic = netdev_priv(ndev);

	if (!aq_nic->aq_hw_ops->hw_filter_vlan_set)
		return -EOPNOTSUPP;

	clear_bit(vid, aq_nic->active_vlans);
#ifndef __VMKLNX__
	if (-ENOENT == aq_del_fvlan_by_vlan(aq_nic, vid))
#endif
		return aq_filters_vlans_update(aq_nic);

	return 0;
}

#ifdef __VMKLNX__
static void aq_ndo_vmk_vlan_rx_kill_vid(struct net_device *ndev, u16 vid)
{
	aq_ndo_vlan_rx_kill_vid(ndev, 0, vid);
}
#endif

#ifndef __VMKLNX__
static const struct net_device_ops aq_ndev_ops = {
	.ndo_open = aq_ndev_open,
	.ndo_stop = aq_ndev_close,
	.ndo_start_xmit = aq_ndev_start_xmit,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
	.ndo_set_multicast_list = aq_ndev_set_multicast_settings,
#else
	.ndo_set_rx_mode = aq_ndev_set_multicast_settings,
#endif
#if (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7, 5) && \
     RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 0))
	.extended.ndo_change_mtu = aq_ndev_change_mtu,
#else
	.ndo_change_mtu = aq_ndev_change_mtu,
#endif
	.ndo_set_mac_address = aq_ndev_set_mac_address,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
	.ndo_set_features = aq_ndev_set_features,
#endif
	.ndo_do_ioctl = aq_ndev_ioctl,
	.ndo_vlan_rx_add_vid = aq_ndo_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = aq_ndo_vlan_rx_kill_vid,
};
#endif

static int __init aq_ndev_init_module(void)
{
	int ret;

	aq_ndev_wq = create_singlethread_workqueue(aq_ndev_driver_name);
	if (!aq_ndev_wq) {
		pr_err("Failed to create workwueue\n");
		return -ENOMEM;
	}

	ret = aq_pci_func_register_driver();
	if (ret) {
		destroy_workqueue(aq_ndev_wq);
		return ret;
	}

	return 0;
}

static void __exit aq_ndev_exit_module(void)
{
	aq_pci_func_unregister_driver();
	
	if (aq_ndev_wq) {
		destroy_workqueue(aq_ndev_wq);
		aq_ndev_wq = NULL;
	}
}

module_init(aq_ndev_init_module);
module_exit(aq_ndev_exit_module);
