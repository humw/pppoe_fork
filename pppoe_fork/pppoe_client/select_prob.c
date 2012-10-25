//      select_prob.c
//      
//      Copyright 2012 hmw <ssungle@gmail.com>
//      
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//      
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//      
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.
//      
//      


#include <stdio.h>
#include <sys/select.h>
#include "discovery.h"

int main(int argc, char **argv)
{
	int sock;
	sock=socket(AF_PACKET,SOCK_RAW,htons(ETHER_TYPE_DISCOVERY));
	if (!sock) {
		printf("%s\n",strerror(errno));
		return 0;
	}
	struct Connection_info con_info;
	memset(&con_info,0,sizeof(con_info));
	con_info.my_NIC=argv[1];
	memset(con_info.my_mac,0x69,6);
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(sock,&fdset);
	struct timeval timeout;
	memset(&timeout,0,sizeof(timeout));
	timeout.tv_sec=5;
	timeout.tv_usec=0;
	int status;
	sendPADI(sock,&con_info);
	status=select(sock+1,&fdset,NULL,NULL,&timeout);
	switch(status) {
		case 0:
			printf("timeout!\n");
			break;
		case -1:
			printf("%s\n",strerror(errno));
			break;
		default:
			recv_PADO(sock,&con_info);
			break;
	}
	return 0;
}

