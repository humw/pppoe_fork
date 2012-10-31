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
;

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
		cfg_req_ack.Protocol_Field=htons(LCP);
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

int CHAP_handle (struct Connection_info *my_connection,struct ppp_frame_for_PPPoE *buff) {
	struct LCP_packet *chap_packet=(struct LCP_packet *)buff->Information;
	if(chap_packet->Code==3) {
		printf("CHAP succeed!\n");
		return 1;
	}
	if(chap_packet->Code==4) {
		printf("CHAP failed!\n");
		exit(1);
	}
	if(chap_packet->Code==1) {
		unsigned char string_to_be_digest[100];
		memset(&string_to_be_digest,0,100);
		memcpy(string_to_be_digest,&chap_packet->Identifier,sizeof(chap_packet->Identifier));
		memcpy(string_to_be_digest+sizeof(chap_packet->Identifier),"aaaa",4);//CHAP secret!!!
		struct CHAP_value *chap_value=(struct CHAP_value *)chap_packet->Data;
		memcpy(string_to_be_digest+sizeof(chap_packet->Identifier)+4,chap_value->Value,chap_value->Value_size);
		
		unsigned char md5[16];
		memset(md5,0,16);
		struct MD5Context ctx;
		MD5Init(&ctx);
		MD5Update(&ctx,string_to_be_digest,sizeof(chap_packet->Identifier)+4+chap_value->Value_size);
		MD5Final(md5,&ctx);
		
		unsigned char temp[MAC_LEN];
		memcpy(temp,buff->DESTINATION_ADDR,MAC_LEN);
		memcpy(buff->DESTINATION_ADDR,buff->SOURCE_ADDR,MAC_LEN);
		memcpy(buff->SOURCE_ADDR,temp,MAC_LEN);
		buff->pppoe_length=htons(1+16+4+2+1+1+2);//caculate by myself!
		chap_packet->Code=2;
		chap_packet->Length=htons(1+1+2+1+16+4);//caculate by myself!
		struct CHAP_value md5_value;
		memset(&md5_value,0,sizeof(md5_value));
		md5_value.Value_size=16;
		memcpy(md5_value.Value,md5,16);
		memcpy(md5_value.Name,"aaaa",4);
		memcpy(chap_packet->Data,&md5_value,sizeof(md5_value));
		
		struct sockaddr_ll dst_addr;
		memset(&dst_addr,0,sizeof(dst_addr));
		dst_addr.sll_family=AF_PACKET;
		dst_addr.sll_ifindex=my_connection->my_ifindex;
		dst_addr.sll_halen=MAC_LEN;
		memcpy(dst_addr.sll_addr,my_connection->peer_mac,MAC_LEN);
		//dst_addr's ready!
		
		int send_count=sendto(my_connection->session_sock,buff,ETH_HEARER_LEN_WITHOUT_CRC+PPPOE_HEADER_LEN+ntohs(buff->pppoe_length),0,(const struct sockaddr *)&dst_addr,sizeof(dst_addr));
		if (send_count==-1) {
			printf("errno is sending the response of CHAP challenge!\n%s\n",strerror(errno));
			exit(1);
		}
	}
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
			if(buff.Protocol_Field==htons(LCP)) {
				LCP_handle(my_connection,&buff,LCP_ID);
			}
			if(buff.Protocol_Field==htons(CHAP)) {
				if(CHAP_handle(my_connection,&buff)==1) {
					//send_IPCP_REQ(my_connection);
				}
			}
			/*if(buff.Protocol_Field==htons(IPCP)) {
				IPCP_handle(my_connection);
			}*/
		}
	}
}

	

