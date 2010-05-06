#include <router_cmd.h>
#include <interface.h>

#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <linux/in_route.h>

#include <asm/libnetlink.h>
#include <asm/eth.h>

#include <netdb.h>


#define SPRINT_BSIZE 64
#define SPRINT_BUF(x)	char x[SPRINT_BSIZE]
#define _SL_	"\n"

typedef struct inet_prefix {
	unsigned char  family;
	unsigned char bytelen;
	short bitlen;
	unsigned int flags;
	unsigned int data[4];
} inet_prefix;

struct rtnl_handle rth;

static const char *mx_names[RTAX_MAX+1] = {
	[RTAX_MTU]	= "mtu",
	[RTAX_WINDOW]	= "window",
	[RTAX_RTT]	= "rtt",
	[RTAX_RTTVAR]	= "rttvar",
	[RTAX_SSTHRESH] = "ssthresh",
	[RTAX_CWND]	= "cwnd",
	[RTAX_ADVMSS]	= "advmss",
	[RTAX_REORDERING]="reordering",
	[RTAX_HOPLIMIT] = "hoplimit",
	[RTAX_INITCWND] = "initcwnd",
	[RTAX_FEATURES] = "features",
	[RTAX_RTO_MIN]	= "rto_min",
	[RTAX_MAX]	= "initrwnd",
};

static struct
{
	int tb;
	int cloned;
	int flushed;
	char *flushb;
	int flushp;
	int flushe;
	int protocol, protocolmask;
	int scope, scopemask;
	int type, typemask;
	int tos, tosmask;
	int iif, iifmask;
	int oif, oifmask;
	int realm, realmmask;
	inet_prefix rprefsrc;
	inet_prefix rvia;
	inet_prefix rdst;
	inet_prefix mdst;
	inet_prefix rsrc;
	inet_prefix msrc;
} filter;

#define DN_MAXADDL 20
#define IPX_NODE_LEN 6

struct dn_naddr
{
        unsigned short          a_len;
        unsigned char a_addr[DN_MAXADDL];
};

struct ipx_addr {
	u_int32_t ipx_net;
	u_int8_t  ipx_node[IPX_NODE_LEN];
};

static int show_stats = 0;

struct idxmap
{
	struct idxmap * next;
	unsigned	index;
	int		type;
	int		alen;
	unsigned	flags;
	unsigned char	addr[20];
	char		name[16];
};

int get_hz()
{
	return 0;
}

int get_user_hz()
{
	return 0;
}

char * rtnl_rtscope_n2a(int id, char *buf, int len)
{
	snprintf(buf, len, "%d", id);
	return buf;
}

char * rtnl_rttable_n2a(__u32 id, char *buf, int len)
{
	snprintf(buf, len, "%u", id);
	return buf;
}

char * rtnl_rtprot_n2a(int id, char *buf, int len)
{
	snprintf(buf, len, "%d", id);
	return buf;
}

char * rtnl_dsfield_n2a(int id, char *buf, int len)
{
	snprintf(buf, len, "0x%02x", id);
	return buf;
}

char * rtnl_rtrealm_n2a(int id, char *buf, int len)
{
	snprintf(buf, len, "%d", id);
	return buf;
}

static struct idxmap *idxmap[16];

static __inline__ int do_digit(char *str, u_int32_t addr, u_int32_t scale, size_t *pos, size_t len)
{
	u_int32_t tmp = addr >> (scale * 4);

	if (*pos == len)
		return 1;

	tmp &= 0x0f;
	if (tmp > 9)
		*str = tmp + 'A' - 10;
	else
		*str = tmp + '0';
	(*pos)++;

	return 0;
}

static __inline__ u_int16_t dn_ntohs(u_int16_t addr)
{
	union {
		u_int8_t byte[2];
		u_int16_t word;
	} u;

	u.word = addr;
	return ((u_int16_t)u.byte[0]) | (((u_int16_t)u.byte[1]) << 8);
}


static __inline__ int do_digit2(char *str, u_int16_t *addr, u_int16_t scale, size_t *pos, size_t len, int *started)
{
	u_int16_t tmp = *addr / scale;

	if (*pos == len)
		return 1;

	if (((tmp) > 0) || *started || (scale == 1)) {
		*str = tmp + '0';
		*started = 1;
		(*pos)++;
		*addr -= (tmp * scale);
	}

	return 0;
}


char *rtnl_rtntype_n2a(int id, char *buf, int len)
{
	switch (id) {
	case RTN_UNSPEC:
		return "none";
	case RTN_UNICAST:
		return "unicast";
	case RTN_LOCAL:
		return "local";
	case RTN_BROADCAST:
		return "broadcast";
	case RTN_ANYCAST:
		return "anycast";
	case RTN_MULTICAST:
		return "multicast";
	case RTN_BLACKHOLE:
		return "blackhole";
	case RTN_UNREACHABLE:
		return "unreachable";
	case RTN_PROHIBIT:
		return "prohibit";
	case RTN_THROW:
		return "throw";
	case RTN_NAT:
		return "nat";
	case RTN_XRESOLVE:
		return "xresolve";
	default:
		snprintf(buf, len, "%d", id);
		return buf;
	}
}

