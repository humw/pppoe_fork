//      pppoe_multiple.c
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


#include "discovery.h"


int main(int argc, char **argv)
{
	const char *options="c:i:";
	int opt_status=0;
	int pppoe_count=1;
	char *NIC_name;
	while((opt_status=getopt(argc,argv,options))!=-1)
		switch (opt_status) {
			case 'c':
				pppoe_count=atoi(optarg);
				break;
			case 'i':
				NIC_name=optarg;
				break;
			default:
				break;
		}
	//how many pppoe client to fork is determined!
	
	struct Connection_info infos[pppoe_count];
	memset(infos,0,sizeof(infos));
	//shuttles for pppoe infomations
	
	srand((unsigned int)getpid()); //ready for rand
	set_promisc(NIC_name); //set the NIC to be used promisc mode
	
	
	
	int count1;
	for(count1=0;count1<pppoe_count;++count1) {
		infos[count1].ifindex=get_ifindex(NIC_name); //get the ifindex of NIC
		infos[count1].my_mac[0]=0x11;
		infos[count1].my_mac[1]=0x22;
		infos[count1].my_mac[2]=0x33;
		int loop_count;
		for(loop_count=3;loop_count<6;++loop_count) {
		infos[count1].my_mac[loop_count]=(int)(rand()/(float)RAND_MAX*255);
		}
		//src mac of package done!
		infos[count1].discovery_sock=socket(AF_PACKET,SOCK_RAW,htons(ETHER_TYPE_DISCOVERY));
		if(infos[count1].discovery_sock<0) {
			printf("error in creating discovery socket\n%s\n",strerror(errno));
			exit(1);
		}
		//discovery socket's ready!
		sendPADI(&infos[count1]);
		recv_PADO(&infos[count1]);
		sendPADR(&infos[count1]);
		recv_PADS(&infos[count1]);
		if(fork()==0) {
			session(&infos[count1]);
		}
		printf("client %i is forked!\n",count1+1);
		sleep(1);
	}
	return 0;
}


