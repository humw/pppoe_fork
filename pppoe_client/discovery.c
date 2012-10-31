#include "discovery.h"
;

int sendPADI (int sock,struct Connection_info *my_connection) {
	my_connection->my_ifindex=get_ifindex(sock,my_connection->my_NIC);
	char dst_mac[MAC_LEN];
	memset(dst_mac,0xff,MAC_LEN);
	//dst_mac is a broadcast
	struct sockaddr_ll dst_addr;
	memset(&dst_addr,0,sizeof(dst_addr));
	dst_addr.sll_family=AF_PACKET;
	dst_addr.sll_ifindex=my_connection->my_ifindex;
	dst_addr.sll_halen=MAC_LEN;
	memcpy(dst_addr.sll_addr,dst_mac,MAC_LEN);
	//dst_addr's ready!
	struct PPPOE_TAG tag1;
	memset(&tag1,0,sizeof(tag1));
	tag1.TAG_TYPE=htons(Service_Name);
	tag1.TAG_LENGTH=0x0000;
	struct PPPOE_TAG tag2;
	memset(&tag2,0,sizeof(tag2));
	tag2.TAG_TYPE=htons(Host_Uniq);
	tag2.TAG_LENGTH=htons(0x0002);
	my_connection->host_uniq=(unsigned short int)(rand()/(float)RAND_MAX*65535);
	unsigned short int host_uniq_n=htons(my_connection->host_uniq);
	memcpy(&tag2.TAG_VALUE,&host_uniq_n,sizeof(host_uniq_n));
	//printf("the host uniq we sent is %#X------------\n",my_connection->host_uniq);
	//TAGs are ready!!
	struct PADX_header buff;
	memset(&buff,0,sizeof(buff));
	memcpy(buff.eth_dst_mac,dst_mac,MAC_LEN);
	memcpy(buff.eth_src_mac,my_connection->my_mac,MAC_LEN);
	buff.eth_type=htons(ETHER_TYPE_DISCOVERY);
	buff.pppoe_type=0x1;
	buff.pppoe_ver=0x1;
	buff.pppoe_code=CODE_OF_PADI;
	buff.pppoe_session_id=0x0000;
	buff.pppoe_length=htons(sizeof(tag1.TAG_LENGTH)+sizeof(tag1.TAG_TYPE)+ntohs(tag2.TAG_LENGTH)+sizeof(tag2.TAG_TYPE)+sizeof(tag2.TAG_LENGTH));
	memcpy(buff.payload,&tag1,sizeof(tag1.TAG_TYPE)+sizeof(tag1.TAG_LENGTH));
	memcpy(buff.payload+sizeof(tag1.TAG_TYPE)+sizeof(tag1.TAG_LENGTH),&tag2,sizeof(tag2.TAG_TYPE)+sizeof(tag2.TAG_LENGTH)+ntohs(tag2.TAG_LENGTH));
	//buff's ready!
	if (sendto(sock,&buff,ETH_HEARER_LEN_WITHOUT_CRC+PPPOE_HEADER_LEN+ntohs(buff.pppoe_length),0,(const struct sockaddr *)&dst_addr,sizeof(dst_addr))) {
		return 0;
	}
	else {
		//printf("%s\n",strerror(errno));
		exit;
	}
}

int get_ifindex(int sock,char *device_name) {
	struct ifreq ifr;
	memset(&ifr,0,sizeof(ifr));
	strncpy (ifr.ifr_name,device_name, sizeof(ifr.ifr_name) - 1);
	ifr.ifr_name[sizeof(ifr.ifr_name)-1]='\0';
	if(ioctl(sock,SIOCGIFINDEX,&ifr) == -1) {
		printf("%s\n",strerror(errno));
		exit(1);
	}
	return ifr.ifr_ifindex;
}

int recv_PADO (int sock,struct Connection_info *my_connection) {
	//printf("receiving...\n");
	struct PADX_header buff;
	memset(&buff,0,sizeof(buff));
	ssize_t count;
	while (count=recv(sock,&buff,sizeof(buff),0)) {
	//	printf("count is %i\n",count);
		if (count >0) {
			if (parse_PADO(&buff,my_connection)) {
			//	printf("got PADO!\n");
				memcpy(my_connection->peer_mac,buff.eth_src_mac,MAC_LEN);
				return 0;
			}
			else {
			//	printf("got a packet which is not expected\n");
				continue;
			}
		}
		else {
		//	printf("recv_PADO failed!\t%s\n",strerror(errno));
			exit(2);
		}
	}
}
	
	
int parse_PADO (struct PADX_header *PADO,struct Connection_info *my_connection) {
	//printf("start to parse PADO\n");
	if (memcmp(PADO->eth_dst_mac,my_connection->my_mac,MAC_LEN) ==0) {
	int i;
		for(i=0;i<ntohs(PADO->pppoe_length);) {
			struct PPPOE_TAG *tag=(struct PPPOE_TAG *)(PADO->payload+i);
			//printf("TAG TYPE is %x\n",ntohs(tag->TAG_TYPE));
			//printf("TAG LENGTH is %x\n",ntohs(tag->TAG_LENGTH));
			if (tag->TAG_TYPE == htons(Host_Uniq)) {
				unsigned short int host_uniq_in_packet;
				memcpy(&host_uniq_in_packet,tag->TAG_VALUE,ntohs(tag->TAG_LENGTH));
				//printf("HSTUIQ in packet -----%#X\n",ntohs(host_uniq_in_packet));
				//printf("HSTUIQ in connection -----%#X\n",my_connection->host_uniq);
				if(my_connection->host_uniq == ntohs(host_uniq_in_packet)) {
					//printf("host uniq value equal!\nPADO got!\n");
					return 1;
				}
				else {
					return 0;
				}
			}
			else {
				i+=(sizeof(tag->TAG_TYPE)+sizeof(tag->TAG_LENGTH)+ntohs(tag->TAG_LENGTH));
			}
		}
	}
	return 0;
}