static const char *dnet_ntop(int af, const void* ptr,char *str, size_t len)
{
	struct dn_naddr *dna = (struct dn_naddr*) ptr;
	u_int16_t addr = dn_ntohs(*(u_int16_t *)dna->a_addr);
	u_int16_t area = addr >> 10;
	size_t pos = 0;
	int started = 0;

	if (dna->a_len != 2)
		return NULL;

	addr &= 0x03ff;

	if (len == 0)
		return str;

	if (do_digit2(str + pos, &area, 10, &pos, len, &started))
		return str;

	if (do_digit2(str + pos, &area, 1, &pos, len, &started))
		return str;

	if (pos == len)
		return str;

	*(str + pos) = '.';
	pos++;
	started = 0;

	if (do_digit2(str + pos, &addr, 1000, &pos, len, &started))
		return str;

	if (do_digit2(str + pos, &addr, 100, &pos, len, &started))
		return str;

	if (do_digit2(str + pos, &addr, 10, &pos, len, &started))
		return str;

	if (do_digit2(str + pos, &addr, 1, &pos, len, &started))
		return str;

	if (pos == len)
		return str;

	*(str + pos) = 0;

	return str;
}

const char *ll_idx_n2a(unsigned idx, char *buf)
{
	struct idxmap *im;

	if (idx == 0)
		return "*";
	for (im = idxmap[idx&0xF]; im; im = im->next)
		if (im->index == idx)
			return im->name;
	snprintf(buf, 16, "if%d", idx);
	return buf;
}

const char *ll_index_to_name(unsigned idx)
{
	static char nbuf[16];

	return ll_idx_n2a(idx, nbuf);
}

static const char *ipx_ntop(int af, const void* ptr, char *str, size_t len)
{
	int i;
	size_t pos = 0;
	struct ipx_addr *addr = (struct ipx_addr*) ptr;

	if (len == 0)
		return str;

	for(i = 7; i >= 0; i--)
		if (do_digit(str + pos, ntohl(addr->ipx_net), i, &pos, len))
			return str;

	if (pos == len)
		return str;

	*(str + pos) = '.';
	pos++;

	for(i = 0; i < 6; i++) {
		if (do_digit(str + pos, addr->ipx_node[i], 1, &pos, len))
			return str;
		if (do_digit(str + pos, addr->ipx_node[i], 0, &pos, len))
			return str;
	}

	if (pos == len)
		return str;

	*(str + pos) = 0;

	return str;
}

const char *rt_addr_n2a(int af, int len, const void *addr, char *buf, int buflen)
{

	struct dn_naddr dna = { 2, { 0, 0, }};
	switch (af) {
	case AF_INET:
	case AF_INET6:
		return inet_ntop(af, addr, buf, buflen);
	case AF_IPX:
		return ipx_ntop(af, addr, buf, buflen);
	case AF_DECnet:
		memcpy(dna.a_addr, addr, 2);
		return dnet_ntop(af, &dna, buf, buflen);
	default:
		return "???";
	}
}

int inet_addr_match(const inet_prefix *a, const inet_prefix *b, int bits)
{
	const __u32 *a1 = a->data;
	const __u32 *a2 = b->data;
	int words = bits >> 0x05;

	bits &= 0x1f;

	if (words)
		if (memcmp(a1, a2, words << 2))
			return -1;

	if (bits) {
		__u32 w1, w2;
		__u32 mask;

		w1 = a1[words];
		w2 = a2[words];

		mask = htonl((0xffffffff) << (0x20 - bits));

		if ((w1 ^ w2) & mask)
			return 1;
	}

	return 0;
}

static inline int rtm_get_table(struct rtmsg *r, struct rtattr **tb)
{
	__u32 table = r->rtm_table;
	if (tb[RTA_TABLE])
		table = *(__u32*) RTA_DATA(tb[RTA_TABLE]);
	return table;
}

static int flush_update(void)
{
	if (rtnl_send_check(&rth, filter.flushb, filter.flushp) < 0) {
		perror("Failed to send flush request");
		return -1;
	}
	filter.flushp = 0;
	return 0;
}

const char *format_host(int af, int len, const void *addr,
			char *buf, int buflen)
{
#ifdef RESOLVE_HOSTNAMES
	if (resolve_hosts) {
		const char *n;

		if (len <= 0) {
			switch (af) {
			case AF_INET:
				len = 4;
				break;
			case AF_INET6:
				len = 16;
				break;
			case AF_IPX:
				len = 10;
				break;
#ifdef AF_DECnet
			/* I see no reasons why gethostbyname
			   may not work for DECnet */
			case AF_DECnet:
				len = 2;
				break;
#endif
			default: ;
			}
		}
		if (len > 0 &&
		    (n = resolve_address(addr, len, af)) != NULL)
			return n;
	}
#endif
	return rt_addr_n2a(af, len, addr, buf, buflen);
}



