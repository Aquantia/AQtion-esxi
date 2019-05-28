/*
 * aQuantia Corporation Network Driver
 * Copyright (C) 2014-2017 aQuantia Corporation. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

/* File aq_compat.h: Backward compat with previous linux kernel versions */

#ifndef AQ_COMPAT_H
#define AQ_COMPAT_H

#include <linux/version.h>

#ifndef RHEL_RELEASE_VERSION
#define RHEL_RELEASE_VERSION(a, b) (((a) << 8) + (b))
#endif

#ifndef RHEL_RELEASE_CODE
#define RHEL_RELEASE_CODE 0
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
#if !RHEL_RELEASE_CODE || (RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(7, 6))

#define from_timer(var, callback_timer, timer_fieldname) \
	container_of(callback_timer, typeof(*var), timer_fieldname)

static inline void timer_setup(struct timer_list *timer,
			       void (*callback)(struct timer_list *),
			       unsigned int flags)
{
	setup_timer(timer, (void (*)(unsigned long))callback,
		    (unsigned long)timer);
}

#endif
#endif

#ifndef SPEED_5000
#define SPEED_5000 5000
#endif

#ifndef ETH_MIN_MTU
#define ETH_MIN_MTU	68
#endif

#ifndef SKB_ALLOC_NAPI
#ifdef __VMKLNX__
static inline struct sk_buff *netdev_alloc_skb_ip_align(struct net_device *dev,
							unsigned int length)
{
	struct sk_buff *skb = netdev_alloc_skb(dev, length + NET_IP_ALIGN);

	if (NET_IP_ALIGN && skb)
		skb_reserve(skb, NET_IP_ALIGN);
	return skb;
}
#endif

static inline struct sk_buff *napi_alloc_skb(struct napi_struct *napi,
					     unsigned int length)
{
	return netdev_alloc_skb_ip_align(napi->dev, length);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 7, 0) && !defined(__VMKLNX__)
/* from commit 1dff8083a024650c75a9c961c38082473ceae8cf */
#define page_to_virt(x)	__va(PFN_PHYS(page_to_pfn(x)))
#endif	/* 4.7.0 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 6, 0)

#if !(RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE > RHEL_RELEASE_VERSION(7, 2)))
/* from commit fe896d1878949ea92ba547587bc3075cc688fb8f */
#ifdef __VMKLNX__
#define page_ref_inc(p) atomic_inc(&(p)->_refcount)
#define page_ref_count(p) atomic_read(&(p)->_refcount)
#else
static inline void page_ref_inc(struct page *page)
{
	atomic_inc(&page->_count);
}

static inline int page_ref_count(struct page *page)
{
	return atomic_read(&page->_count);
}
#endif /*__VMKLNX__*/

#endif /* !(RHEL_RELEASE_CODE && (RHEL_RELEASE_CODE > RHEL_RELEASE_VERSION(7, 2)))*/

#endif /* 4.6.0 */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0)) && !(RHEL_RELEASE_CODE)

/* from commit 286ab723d4b83d37deb4017008ef1444a95cfb0d */
static inline void ether_addr_copy(u8 *dst, const u8 *src)
{
	memcpy(dst, src, 6);
}
#endif /* 3.14.0 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 0)

/* introduced in commit 56193d1bce2b2759cb4bdcc00cd05544894a0c90
 * pull the whole head buffer len for now
 */
#define eth_get_headlen(__data, __max_len) (__max_len)

/* ->xmit_more introduced in commit
 * 0b725a2ca61bedc33a2a63d0451d528b268cf975 for 3.18-rc1
 */
static inline int skb_xmit_more(struct sk_buff *skb)
{
	return 0;
}

#else /* 3.18.0 */
static inline int skb_xmit_more(struct sk_buff *skb)
{
	return skb->xmit_more;
}

#endif	/* 3.18.0 */

#ifndef __VMKLNX__
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
#define IFF_UNICAST_FLT        0
#define dev_alloc_pages(__order) alloc_pages_node(NUMA_NO_NODE, \
						  GFP_ATOMIC |  \
						  __GFP_COMP |  \
						  __GFP_COLD,   \
						  __order)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