int set_promisc (int sock,char *device_name) {
	struct ifreq ifr;
	memset(&ifr,0,sizeof(ifr));
	strncpy (ifr.ifr_name,device_name, sizeof(ifr.ifr_name) - 1);
	ifr.ifr_name[sizeof(ifr.ifr_name)-1]='\0';
	ioctl(sock,SIOCGIFFLAGS,&ifr);
	ifr.ifr_flags |= IFF_PROMISC;
	ioctl(sock,SIOCSIFFLAGS,&ifr);
	return 0;
}

int sendPADR (int sock,struct Connection_info *my_connection) {
	struct sockaddr_ll dst_addr;
	memset(&dst_addr,0,sizeof(dst_addr));
	dst_addr.sll_family=AF_PACKET;
	dst_addr.sll_ifindex=my_connection->my_ifindex;
	dst_addr.sll_halen=MAC_LEN;
	memcpy(dst_addr.sll_addr,my_connection->peer_mac,MAC_LEN);
	//dst_addr's ready!
	struct PPPOE_TAG tag1;
	memset(&tag1,0,sizeof(tag1));
	tag1.TAG_TYPE=htons(Service_Name);
	tag1.TAG_LENGTH=0x0000;
	struct PPPOE_TAG tag2;
	memset(&tag2,0,sizeof(tag2));
	tag2.TAG_TYPE=htons(Host_Uniq);
	tag2.TAG_LENGTH=htons(0x0002);
	unsigned short int host_uniq_n=htons(my_connection->host_uniq);
	memcpy(&tag2.TAG_VALUE,&host_uniq_n,sizeof(host_uniq_n));
	
	//TAGs are ready!!
	struct PADX_header buff;
	memset(&buff,0,sizeof(buff));
	memcpy(buff.eth_dst_mac,my_connection->peer_mac,MAC_LEN);
	memcpy(buff.eth_src_mac,my_connection->my_mac,MAC_LEN);
	buff.eth_type=htons(ETHER_TYPE_DISCOVERY);
	buff.pppoe_type=0x1;
	buff.pppoe_ver=0x1;
	buff.pppoe_code=CODE_OF_PADR;
	buff.pppoe_session_id=0x0000;
	buff.pppoe_length=htons(sizeof(tag1.TAG_LENGTH)+sizeof(tag1.TAG_TYPE)+ntohs(tag2.TAG_LENGTH)+sizeof(tag2.TAG_TYPE)+sizeof(tag2.TAG_LENGTH));
	memcpy(buff.payload,&tag1,sizeof(tag1.TAG_TYPE)+sizeof(tag1.TAG_LENGTH));
	memcpy(buff.payload+sizeof(tag1.TAG_TYPE)+sizeof(tag1.TAG_LENGTH),&tag2,sizeof(tag2.TAG_TYPE)+sizeof(tag2.TAG_LENGTH)+ntohs(tag2.TAG_LENGTH));
	if (sendto(sock,&buff,ETH_HEARER_LEN_WITHOUT_CRC+PPPOE_HEADER_LEN+ntohs(buff.pppoe_length),0,(const struct sockaddr *)&dst_addr,sizeof(dst_addr))) {
		return 0;
	}
	else {
		//printf("%s\n",strerror(errno));
		exit;
	}
}

int recv_PADS (int sock,struct Connection_info *my_connection) {
	struct PADX_header buff;
	memset(&buff,0,sizeof(buff));
	ssize_t count;
	while (count=recv(sock,&buff,sizeof(buff),0)) {
		//printf("count is %i\n",count);
		if (count >0) {
			if (parse_PADS(&buff,my_connection)) {
				//printf("got PADS!\n");
			//	printf("the session id is %i\n",ntohs(buff.pppoe_session_id));
				memcpy(&my_connection->pppoe_session_id,&buff.pppoe_session_id,sizeof(buff.pppoe_session_id));
				return 0;
			}
			else {
		//		printf("got a packet which is not expected\n");
				continue;
			}
		}
		else {
			//printf("recv_PADS failed!\t%s\n",strerror(errno));
			exit(2);
		}
	}
}


int parse_PADS (struct PADX_header *PADS,struct Connection_info *my_connection) {
	//printf("start to parse PADS\n");
	if(PADS->pppoe_code==CODE_OF_PADS) {
		return 1;
	}
	return 0;
}