int print_route(const struct sockaddr_nl *who, struct nlmsghdr *n, void *arg)
{

	FILE *fp = (FILE*)arg;
	struct rtmsg *r = NLMSG_DATA(n);
	int len = n->nlmsg_len;
	struct rtattr * tb[RTA_MAX+1];
	char abuf[256];
	inet_prefix dst;
	inet_prefix src;
	inet_prefix prefsrc;
	inet_prefix via;
	int host_len = -1;
	static int ip6_multiple_tables;
	__u32 table;
	SPRINT_BUF(b1);
	static int hz;

	if (n->nlmsg_type != RTM_NEWROUTE && n->nlmsg_type != RTM_DELROUTE) {
		fprintf(stderr, "Not a route: %08x %08x %08x\n",
			n->nlmsg_len, n->nlmsg_type, n->nlmsg_flags);
		return 0;
	}
	if (filter.flushb && n->nlmsg_type != RTM_NEWROUTE)
		return 0;
	len -= NLMSG_LENGTH(sizeof(*r));
	if (len < 0) {
		fprintf(stderr, "BUG: wrong nlmsg len %d\n", len);
		return -1;
	}

	if (r->rtm_family == AF_INET6)
		host_len = 128;
	else if (r->rtm_family == AF_INET)
		host_len = 32;
	else if (r->rtm_family == AF_DECnet)
		host_len = 16;
	else if (r->rtm_family == AF_IPX)
		host_len = 80;

	parse_rtattr(tb, RTA_MAX, RTM_RTA(r), len);
	table = rtm_get_table(r, tb);

	if (r->rtm_family == AF_INET6 && table != RT_TABLE_MAIN)
		ip6_multiple_tables = 1;

	if (r->rtm_family == AF_INET6 && !ip6_multiple_tables) {
		if (filter.cloned) {
			if (!(r->rtm_flags&RTM_F_CLONED))
				return 0;
		}
		if (filter.tb) {
			if (!filter.cloned && r->rtm_flags&RTM_F_CLONED)
				return 0;
			if (filter.tb == RT_TABLE_LOCAL) {
				if (r->rtm_type != RTN_LOCAL)
					return 0;
			} else if (filter.tb == RT_TABLE_MAIN) {
				if (r->rtm_type == RTN_LOCAL)
					return 0;
			} else {
				return 0;
			}
		}
	} else {
		if (filter.cloned) {
			if (!(r->rtm_flags&RTM_F_CLONED))
				return 0;
		}
		if (filter.tb > 0 && filter.tb != table)
			return 0;
	}
	if ((filter.protocol^r->rtm_protocol)&filter.protocolmask)
		return 0;
	if ((filter.scope^r->rtm_scope)&filter.scopemask)
		return 0;
	if ((filter.type^r->rtm_type)&filter.typemask)
		return 0;
	if ((filter.tos^r->rtm_tos)&filter.tosmask)
		return 0;
	if (filter.rdst.family &&
	    (r->rtm_family != filter.rdst.family || filter.rdst.bitlen > r->rtm_dst_len))
		return 0;
	if (filter.mdst.family &&
	    (r->rtm_family != filter.mdst.family ||
	     (filter.mdst.bitlen >= 0 && filter.mdst.bitlen < r->rtm_dst_len)))
		return 0;
	if (filter.rsrc.family &&
	    (r->rtm_family != filter.rsrc.family || filter.rsrc.bitlen > r->rtm_src_len))
		return 0;
	if (filter.msrc.family &&
	    (r->rtm_family != filter.msrc.family ||
	     (filter.msrc.bitlen >= 0 && filter.msrc.bitlen < r->rtm_src_len)))
		return 0;
	if (filter.rvia.family && r->rtm_family != filter.rvia.family)
		return 0;
	if (filter.rprefsrc.family && r->rtm_family != filter.rprefsrc.family)
		return 0;

	memset(&dst, 0, sizeof(dst));
	dst.family = r->rtm_family;
	if (tb[RTA_DST])
		memcpy(&dst.data, RTA_DATA(tb[RTA_DST]), (r->rtm_dst_len+7)/8);
	if (filter.rsrc.family || filter.msrc.family) {
		memset(&src, 0, sizeof(src));
		src.family = r->rtm_family;
		if (tb[RTA_SRC])
			memcpy(&src.data, RTA_DATA(tb[RTA_SRC]), (r->rtm_src_len+7)/8);
	}
	if (filter.rvia.bitlen>0) {
		memset(&via, 0, sizeof(via));
		via.family = r->rtm_family;
		if (tb[RTA_GATEWAY])
			memcpy(&via.data, RTA_DATA(tb[RTA_GATEWAY]), host_len/8);
	}
	if (filter.rprefsrc.bitlen>0) {
		memset(&prefsrc, 0, sizeof(prefsrc));
		prefsrc.family = r->rtm_family;
		if (tb[RTA_PREFSRC])
			memcpy(&prefsrc.data, RTA_DATA(tb[RTA_PREFSRC]), host_len/8);
	}

	if (filter.rdst.family && inet_addr_match(&dst, &filter.rdst, filter.rdst.bitlen))
		return 0;
	if (filter.mdst.family && filter.mdst.bitlen >= 0 &&
	    inet_addr_match(&dst, &filter.mdst, r->rtm_dst_len))
		return 0;

	if (filter.rsrc.family && inet_addr_match(&src, &filter.rsrc, filter.rsrc.bitlen))
		return 0;
	if (filter.msrc.family && filter.msrc.bitlen >= 0 &&
	    inet_addr_match(&src, &filter.msrc, r->rtm_src_len))
		return 0;

	if (filter.rvia.family && inet_addr_match(&via, &filter.rvia, filter.rvia.bitlen))
		return 0;
	if (filter.rprefsrc.family && inet_addr_match(&prefsrc, &filter.rprefsrc, filter.rprefsrc.bitlen))
		return 0;
	if (filter.realmmask) {
		__u32 realms = 0;
		if (tb[RTA_FLOW])
			realms = *(__u32*)RTA_DATA(tb[RTA_FLOW]);
		if ((realms^filter.realm)&filter.realmmask)
			return 0;
	}
	if (filter.iifmask) {
		int iif = 0;
		if (tb[RTA_IIF])
			iif = *(int*)RTA_DATA(tb[RTA_IIF]);
		if ((iif^filter.iif)&filter.iifmask)
			return 0;
	}
	if (filter.oifmask) {
		int oif = 0;
		if (tb[RTA_OIF])
			oif = *(int*)RTA_DATA(tb[RTA_OIF]);
		if ((oif^filter.oif)&filter.oifmask)
			return 0;
	}
	if (filter.flushb &&
	    r->rtm_family == AF_INET6 &&
	    r->rtm_dst_len == 0 &&
	    r->rtm_type == RTN_UNREACHABLE &&
	    tb[RTA_PRIORITY] &&
	    *(int*)RTA_DATA(tb[RTA_PRIORITY]) == -1)
		return 0;

	if (filter.flushb) {
		struct nlmsghdr *fn;
		if (NLMSG_ALIGN(filter.flushp) + n->nlmsg_len > filter.flushe) {
			if (flush_update())
				return -1;
		}
		fn = (struct nlmsghdr*)(filter.flushb + NLMSG_ALIGN(filter.flushp));
		memcpy(fn, n, n->nlmsg_len);
		fn->nlmsg_type = RTM_DELROUTE;
		fn->nlmsg_flags = NLM_F_REQUEST;
		fn->nlmsg_seq = ++rth.seq;
		filter.flushp = (((char*)fn) + n->nlmsg_len) - filter.flushb;
		filter.flushed++;
		if (show_stats < 2)
			return 0;
	}

	if (n->nlmsg_type == RTM_DELROUTE)
		fprintf(fp, "Deleted ");
	if (r->rtm_type != RTN_UNICAST && !filter.type)
		fprintf(fp, "%s ", rtnl_rtntype_n2a(r->rtm_type, b1, sizeof(b1)));

	if (tb[RTA_DST]) {
		if (r->rtm_dst_len != host_len) {
			fprintf(fp, "%s/%u ", rt_addr_n2a(r->rtm_family,
							 RTA_PAYLOAD(tb[RTA_DST]),
							 RTA_DATA(tb[RTA_DST]),
							 abuf, sizeof(abuf)),
				r->rtm_dst_len
				);
		} else {
			fprintf(fp, "%s ", format_host(r->rtm_family,
						       RTA_PAYLOAD(tb[RTA_DST]),
						       RTA_DATA(tb[RTA_DST]),
						       abuf, sizeof(abuf))
				);
		}
	} else if (r->rtm_dst_len) {
		fprintf(fp, "0/%d ", r->rtm_dst_len);
	} else {
		fprintf(fp, "default ");
	}
	if (tb[RTA_SRC]) {
		if (r->rtm_src_len != host_len) {
			fprintf(fp, "from %s/%u ", rt_addr_n2a(r->rtm_family,
							 RTA_PAYLOAD(tb[RTA_SRC]),
							 RTA_DATA(tb[RTA_SRC]),
							 abuf, sizeof(abuf)),
				r->rtm_src_len
				);
		} else {
			fprintf(fp, "from %s ", format_host(r->rtm_family,
						       RTA_PAYLOAD(tb[RTA_SRC]),
						       RTA_DATA(tb[RTA_SRC]),
						       abuf, sizeof(abuf))
				);
		}
	} else if (r->rtm_src_len) {
		fprintf(fp, "from 0/%u ", r->rtm_src_len);
	}
	if (r->rtm_tos && filter.tosmask != -1) {
		SPRINT_BUF(b1);
		fprintf(fp, "tos %s ", rtnl_dsfield_n2a(r->rtm_tos, b1, sizeof(b1)));
	}

	if (tb[RTA_GATEWAY] && filter.rvia.bitlen != host_len) {
		fprintf(fp, "via %s ",
			format_host(r->rtm_family,
				    RTA_PAYLOAD(tb[RTA_GATEWAY]),
				    RTA_DATA(tb[RTA_GATEWAY]),
				    abuf, sizeof(abuf)));
	}
	if (tb[RTA_OIF] && filter.oifmask != -1)
		fprintf(fp, "dev %s ", ll_index_to_name(*(int*)RTA_DATA(tb[RTA_OIF])));

	if (!(r->rtm_flags&RTM_F_CLONED)) {
		if (table != RT_TABLE_MAIN && !filter.tb)
			fprintf(fp, " table %s ", rtnl_rttable_n2a(table, b1, sizeof(b1)));
		if (r->rtm_protocol != RTPROT_BOOT && filter.protocolmask != -1)
			fprintf(fp, " proto %s ", rtnl_rtprot_n2a(r->rtm_protocol, b1, sizeof(b1)));
		if (r->rtm_scope != RT_SCOPE_UNIVERSE && filter.scopemask != -1)
			fprintf(fp, " scope %s ", rtnl_rtscope_n2a(r->rtm_scope, b1, sizeof(b1)));
	}
	if (tb[RTA_PREFSRC] && filter.rprefsrc.bitlen != host_len) {
		/* Do not use format_host(). It is our local addr
		   and symbolic name will not be useful.
		 */
		fprintf(fp, " src %s ",
			rt_addr_n2a(r->rtm_family,
				    RTA_PAYLOAD(tb[RTA_PREFSRC]),
				    RTA_DATA(tb[RTA_PREFSRC]),
				    abuf, sizeof(abuf)));
	}
	if (tb[RTA_PRIORITY])
		fprintf(fp, " metric %d ", *(__u32*)RTA_DATA(tb[RTA_PRIORITY]));
	if (r->rtm_flags & RTNH_F_DEAD)
		fprintf(fp, "dead ");
	if (r->rtm_flags & RTNH_F_ONLINK)
		fprintf(fp, "onlink ");
	if (r->rtm_flags & RTNH_F_PERVASIVE)
		fprintf(fp, "pervasive ");
	if (r->rtm_flags & RTM_F_NOTIFY)
		fprintf(fp, "notify ");

	if (tb[RTA_FLOW] && filter.realmmask != ~0U) {
		__u32 to = *(__u32*)RTA_DATA(tb[RTA_FLOW]);
		__u32 from = to>>16;
		to &= 0xFFFF;
		fprintf(fp, "realm%s ", from ? "s" : "");
		if (from) {
			fprintf(fp, "%s/",
				rtnl_rtrealm_n2a(from, b1, sizeof(b1)));
		}
		fprintf(fp, "%s ",
			rtnl_rtrealm_n2a(to, b1, sizeof(b1)));
	}
	if ((r->rtm_flags&RTM_F_CLONED) && r->rtm_family == AF_INET) {
		__u32 flags = r->rtm_flags&~0xFFFF;
		int first = 1;

		fprintf(fp, "%s    cache ", _SL_);

#define PRTFL(fl,flname) if (flags&RTCF_##fl) { \
  flags &= ~RTCF_##fl; \
  fprintf(fp, "%s" flname "%s", first ? "<" : "", flags ? "," : "> "); \
  first = 0; }
		PRTFL(LOCAL, "local");
		PRTFL(REJECT, "reject");
		PRTFL(MULTICAST, "mc");
		PRTFL(BROADCAST, "brd");
		PRTFL(DNAT, "dst-nat");
		PRTFL(SNAT, "src-nat");
		PRTFL(MASQ, "masq");
		PRTFL(DIRECTDST, "dst-direct");
		PRTFL(DIRECTSRC, "src-direct");
		PRTFL(REDIRECTED, "redirected");
		PRTFL(DOREDIRECT, "redirect");
		PRTFL(FAST, "fastroute");
		PRTFL(NOTIFY, "notify");
		PRTFL(TPROXY, "proxy");

		if (flags)
			fprintf(fp, "%s%x> ", first ? "<" : "", flags);
		if (tb[RTA_CACHEINFO]) {
			struct rta_cacheinfo *ci = RTA_DATA(tb[RTA_CACHEINFO]);
			if (!hz)
				hz = get_user_hz();
			if (ci->rta_expires != 0)
				fprintf(fp, " expires %dsec", ci->rta_expires/hz);
			if (ci->rta_error != 0)
				fprintf(fp, " error %d", ci->rta_error);
			if (show_stats) {
				if (ci->rta_clntref)
					fprintf(fp, " users %d", ci->rta_clntref);
				if (ci->rta_used != 0)
					fprintf(fp, " used %d", ci->rta_used);
				if (ci->rta_lastuse != 0)
					fprintf(fp, " age %dsec", ci->rta_lastuse/hz);
			}
#ifdef RTNETLINK_HAVE_PEERINFO
			if (ci->rta_id)
				fprintf(fp, " ipid 0x%04x", ci->rta_id);
			if (ci->rta_ts || ci->rta_tsage)
				fprintf(fp, " ts 0x%x tsage %dsec", ci->rta_ts, ci->rta_tsage);
#endif
		}
	} else if (r->rtm_family == AF_INET6) {
		struct rta_cacheinfo *ci = NULL;
		if (tb[RTA_CACHEINFO])
			ci = RTA_DATA(tb[RTA_CACHEINFO]);
		if ((r->rtm_flags & RTM_F_CLONED) || (ci && ci->rta_expires)) {
			if (!hz)
				hz = get_user_hz();
			if (r->rtm_flags & RTM_F_CLONED)
				fprintf(fp, "%s    cache ", _SL_);
			if (ci->rta_expires)
				fprintf(fp, " expires %dsec", ci->rta_expires/hz);
			if (ci->rta_error != 0)
				fprintf(fp, " error %d", ci->rta_error);
			if (show_stats) {
				if (ci->rta_clntref)
					fprintf(fp, " users %d", ci->rta_clntref);
				if (ci->rta_used != 0)
					fprintf(fp, " used %d", ci->rta_used);
				if (ci->rta_lastuse != 0)
					fprintf(fp, " age %dsec", ci->rta_lastuse/hz);
			}
		} else if (ci) {
			if (ci->rta_error != 0)
				fprintf(fp, " error %d", ci->rta_error);
		}
	}
	if (tb[RTA_METRICS]) {
		int i;
		unsigned mxlock = 0;
		struct rtattr *mxrta[RTAX_MAX+1];

		parse_rtattr(mxrta, RTAX_MAX, RTA_DATA(tb[RTA_METRICS]),
			    RTA_PAYLOAD(tb[RTA_METRICS]));
		if (mxrta[RTAX_LOCK])
			mxlock = *(unsigned*)RTA_DATA(mxrta[RTAX_LOCK]);

		for (i=2; i<= RTAX_MAX; i++) {
			unsigned val;

			if (mxrta[i] == NULL)
				continue;
			if (!hz)
				hz = get_hz();

			if (i < sizeof(mx_names)/sizeof(char*) && mx_names[i])
				fprintf(fp, " %s", mx_names[i]);
			else
				fprintf(fp, " metric %d", i);
			if (mxlock & (1<<i))
				fprintf(fp, " lock");

			val = *(unsigned*)RTA_DATA(mxrta[i]);
			switch (i) {
			case RTAX_HOPLIMIT:
				if ((long)val == -1)
					val = 0;
				/* fall through */
			default:
				fprintf(fp, " %u", val);
				break;

			case RTAX_RTT:
			case RTAX_RTTVAR:
			case RTAX_RTO_MIN:
				val *= 1000;
				if (i == RTAX_RTT)
					val /= 8;
				else if (i == RTAX_RTTVAR)
					val /= 4;

				if (val >= hz)
					fprintf(fp, " %llums",
						(unsigned long long) val / hz);
				else
					fprintf(fp, " %.2fms", 
						(double)val / hz);
			}
		}
	}
	if (tb[RTA_IIF] && filter.iifmask != -1) {
		fprintf(fp, " iif %s", ll_index_to_name(*(int*)RTA_DATA(tb[RTA_IIF])));
	}
	if (tb[RTA_MULTIPATH]) {
		struct rtnexthop *nh = RTA_DATA(tb[RTA_MULTIPATH]);
		int first = 0;

		len = RTA_PAYLOAD(tb[RTA_MULTIPATH]);

		for (;;) {
			if (len < sizeof(*nh))
				break;
			if (nh->rtnh_len > len)
				break;
			if (r->rtm_flags&RTM_F_CLONED && r->rtm_type == RTN_MULTICAST) {
				if (first)
					fprintf(fp, " Oifs:");
				else
					fprintf(fp, " ");
			} else
				fprintf(fp, "%s\tnexthop", _SL_);
			if (nh->rtnh_len > sizeof(*nh)) {
				parse_rtattr(tb, RTA_MAX, RTNH_DATA(nh), nh->rtnh_len - sizeof(*nh));
				if (tb[RTA_GATEWAY]) {
					fprintf(fp, " via %s ",
						format_host(r->rtm_family,
							    RTA_PAYLOAD(tb[RTA_GATEWAY]),
							    RTA_DATA(tb[RTA_GATEWAY]),
							    abuf, sizeof(abuf)));
				}
				if (tb[RTA_FLOW]) {
					__u32 to = *(__u32*)RTA_DATA(tb[RTA_FLOW]);
					__u32 from = to>>16;
					to &= 0xFFFF;
					fprintf(fp, " realm%s ", from ? "s" : "");
					if (from) {
						fprintf(fp, "%s/",
							rtnl_rtrealm_n2a(from, b1, sizeof(b1)));
					}
					fprintf(fp, "%s",
						rtnl_rtrealm_n2a(to, b1, sizeof(b1)));
				}
			}
			if (r->rtm_flags&RTM_F_CLONED && r->rtm_type == RTN_MULTICAST) {
				fprintf(fp, " %s", ll_index_to_name(nh->rtnh_ifindex));
				if (nh->rtnh_hops != 1)
					fprintf(fp, "(ttl>%d)", nh->rtnh_hops);
			} else {
				fprintf(fp, " dev %s", ll_index_to_name(nh->rtnh_ifindex));
				fprintf(fp, " weight %d", nh->rtnh_hops+1);
			}
			if (nh->rtnh_flags & RTNH_F_DEAD)
				fprintf(fp, " dead");
			if (nh->rtnh_flags & RTNH_F_ONLINK)
				fprintf(fp, " onlink");
			if (nh->rtnh_flags & RTNH_F_PERVASIVE)
				fprintf(fp, " pervasive");
			len -= NLMSG_ALIGN(nh->rtnh_len);
			nh = RTNH_NEXT(nh);
		}
	}
	fprintf(fp, "\n");
	fflush(fp);
	return 0;
}

