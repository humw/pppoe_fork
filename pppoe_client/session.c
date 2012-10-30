//      session.c
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

unsigned char send_CFG_REQ (struct Connection_info *my_connection) {
	my_connection->session_sock=socket(AF_PACKET,SOCK_RAW,htons(ETHER_TYPE_PPP_SESSION));
	if (my_connection->session_sock == -1) {
			printf("errro in creating session socket!\n%s\n",strerror(errno));
		}
	struct magic_number m_num;
	memset(&m_num,0,sizeof(m_num));
	struct MRU mru;
	memset(&mru,0,sizeof(mru));
	struct LCP_packet lcp_packet;
	memset(&lcp_packet,0,sizeof(lcp_packet));
	struct ppp_frame_for_PPPoE buff;
	memset(&buff,0,sizeof(buff));
	
	struct sockaddr_ll dst_addr;
	memset(&dst_addr,0,sizeof(dst_addr));
	dst_addr.sll_family=AF_PACKET;
	dst_addr.sll_ifindex=my_connection->my_ifindex;
	dst_addr.sll_halen=MAC_LEN;
	memcpy(dst_addr.sll_addr,my_connection->peer_mac,MAC_LEN);
	//dst_addr's ready!
	
	m_num.Type=0x05;
	m_num.Length=0x06;
	m_num.Data=(int)(rand()/(float)RAND_MAX*1024);
	// magic number's ready!
	
	mru.Type=0x01;
	mru.Length=0x04;
	mru.Data=htons(1492);
	// mru's ready!
	
	lcp_packet.Code=0x01;
	lcp_packet.Identifier=0x00;
	lcp_packet.Length=htons(4+m_num.Length+mru.Length);
	memcpy(lcp_packet.Data,&mru,sizeof(mru));
	memcpy(lcp_packet.Data+sizeof(mru),&m_num,sizeof(m_num));
	// lcp_packet's ready!
	
	memcpy(buff.DESTINATION_ADDR,my_connection->peer_mac,MAC_LEN);
	memcpy(buff.SOURCE_ADDR,my_connection->my_mac,MAC_LEN);
	buff.ETHER_TYPE=htons(ETHER_TYPE_PPP_SESSION);
	buff.pppoe_type=0x1;
	buff.pppoe_ver=0x1;
	buff.pppoe_code=0x00;
	buff.pppoe_session_id=my_connection->pppoe_session_id;
	buff.pppoe_length=htons(ntohs(lcp_packet.Length)+2);
	buff.Protocol_Field=htons(0xc021);
	memcpy(buff.Information,&lcp_packet,ntohs(lcp_packet.Length));
	//buff's ready!
	
	if (sendto(my_connection->session_sock,&buff,ETH_HEARER_LEN_WITHOUT_CRC+PPPOE_HEADER_LEN+ntohs(buff.pppoe_length),0,(const struct sockaddr *)&dst_addr,sizeof(dst_addr))==-1) {
			printf("error in sending configure request!\n%s\n",strerror(errno));
			exit(1);
	}
	else {
		return lcp_packet.Identifier;
	}
}	

int LCP_handle (struct Connection_info *my_connection,struct ppp_frame_for_PPPoE *buff,unsigned char LCP_ID) {
	struct LCP_packet *lcp_packet=(struct LCP_packet *)buff->Information;
	if(lcp_packet->Code==0x02 && lcp_packet->Identifier==LCP_ID) {
		printf("we got a configure ACK!\n");
		return 0;
	}
	if(lcp_packet->Code==0x01) {
		lcp_packet->Code=0x02;
		struct ppp_frame_for_PPPoE cfg_req_ack;
		struct sockaddr_ll dst_addr;
		memset(&dst_addr,0,sizeof(dst_addr));
		dst_addr.sll_family=AF_PACKET;
		dst_addr.sll_ifindex=my_connection->my_ifindex;
		dst_addr.sll_halen=MAC_LEN;
		memcpy(dst_addr.sll_addr,my_connection->peer_mac,MAC_LEN);
		//dst_addr's ready!
		memcpy(cfg_req_ack.DESTINATION_ADDR,my_connection->peer_mac,MAC_LEN);
		memcpy(cfg_req_ack.SOURCE_ADDR,my_connection->my_mac,MAC_LEN);
		cfg_req_ack.ETHER_TYPE=htons(ETHER_TYPE_PPP_SESSION);
		cfg_req_ack.pppoe_type=0x1;
		cfg_req_ack.pppoe_ver=0x1;
		cfg_req_ack.pppoe_code=0x00;
		cfg_req_ack.pppoe_session_id=my_connection->pppoe_session_id;
		cfg_req_ack.pppoe_length=htons(ntohs(lcp_packet->Length)+2);
		cfg_req_ack.Protocol_Field=htons(0xc021);
		memcpy(cfg_req_ack.Information,lcp_packet,ntohs(lcp_packet->Length));
		//cfg_ack's ready!
		int sent_count=sendto(my_connection->session_sock,&cfg_req_ack,ETH_HEARER_LEN_WITHOUT_CRC+PPPOE_HEADER_LEN+ntohs(cfg_req_ack.pppoe_length),0,(const struct sockaddr *)&dst_addr,sizeof(dst_addr));
		if(sent_count==-1) {
			printf("error in sending config ACK!\n%s\n",strerror(errno));
			exit(1);
		}
		return 0;
	}
	return 99;
}	
	
void session (struct Connection_info *my_connection) {
	unsigned char LCP_ID=send_CFG_REQ(my_connection);
	struct timeval tv;
	tv.tv_sec=3;
	tv.tv_usec=0;
	fd_set readable;
	FD_ZERO(&readable);
	FD_SET(my_connection->session_sock,&readable);
	int count;
	for(;;) {
		count=select(my_connection->session_sock+1,&readable,NULL,NULL,&tv);
		if(count==-1) {
			printf("error in selecting!\n%s\n",strerror(errno));
			exit(1);
		}
		if(count==0) {
			printf("no packet hits the session socket!\n");
			exit(1);
		}
		if(count>0) {
			struct ppp_frame_for_PPPoE buff;
			memset(&buff,0,sizeof(buff));
			ssize_t recv_count;
			recv_count=recv(my_connection->session_sock,&buff,sizeof(buff),0);
			if(recv_count==-1) {
				printf("error in receiving packets from session socket!\n%s\n",strerror(errno));
				exit(1);
			}
			if(buff.pppoe_session_id!=my_connection->pppoe_session_id) {
				continue;
			}
			if(buff.Protocol_Field==htons(0xc021)) {
				LCP_handle(my_connection,&buff,LCP_ID);
			}
		}
	}
}

	

