#include "discovery.h"
;

int sendPADI (struct Connection_info *my_connection) {
	char dst_mac[MAC_LEN];
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
	struct pppoe_packet buff;
	memset(&buff,0,sizeof(buff));
	memset(buff.eth_dst_mac,0xFF,MAC_LEN); //PADI is a broadcast
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
	if (send(discovery_socket,&buff,ETH_HEARER_LEN_WITHOUT_CRC+PPPOE_HEADER_LEN+ntohs(buff.pppoe_length),0)) {
		return 0;
	}
	else {
		printf("error in sending PADI of client %i",my_connection->index);
		return 1;
	}
}

int get_ifindex(char *device_name) {
	int sock=socket(AF_PACKET,SOCK_RAW,htons(ETHER_TYPE_DISCOVERY));
	struct ifreq ifr;
	memset(&ifr,0,sizeof(ifr));
	strncpy (ifr.ifr_name,device_name, sizeof(ifr.ifr_name) - 1);
	ifr.ifr_name[sizeof(ifr.ifr_name)-1]='\0';
	if(ioctl(sock,SIOCGIFINDEX,&ifr) == -1) {
		printf("%s\n",strerror(errno));
		exit(1);
	}
	close(sock);
	return ifr.ifr_ifindex;
}

int recv_PADO (struct Connection_info *my_connection) {
	fd_set readable;
	FD_ZERO(&readable);
	FD_SET(discovery_socket,&readable);
	struct timeval tv;
	memset(&tv,0,sizeof(tv));
	tv.tv_sec=TIME_OUT_DISCOVERY;
	int select_count;
	while(select_count=select(discovery_socket+1,&readable,NULL,NULL,&tv)) {
		if (select_count==0) {
			printf("time out in receiving the PADO of client %i\n",my_connection->index);
			return 1;
			}
		if (select_count==-1) {
			printf("error in receiving the PADO of client %i\n",my_connection->index);
			return 1;
			}
		if (select_count==1) {
			struct pppoe_packet buff;
			memset(&buff,0,sizeof(buff));
			ssize_t count;
			count=recv(my_connection->discovery_sock,&buff,sizeof(buff),0)
			if (count <=0) {
				printf("error in receiving the PADO of client %i\n",my_connection->index);
				return 1;
			}
			else {
				if (parse_PADO(&buff,my_connection)) {
					//	got PADO!
					memcpy(my_connection->peer_mac,buff.eth_src_mac,MAC_LEN);
					return 0;
					}
				else {
					//got a packet which is not expected
					continue;
					}
				}
			}
	}
}
			
int parse_PADO (struct pppoe_packet *PADO,struct Connection_info *my_connection) {
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

int set_promisc (char *device_name) {
	int sock=socket(AF_PACKET,SOCK_RAW,htons(ETHER_TYPE_DISCOVERY));
	struct ifreq ifr;
	memset(&ifr,0,sizeof(ifr));
	strncpy (ifr.ifr_name,device_name, sizeof(ifr.ifr_name) - 1);
	ifr.ifr_name[sizeof(ifr.ifr_name)-1]='\0';
	ioctl(sock,SIOCGIFFLAGS,&ifr);
	ifr.ifr_flags |= IFF_PROMISC;
	ioctl(sock,SIOCSIFFLAGS,&ifr);
	close(sock);
	return 0;
}

int sendPADR (struct Connection_info *my_connection) {
	struct sockaddr_ll dst_addr;
	memset(&dst_addr,0,sizeof(dst_addr));
	dst_addr.sll_family=AF_PACKET;
	dst_addr.sll_ifindex=my_connection->ifindex;
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
	if (sendto(my_connection->discovery_sock,&buff,ETH_HEARER_LEN_WITHOUT_CRC+PPPOE_HEADER_LEN+ntohs(buff.pppoe_length),0,(const struct sockaddr *)&dst_addr,sizeof(dst_addr))) {
		return 0;
	}
	else {
		//printf("%s\n",strerror(errno));
		exit;
	}
}

int recv_PADS (struct Connection_info *my_connection) {
	struct PADX_header buff;
	memset(&buff,0,sizeof(buff));
	ssize_t count;
	while (count=recv(my_connection->discovery_sock,&buff,sizeof(buff),0)) {
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

void discovery (struct Connection_info *my_connection) {
	my_connection->my_mac[0]=0x11;
	my_connection->my_mac[1]=0x22;
	my_connection->my_mac[2]=0x33;
	int loop_count;
	for(loop_count=3;loop_count<6;++loop_count) {
		my_connection->my_mac[loop_count]=(int)(rand()/(float)RAND_MAX*255);
	}
	//fill the mac address of our side
	if (sendPADI(my_connection)==1) return;
	if (recv_PADO(my_connection)==1) return;
	if (sendPADR(my_connection)==1) return;
	if (recv_PADS(my_connection)==1) return;
	my_connection->discovery_succeed=1;
}