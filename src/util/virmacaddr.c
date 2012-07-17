/*
 * virmacaddr.c: MAC address handling
 *
 * Copyright (C) 2006-2012 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Authors:
 *     Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>

#include "c-ctype.h"
#include "virmacaddr.h"
#include "virrandom.h"

/* Compare two MAC addresses, ignoring differences in case,
 * as well as leading zeros.
 */
int
virMacAddrCompare(const char *p, const char *q)
{
    unsigned char c, d;
    do {
        while (*p == '0' && c_isxdigit(p[1]))
            ++p;
        while (*q == '0' && c_isxdigit(q[1]))
            ++q;
        c = c_tolower(*p);
        d = c_tolower(*q);

        if (c == 0 || d == 0)
            break;

        ++p;
        ++q;
    } while (c == d);

    if (UCHAR_MAX <= INT_MAX)
        return c - d;

    /* On machines where 'char' and 'int' are types of the same size, the
       difference of two 'unsigned char' values - including the sign bit -
       doesn't fit in an 'int'.  */
    return c > d ? 1 : c < d ? -1 : 0;
}

/**
 * virMacAddrCmp:
 * @mac1: pointer to 1st MAC address
 * @mac2: pointer to 2nd MAC address
 *
 * Return 0 if MAC addresses are equal,
 * < 0 if mac1 < mac2,
 * > 0 if mac1 > mac2
 */
int
virMacAddrCmp(const virMacAddrPtr mac1, const virMacAddrPtr mac2)
{
    return memcmp(mac1->addr, mac2->addr, VIR_MAC_BUFLEN);
}

/**
 * virMacAddrCmpRaw:
 * @mac1: pointer to 1st MAC address
 * @mac2: pointer to 2nd MAC address in plain buffer
 *
 * Return 0 if MAC addresses are equal,
 * < 0 if mac1 < mac2,
 * > 0 if mac1 > mac2
 */
int
virMacAddrCmpRaw(const virMacAddrPtr mac1,
                 const unsigned char mac2[VIR_MAC_BUFLEN])
{
    return memcmp(mac1->addr, mac2, VIR_MAC_BUFLEN);
}

/**
 * virMacAddrSet
 * @dst: pointer to destination
 * @src: pointer to source
 *
 * Copy src to dst
 */
void
virMacAddrSet(virMacAddrPtr dst, const virMacAddrPtr src)
{
    memcpy(dst, src, sizeof(*src));
}

/**
 * virMacAddrSetRaw
 * @dst: pointer to destination to hold MAC address
 * @src: raw MAC address data
 *
 * Set the MAC address to the given value
 */
void
virMacAddrSetRaw(virMacAddrPtr dst, const unsigned char src[VIR_MAC_BUFLEN])
{
    memcpy(dst->addr, src, VIR_MAC_BUFLEN);
}

/**
 * virMacAddrGetRaw
 * @src: pointer to MAC address
 * @dst: pointer to raw memory to write MAC address into
 *
 * Copies the MAC address into raw memory
 */
void
virMacAddrGetRaw(virMacAddrPtr src, unsigned char dst[VIR_MAC_BUFLEN])
{
    memcpy(dst, src->addr, VIR_MAC_BUFLEN);
}

/**
 * virMacAddrParse:
 * @str: string representation of MAC address, e.g., "0:1E:FC:E:3a:CB"
 * @addr: 6-byte MAC address
 *
 * Parse a MAC address
 *
 * Return 0 upon success, or -1 in case of error.
 */
int
virMacAddrParse(const char* str, virMacAddrPtr addr)
{
    int i;

    errno = 0;
    for (i = 0; i < VIR_MAC_BUFLEN; i++) {
        char *end_ptr;
        unsigned long result;

        /* This is solely to avoid accepting the leading
         * space or "+" that strtoul would otherwise accept.
         */
        if (!c_isxdigit(*str))
            break;

        result = strtoul(str, &end_ptr, 16); /* exempt from syntax-check */

        if ((end_ptr - str) < 1 || 2 < (end_ptr - str) ||
            (errno != 0) ||
            (0xFF < result))
            break;

        addr->addr[i] = (unsigned char) result;

        if ((i == 5) && (*end_ptr == '\0'))
            return 0;
        if (*end_ptr != ':')
            break;

        str = end_ptr + 1;
    }

    return -1;
}

void virMacAddrFormat(const virMacAddrPtr addr,
                      char *str)
{
    snprintf(str, VIR_MAC_STRING_BUFLEN,
             "%02X:%02X:%02X:%02X:%02X:%02X",
             addr->addr[0], addr->addr[1], addr->addr[2],
             addr->addr[3], addr->addr[4], addr->addr[5]);
    str[VIR_MAC_STRING_BUFLEN-1] = '\0';
}

void virMacAddrGenerate(const unsigned char prefix[VIR_MAC_PREFIX_BUFLEN],
                        virMacAddrPtr addr)
{
    addr->addr[0] = prefix[0];
    addr->addr[1] = prefix[1];
    addr->addr[2] = prefix[2];
    addr->addr[3] = virRandomBits(8);
    addr->addr[4] = virRandomBits(8);
    addr->addr[5] = virRandomBits(8);
}

/* The low order bit of the first byte is the "multicast" bit. */
bool
virMacAddrIsMulticast(const virMacAddrPtr mac)
{
    return !!(mac->addr[0] & 1);
}

bool
virMacAddrIsUnicast(const virMacAddrPtr mac)
{
    return !(mac->addr[0] & 1);
}
