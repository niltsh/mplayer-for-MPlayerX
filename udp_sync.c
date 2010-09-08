/*
 * Network playback synchronization
 * Copyright (C) 2009 Google Inc.
 *
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#if !HAVE_WINSOCK2_H
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <signal.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif /* HAVE_WINSOCK2_H */

#include "mplayer.h"
#include "mp_core.h"
#include "mp_msg.h"
#include "help_mp.h"
#include "udp_sync.h"


// config options for UDP sync
int udp_master = 0;
int udp_slave  = 0;
int udp_port   = 23867;
const char *udp_ip = "127.0.0.1"; // where the master sends datagrams
                                  // (can be a broadcast address)
float udp_seek_threshold = 1.0;   // how far off before we seek

// remember where the master is in the file
static float udp_master_position = -1.0;

// how far off is still considered equal
#define UDP_TIMING_TOLERANCE 0.02

static void set_blocking(int fd, int blocking)
{
    long sock_flags;
#if HAVE_WINSOCK2_H
    sock_flags = blocking;
    ioctlsocket(fd, FIONBIO, &sock_flags);
#else
    sock_flags = fcntl(fd, F_GETFL, 0);
    sock_flags = blocking ? sock_flags & ~O_NONBLOCK : sock_flags | O_NONBLOCK;
    fcntl(fd, F_SETFL, sock_flags);
#endif /* HAVE_WINSOCK2_H */
}

// gets a datagram from the master with or without blocking.  updates
// master_position if successful.  if the master has exited, returns 1.
// returns -1 on error.
// otherwise, returns 0.
static int get_udp(int blocking, float *master_position)
{
    struct sockaddr_in cliaddr;
    char mesg[100];
    socklen_t len;

    int chars_received = -1;
    int n;

    static int sockfd = -1;
    if (sockfd == -1) {
        struct timeval tv = { .tv_sec = 30 };
        struct sockaddr_in servaddr = { 0 };

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1)
            return -1;

        servaddr.sin_family      = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port        = htons(udp_port);
        bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    }

    set_blocking(sockfd, blocking);

    len = sizeof(cliaddr);

    while (-1 != (n = recvfrom(sockfd, mesg, sizeof(mesg)-1, 0,
                               (struct sockaddr *)&cliaddr, &len))) {
        // flush out any further messages so we don't get behind
        if (chars_received == -1)
            set_blocking(sockfd, 0);

        chars_received = n;
        mesg[chars_received] = 0;
        if (strcmp(mesg, "bye") == 0)
            return 1;
    }

    // UDP wait error, probably a timeout.  Safe to ignore.
    if (chars_received == -1)
        return 0;

    // make sure we at least do not crash in case a
    // failed recvfrom has corrupted the buffer
    mesg[chars_received] = 0;

    sscanf(mesg, "%f", master_position);
    return 0;
}

void send_udp(const char *send_to_ip, int port, char *mesg)
{
    static int sockfd = -1;
    static struct sockaddr_in socketinfo;

    if (sockfd == -1) {
        static const int one = 1;
        int ip_valid = 0;

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1)
            exit_player(EXIT_ERROR);

        // Enable broadcast
        setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));

#if HAVE_WINSOCK2_H
        socketinfo.sin_addr.s_addr = inet_addr(send_to_ip);
        ip_valid = socketinfo.sin_addr.s_addr != INADDR_NONE;
#else
        ip_valid = inet_aton(send_to_ip, &socketinfo.sin_addr);
#endif

        if (!ip_valid) {
            mp_msg(MSGT_CPLAYER, MSGL_FATAL, MSGTR_InvalidIP);
            exit_player(EXIT_ERROR);
        }

        socketinfo.sin_family = AF_INET;
        socketinfo.sin_port   = htons(port);
    }

    sendto(sockfd, mesg, strlen(mesg), 0, (struct sockaddr *) &socketinfo,
           sizeof(socketinfo));
}

// this function makes sure we stay as close as possible to the master's
// position.  returns 1 if the master tells us to exit, 0 otherwise.
int udp_slave_sync(MPContext *mpctx)
{
    // grab any waiting datagrams without blocking
    int master_exited = get_udp(0, &udp_master_position);

    while (!master_exited) {
        float my_position = mpctx->sh_video->pts;

        // if we're way off, seek to catch up
        if (FFABS(my_position - udp_master_position) > udp_seek_threshold) {
            abs_seek_pos  = SEEK_ABSOLUTE;
            rel_seek_secs = udp_master_position;
            break;
        }

        // normally we expect that the master will have just played the
        // frame we're ready to play.  break out and play it, and we'll be
        // right in sync.
        // or, the master might be up to a few seconds ahead of us, in
        // which case we also want to play the current frame immediately,
        // without waiting.
        // UDP_TIMING_TOLERANCE is a small value that lets us consider
        // the master equal to us even if it's very slightly ahead.
        if (udp_master_position + UDP_TIMING_TOLERANCE > my_position)
            break;

        // the remaining case is that we're slightly ahead of the master.
        // usually, it just means we called get_udp() before the datagram
        // arrived.  call get_udp again, but this time block until we receive
        // a datagram.
        master_exited = get_udp(1, &udp_master_position);
    }

    return master_exited;
}