int ll_remember_index(const struct sockaddr_nl *who,
		      struct nlmsghdr *n, void *arg)
{
	int h;
	struct ifinfomsg *ifi = NLMSG_DATA(n);
	struct idxmap *im, **imp;
	struct rtattr *tb[IFLA_MAX+1];

	if (n->nlmsg_type != RTM_NEWLINK)
		return 0;

	if (n->nlmsg_len < NLMSG_LENGTH(sizeof(ifi)))
		return -1;


	memset(tb, 0, sizeof(tb));
	parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifi), IFLA_PAYLOAD(n));
	if (tb[IFLA_IFNAME] == NULL)
		return 0;

	h = ifi->ifi_index&0xF;

	for (imp=&idxmap[h]; (im=*imp)!=NULL; imp = &im->next)
		if (im->index == ifi->ifi_index)
			break;

	if (im == NULL) {
		im = malloc(sizeof(*im));
		if (im == NULL)
			return 0;
		im->next = *imp;
		im->index = ifi->ifi_index;
		*imp = im;
	}

	im->type = ifi->ifi_type;
	im->flags = ifi->ifi_flags;
	if (tb[IFLA_ADDRESS]) {
		int alen;
		im->alen = alen = RTA_PAYLOAD(tb[IFLA_ADDRESS]);
		if (alen > sizeof(im->addr))
			alen = sizeof(im->addr);
		memcpy(im->addr, RTA_DATA(tb[IFLA_ADDRESS]), alen);
	} else {
		im->alen = 0;
		memset(im->addr, 0, sizeof(im->addr));
	}
	strcpy(im->name, RTA_DATA(tb[IFLA_IFNAME]));
	return 0;
}