/* introduced in commit 71dfda58aaaf4bf6b1bc59f9d8afa635fa1337d4 */
#define dev_alloc_pages(__order) __skb_alloc_pages(GFP_ATOMIC |    \
						   __GFP_COMP,     \
						   NULL, __order)
#endif  /* 3.19.0 */
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,17,0) &&\
    RHEL_RELEASE_CODE <= RHEL_RELEASE_VERSION(7,3)
#define hlist_add_behind(_a, _b) hlist_add_after(_b, _a)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,17,0)
#if !(RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(6,8) && \
      RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(7,0)) && \
    !(RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7,2))
#ifndef timespec64
#define timespec64 timespec
#define timespec64_to_ns timespec_to_ns
#define ns_to_timespec64 ns_to_timespec
#endif
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0)
#define	IPV6_USER_FLOW	0x0e
#endif

#ifndef __VMKLNX__
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
#define timecounter_adjtime(tc, delta) do { \
		(tc)->nsec += delta; } while(0)
#define skb_vlan_tag_present(__skb) ((__skb)->vlan_tci & VLAN_TAG_PRESENT)
#define skb_vlan_tag_get(__skb) ((__skb)->vlan_tci & ~VLAN_TAG_PRESENT)
#endif
#else /*__VMKLNX__*/
#include <linux/if_vlan.h>
#define skb_vlan_tag_present(__skb) vlan_tx_tag_present(skb)
#define skb_vlan_tag_get(__skb) vlan_tx_tag_get(skb)

static void __vlan_hwaccel_put_tag_internal(struct sk_buff *skb, u16 vlan_tci)
{
	VLAN_RX_SKB_CB(skb)->magic = VLAN_RX_COOKIE_MAGIC;
	VLAN_RX_SKB_CB(skb)->vlan_tag = vlan_tci;
}

#define __vlan_hwaccel_put_tag(skb, vlan_proto, vlan_tci) \
		__vlan_hwaccel_put_tag_internal(skb, vlan_tci)
#endif /*__VMKLNX__*/

#ifdef __VMKLNX__

#ifndef __packed
#define __packed __attribute__((packed))
#endif

#define VLAN_N_VID		4096

static void eth_commit_mac_addr_change(struct net_device *dev, void *p)
{
	struct sockaddr *addr = p;

	memcpy(dev->dev_addr, addr->sa_data, ETH_ALEN);
}

static int eth_mac_addr(struct net_device *dev, void *p)
{
	eth_commit_mac_addr_change(dev, p);
	return 0;
}
#define pr_err(...) printk(KERN_ERR __VA_ARGS__);

static inline unsigned int skb_frag_size(const skb_frag_t *frag)
{
	return frag->size;
}

static inline struct page *skb_frag_page(const skb_frag_t *frag)
{
	return frag->page;
}

static inline dma_addr_t skb_frag_dma_map(struct device *dev,
					  const skb_frag_t *frag,
					  size_t offset, size_t size,
					  enum dma_data_direction dir)
{
	return dma_map_page(dev, skb_frag_page(frag),
			    frag->page_offset + offset, size, dir);
}

#define netdev_uc_count(dev) 0

#define netdev_mc_count(dev) dev->mc_count

#define netdev_for_each_uc_addr(ha, dev) \
	for (ha = NULL; ha; ha = ha->next)

#define netdev_for_each_mc_addr(ha, dev) \
	for (ha = dev->mc_list; ha; ha = ha->next)

#define ETH_FCS_LEN 4

#define netdev_info(dev, ...) printk(KERN_INFO __VA_ARGS__);

#define MDIO_MMD_PMAPMD		1	/* Physical Medium Attachment*/
#define MDIO_MMD_VEND1		30	/* Vendor specific 1 */

#include <linux/gfp.h>
static inline struct page *dev_alloc_pages(unsigned int order)
{
	return alloc_pages(GFP_ATOMIC | __GFP_NOWARN, order);
}

/*We don't have this define, but let's use
 * 24th(0x100,0000) bit since it is not used
 */
