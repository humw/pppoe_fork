//      0924.c
//      
//      Copyright 2012 hmw <hmw@hmw-office-left>
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



#include "discovery.h"
;


int main (int argc, char **argv) {
	if(argc !=2) {
		fputs("plz specify the NIC to be used!\n",stdout);
		return 1;
	}
	int sock;
	sock=socket(AF_PACKET,SOCK_RAW,htons(ETHER_TYPE_DISCOVERY));
	if(sock<0) {
	//	printf("%s\n",strerror(errno));
		exit(1);
	}
	//create a socket!
	
	srand((unsigned int)getpid()); //ready for rand
	
	struct Connection_info my_connection;
	my_connection.my_mac[0]=0x11;
	my_connection.my_mac[1]=0x22;
	my_connection.my_mac[2]=0x33;
	int loop_count;
	for(loop_count=3;loop_count<6;++loop_count) {
		my_connection.my_mac[loop_count]=(int)(rand()/(float)RAND_MAX*255);
	}
	//src mac of package done!
	my_connection.my_NIC=argv[1];

	set_promisc(sock,my_connection.my_NIC);
	sendPADI(sock,&my_connection);
	recv_PADO(sock,&my_connection);
	sendPADR(sock,&my_connection);
	recv_PADS(sock,&my_connection);
	session(&my_connection);
	return 0;
}