void iproute_reset_filter()
{
	memset(&filter, 0, sizeof(filter));
	filter.mdst.bitlen = -1;
	filter.msrc.bitlen = -1;
}

int do_add_route(struct params* params)
{
	/*struct {
		struct nlmsghdr 	n;
		struct rtmsg 		r;
		char   			buf[1024];
	} req;*/
	//int netmask = atoi((char *)params->p[1]);
	/*struct hostent *hostinfo =gethostbyname((char*)params->p[0]);
	struct in_addr addr = *(struct in_addr*)hostinfo->h_addr;
	hostinfo = gethostbyname((char*)params->p[2]);
	struct in_addr gateway = *(struct in_addr*)hostinfo->h_addr;
	hostinfo = gethostbyname((char*)params->p[1]);
	struct in_addr netmask = *(struct in_addr*)hostinfo->h_addr;
	*/
	lkl_add_route((char*)params->p[0],(char*)params->p[1],(char*)params->p[2],(char*)params->p[3]);
	/*struct rtattr * mxrta = (void*)mxbuf;

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST|flags;
	req.n.nlmsg_type = cmd;
	req.r.rtm_family = preferred_family;
	req.r.rstruct hostent *hostinfo =gethostbyname((char*)params->p[0]);
	struct in_addr addr = *(struct in_addr*)hostinfo->h_addr;tm_table = RT_TABLE_MAIN;
	req.r.rtm_scope = RT_SCOPE_NOWHERE;

	if (cmd != RTM_DELROUTE) {
		req.r.rtm_protocol = RTPROT_BOOT;
		req.r.rtm_scope = RT_SCOPE_UNIVERSE;
		req.r.rtm_type = RTN_UNICAST;
	}

	mxrta->rta_type = RTA_METRICS;
	mxrta->rta_len = RTA_LENGTH(0);
	*/
	return 0;
}