#define NETIF_F_RXCSUM BIT(24)
#define NETIF_F_RXHASH 0
#define NETIF_F_LRO 0
#define NETIF_F_NTUPLE 0

/*static struct sk_buff *__build_skb(void *data, unsigned int frag_size)
{
	struct skb_shared_info *shinfo;
	struct sk_buff *skb;
	unsigned int size = frag_size ? : ksize(data);

	skb = kmem_cache_alloc(skbuff_head_cache, GFP_ATOMIC);
	if (!skb)
		return NULL;

	size -= SKB_DATA_ALIGN(sizeof(struct skb_shared_info));

	memset(skb, 0, offsetof(struct sk_buff, tail));
	skb->truesize = SKB_TRUESIZE(size);
	refcount_set(&skb->users, 1);
	skb->head = data;
	skb->data = data;
	skb_reset_tail_pointer(skb);
	skb->end = skb->tail + size;
	skb->mac_header = (typeof(skb->mac_header))~0U;
	skb->transport_header = (typeof(skb->transport_header))~0U;

	 make sure we initialize shinfo sequentially
	shinfo = skb_shinfo(skb);
	memset(shinfo, 0, offsetof(struct skb_shared_info, dataref));
	atomic_set(&shinfo->dataref, 1);

	return skb;
}

static struct sk_buff *build_skb(void *data, unsigned int frag_size)
{
	struct sk_buff *skb = __build_skb(data, frag_size);

	if (skb && frag_size) {
		skb->head_frag = 1;
		if (page_is_pfmemalloc(virt_to_head_page(data)))
			skb->pfmemalloc = 1;
	}
	return skb;
}*/

static void skb_add_rx_frag(struct sk_buff *skb, int i, struct page *page, int off,
		     int size, unsigned int truesize)
{
	skb_fill_page_desc(skb, i, page, off, size);
	skb->len += size;
	skb->data_len += size;
	skb->truesize += truesize;
}

static inline void skb_record_rx_queue(struct sk_buff *skb, u16 rx_queue)
{
	skb->queue_mapping = rx_queue + 1;
}

#define NETIF_F_HW_VLAN_CTAG_FILTER NETIF_F_HW_VLAN_FILTER
#define NETIF_F_HW_VLAN_CTAG_RX NETIF_F_HW_VLAN_RX
#define NETIF_F_HW_VLAN_CTAG_TX NETIF_F_HW_VLAN_TX

#define trace_aq_tx_descriptor(x, y, z)
#define trace_aq_rx_descr(x, y, z)

#define IFF_UNICAST_FLT 0

#define NEXTHDR_TCP IPPROTO_TCP
#define NEXTHDR_UDP IPPROTO_UDP

#define cpumask_t u64

#define dma_sync_single_range_for_cpu(dev, addr, offset, size, dir) \
		dma_sync_single_for_cpu(dev, addr + offset, size, dir)
#define dma_mapping_error(dev, dma_addr) dma_mapping_error(dma_addr)

#define cpumask_set_cpu(cpu, dstp)

static void aq_set_rsc_gso_size(struct net_device *netdev, struct sk_buff *skb)
{
	u32 ethhdr_sz = eth_header_len((struct ethhdr *)skb->data);

	if ((skb->len - ethhdr_sz) > netdev->mtu) {
		struct iphdr *iph;
		struct tcphdr *tcph;
		iph = (struct iphdr *)(skb->data + ethhdr_sz);
		tcph = (struct tcphdr *)(skb->data + ethhdr_sz + (iph->ihl << 2));
		skb_shinfo(skb)->gso_size = skb->len - (ethhdr_sz +
					    (iph->ihl << 2) + (tcph->doff << 2));
		skb_shinfo(skb)->gso_type = SKB_GSO_TCPV4;
	}
}

static void napi_gro_receive(struct napi_struct *napi, struct sk_buff *skb)
{
	aq_set_rsc_gso_size(napi->dev, skb);
	netif_receive_skb(skb);
}
#endif /*__VMKLNX__*/

#endif /* AQ_COMMON_H */
