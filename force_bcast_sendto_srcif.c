// SPDX-License-Identifier: GPL-2.0+
/* force_bcast_sendto_srcif.c
 * Author: Simon Polack
 *
 * LD_PRELOAD interposer that intercepts sendto() calls towards
 * broadcast address (255.255.255.255) and forces them to use the interface
 * specified by the BCAST_IFACE environment variable.
 *
 * Useful for redirecting broadcast traffic from multi-interface-unaware
 * Art-Net senders (e.g. Infinity-Chimp) on hosts with multiple interfaces.
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>


static int force_ifindex = -1; // force_ifindex: -1 = uninitialized, 0 = disabled/error, >0 = active index

static ssize_t (*real_sendto)(int, const void*, size_t, int, const struct sockaddr*, socklen_t);
static ssize_t (*real_sendmsg)(int, const struct msghdr*, int);

ssize_t sendto(int fd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest, socklen_t destlen) {

    // Initialize
    if (force_ifindex == -1) {
        real_sendto = dlsym(RTLD_NEXT, "sendto");
        real_sendmsg = dlsym(RTLD_NEXT, "sendmsg");

        const char *iface = getenv("BCAST_IFACE");
        force_ifindex = (iface) ? (int)if_nametoindex(iface) : 0;
    }

    // check for broadcast ipv4 -> artnet
    if (force_ifindex > 0 && dest && dest->sa_family == AF_INET &&
        ((struct sockaddr_in*)dest)->sin_addr.s_addr == 0xFFFFFFFF) {

        struct iovec iov = { .iov_base = (void*)buf, .iov_len = len };

        // Prepare Control Message Buffer (on stack, fast)
        char cbuf[CMSG_SPACE(sizeof(struct in_pktinfo))];
        memset(cbuf, 0, sizeof(cbuf)); // Zero out to ensure ipi_spec_dst is 0

        struct msghdr msg = {
            .msg_name = (void*)dest,
            .msg_namelen = destlen,
            .msg_iov = &iov,
            .msg_iovlen = 1,
            .msg_control = cbuf,
            .msg_controllen = sizeof(cbuf)
        };

        // Construct the IP_PKTINFO header
        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = IPPROTO_IP;
        cmsg->cmsg_type  = IP_PKTINFO;
        cmsg->cmsg_len   = CMSG_LEN(sizeof(struct in_pktinfo));

        // Set out ifindex
        ((struct in_pktinfo*)CMSG_DATA(cmsg))->ipi_ifindex = force_ifindex;

        return real_sendmsg(fd, &msg, flags);
    }

    return real_sendto(fd, buf, len, flags, dest, destlen);
}