int do_remove_route(struct params* params)
{
	lkl_remove_route((char*)params->p[0],(char*)params->p[1],(char*)params->p[2],(char*)params->p[3]);
	/*struct {
		struct nlmsghdr 	n;
		struct rtmsg 		r;
		char   			buf[1024];
	} req;*/
	/*int netmask = atoi((char *)params->p[1]);
	struct hostent *hostinfo =gethostbyname((char*)params->p[0]);
	struct in_addr addr = *(struct in_addr*)hostinfo->h_addr;
	hostinfo = gethostbyname((char*)params->p[2]);
	struct in_addr gateway = *(struct in_addr*)hostinfo->h_addr;

	lkl_remove_route(addr.s_addr,gateway.s_addr,netmask);
	/*struct rtattr * mxrta = (void*)mxbuf;

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST|flags;
	req.n.nlmsg_type = cmd;
	req.r.rtm_family = preferred_family;
	req.r.rtm_table = RT_TABLE_MAIN;
	req.r.rtm_scope = RT_SCOPE_NOWHERE;

	if (cmd != RTM_DELROUTE) {
		req.r.rtm_protocol = RTPROT_BOOT;
		req.r.rtm_scope = RT_SCOPE_UNIVERSE;
		req.r.rtm_type = RTN_UNICAST;
	}

	mxrta->rta_type = RTA_METRICS;
	mxrta->rta_len = RTA_LENGTH(0);
	*/
	return 0;
}


int do_show_ip_route(struct params* params)
{
	if (rtnl_open(&rth,0) < 0) {
		printf("Could not open netlink socket\n");
		return -1;
	}

	iproute_reset_filter();
	filter.tb = RT_TABLE_MAIN;

	if (rtnl_wilddump_request(&rth, AF_UNSPEC, RTM_GETLINK) < 0) {
		printf("Cannot send dump request");
		return -1;
	}

	if (rtnl_dump_filter(&rth, ll_remember_index, &idxmap, NULL, NULL) < 0) {
		printf("Dump terminated\n");
		return -1;
	}

	if (rtnl_wilddump_request(&rth, AF_INET, RTM_GETROUTE) < 0) {
		printf("Cannot send dump request");
		exit(1);
	}

	if (rtnl_dump_filter(&rth, print_route, stdout, NULL, NULL) < 0) {
		fprintf(stderr, "Dump terminated\n");
		exit(1);
	}
	
	return 0;
}
/*{"add", 6, DEVICE_ROUTER, do_add_interface, "Add a new interface", (command*) NULL, "<name> <MAC address> <gateway address> <port no>"}*/
int do_add_interface(struct params *params)
{
	int ifindex = get_interface_index((char*)params->p[0]);
	struct tun_device* td = malloc(sizeof(struct tun_device));
	struct hostent *hostinfo =gethostbyname((char*)params->p[2]);
	struct in_addr addr = *(struct in_addr*)hostinfo->h_addr;
	if (ifindex >= 0){
		lkl_printf("LKL::Interface already exists in system\n");
		return -1;
	}else{
		td->type = TUN_HUB;
		td->port = atoi((char*)params->p[3]);
		td->address = addr.s_addr;
		printf("Port %d\n", td->port);
		if ((ifindex=lkl_add_eth_tun((char*)params->p[0],(char*)params->p[1], 32, td)) < 0) {
			printf("LKL init :: could not add interface %s\n",(char*)params->p[0]);
			return -1;
		}
		lkl_change_ifname(ifindex, params->p[0]);
	}
		
	return 0;
}

int do_set_interface_up(struct params *params)
{
	int ifindex = get_interface_index((char*)params->p[0]);
	if (ifindex < 0){
		lkl_printf("LKL::No such interface in system\n");
		return -1;
	}else
		return lkl_if_up(ifindex);
}

int do_set_interface_down(struct params *params)
{
	int ifindex = get_interface_index((char*)params->p[0]);
	if (ifindex < 0){
		lkl_printf("LKL::No such interface in system\n");
		return -1;
	}else
		return lkl_if_down(ifindex);
}

int do_change_if_address(struct params *params)
{
	int netmask_len = atoi((char*)params->p[2]);
	struct hostent *hostinfo =gethostbyname((char*)params->p[1]);
	struct in_addr addr = *(struct in_addr*)hostinfo->h_addr;
	int ifindex = get_interface_index((char*)params->p[0]);
	if (ifindex < 0){
		lkl_printf("LKL::No such interface in system\n");
		return -1;
	}else
		return lkl_if_set_ipv4(ifindex,addr.s_addr,netmask_len);
}

int do_list_router_interfaces(struct params *params)
{
	lkl_list_interfaces(2);
	return 0;
}

int do_delete_interface(struct params *params)
{
	int ifindex = get_interface_index((char*)params->p[0]);
	if (ifindex < 0){
		lkl_printf("LKL::No such interface in system\n");
		return -1;
	}else 
		return lkl_del_eth_tun(ifindex);
}
