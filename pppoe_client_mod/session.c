#include "discovery.h"
;

unsigned char send_CFG_REQ (struct Connection_info *my_connection) {
	struct magic_number m_num;
	memset(&m_num,0,sizeof(m_num));
	struct MRU mru;
	memset(&mru,0,sizeof(mru));
	struct LCP_packet lcp_packet;
	memset(&lcp_packet,0,sizeof(lcp_packet));
	struct ppp_frame_for_pppoe ppp_frame;
	memset(&ppp_frame,0,sizeof(ppp_frame));
	struct pppoe_packet buff;
	memset(&buff,0,sizeof(buff));
	
	
	m_num.Type=0x05;
	m_num.Length=0x06;
	m_num.Data=htons((int)(rand()/(float)RAND_MAX*2048)); // use 10 bits!
	my_connection->LCP_magic_number=m_num.Data;
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
	
	ppp_frame.Protocol_Field=htons(LCP);
	memcpy(ppp_frame.Information,&lcp_packet,ntohs(lcp_packet.Length));
	// ppp frame 's ready!
	
	memcpy(buff.DESTINATION_ADDR,my_connection->peer_mac,MAC_LEN);
	memcpy(buff.SOURCE_ADDR,my_connection->my_mac,MAC_LEN);
	buff.ETHER_TYPE=htons(ETHER_TYPE_PPP_SESSION);
	buff.pppoe_type=0x1;
	buff.pppoe_ver=0x1;
	buff.pppoe_code=CODE_OF_PPP_SESSION;
	buff.pppoe_session_id=my_connection->pppoe_session_id;
	buff.pppoe_length=htons(ntohs(lcp_packet.Length)+2);
	memcpy(buff.payload,&ppp_frame,ntohs(lcp_packet.Length)+2);
	//buff's ready!
	
	if (send(session_socket,&buff,ETH_HEARER_LEN_WITHOUT_CRC+PPPOE_HEADER_LEN+ntohs(buff.pppoe_length),0) {
			printf("error in sending configure request!\n%s\n",strerror(errno));
			exit(1);
	}
	else {
		return lcp_packet.Identifier;
	}
}	

int LCP_handle (struct Connection_info *my_connection,struct pppoe_packet *buff,unsigned char LCP_ID) {
	struct ppp_frame_for_pppoe *ppp_frame=(struct ppp_frame_for_pppoe *)buff->payload;
	struct LCP_packet *lcp_packet=(struct LCP_packet *)ppp_frame->Information;
	if(lcp_packet->Code==0x02 && lcp_packet->Identifier==LCP_ID) {
		//we got a configure ACK!
		return 0;
	}
	if(lcp_packet->Code==0x09) {
		lcp_packet->Code=0x0a;
		unsigned char temp[MAC_LEN];
		memcpy(temp,buff->eth_src_mac,MAC_LEN);
		memcpy(buff->eth_src_mac,buff->eth_dst_mac,MAC_LEN);
		memcpy(buff->eth_dst_mac,temp,MAC_LEN);
		buff->pppoe_length=htons(10);
		lcp_packet->Length=htons(8);
		memcpy(lcp_packet->Data,&my_connection->LCP_magic_number,4);
		int sent_count=send(session_socket,buff,ETH_HEARER_LEN_WITHOUT_CRC+PPPOE_HEADER_LEN+ntohs(buff->pppoe_length),0);
		if(sent_count==-1) {
			printf("error in sending echo reply! client %i\n%s\n",my_connection->index,strerror(errno));
			exit(1);
		}
	}

		
		
	if(lcp_packet->Code==0x01) {
		lcp_packet->Code=0x02;
		struct pppoe_packet cfg_req_ack;
		memcpy(cfg_req_ack.eth_dst_mac,my_connection->peer_mac,MAC_LEN);
		memcpy(cfg_req_ack.eth_src_mac,my_connection->my_mac,MAC_LEN);
		cfg_req_ack.eth_type=htons(ETHER_TYPE_PPP_SESSION);
		cfg_req_ack.pppoe_type=0x1;
		cfg_req_ack.pppoe_ver=0x1;
		cfg_req_ack.pppoe_code=0x00;
		cfg_req_ack.pppoe_session_id=my_connection->pppoe_session_id;
		cfg_req_ack.pppoe_length=htons(ntohs(lcp_packet->Length)+2);
		struct ppp_frame_for_pppoe *ppp_frame_cfg_req_ack=(struct ppp_frame_for_pppoe *)cfg_req_ack.payload;
		ppp_frame_cfg_req_ack->Protocol_Field=htons(LCP);
		memcpy(ppp_frame_cfg_req_ack.Information,lcp_packet,ntohs(lcp_packet->Length));
		//cfg_ack's ready!
		int sent_count=send(session_socket,&cfg_req_ack,ETH_HEARER_LEN_WITHOUT_CRC+PPPOE_HEADER_LEN+ntohs(cfg_req_ack.pppoe_length),0);
		if(sent_count==-1) {
			printf("error in sending config ACK! client %i\n%s\n",my_connection->index,strerror(errno));
			exit(1);
		}
		return 0;
	}
	return 1;
}	

int CHAP_handle (struct Connection_info *my_connection,struct pppoe_packet *buff) {
	struct ppp_frame_for_pppoe *ppp_frame=(struct ppp_frame_for_pppoe *)buff->payload;
	struct LCP_packet *chap_packet=(struct LCP_packet *)ppp_frame->Information;
	if(chap_packet->Code==3) {
		//CHAP succeed!
		return 1;
	}
	if(chap_packet->Code==4) {
		printf("CHAP failed! client %i\n",my_connection->index);
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
		memcpy(temp,buff->eth_dst_mac,MAC_LEN);
		memcpy(buff->eth_dst_mac,buff->eth_src_mac,MAC_LEN);
		memcpy(buff->eth_src_mac,temp,MAC_LEN);
		buff->pppoe_length=htons(1+16+4+2+1+1+2);//caculate by myself!
		chap_packet->Code=2;
		chap_packet->Length=htons(1+1+2+1+16+4);//caculate by myself!
		struct CHAP_value md5_value;
		memset(&md5_value,0,sizeof(md5_value));
		md5_value.Value_size=16;
		memcpy(md5_value.Value,md5,16);
		memcpy(md5_value.Name,"aaaa",4);
		memcpy(chap_packet->Data,&md5_value,sizeof(md5_value));
		int send_count=send(session_socket,buff,ETH_HEARER_LEN_WITHOUT_CRC+PPPOE_HEADER_LEN+ntohs(buff->pppoe_length),0);
		if (send_count==-1) {
			printf("errno is sending the response of CHAP challenge! client %i\n%s\n",my_connection->index,strerror(errno));
			exit(1);
		}
	}
}
		
void send_IPCP_REQ (struct Connection_info *my_connection) {
	struct pppoe_packet buff;
	memset(&buff,0,sizeof(buff));
	memcpy(buff.eth_dst_mac,my_connection->peer_mac,MAC_LEN);
	memcpy(buff.eth_src_mac,my_connection->my_mac,MAC_LEN);
	buff.eth_type=htons(ETHER_TYPE_PPP_SESSION);
	buff.pppoe_type=1;
	buff.pppoe_ver=1;
	buff.pppoe_code=0;
	buff.pppoe_session_id=my_connection->pppoe_session_id;
	buff.pppoe_length=htons(12); //caculate by myself
	struct ppp_frame_for_pppoe *ppp_frame=(struct ppp_frame_for_pppoe *)buff.payload;
	ppp_frame->Protocol_Field=htons(IPCP);
	struct LCP_packet *ipcp_packet=(struct LCP_packet *)ppp_frame->Information;
	ipcp_packet->Code=1;
	ipcp_packet->Identifier=0;
	ipcp_packet->Length=htons(10);
	struct IPCP_ip_address *ip_address=(struct IPCP_ip_address *)ipcp_packet->Data;
	ip_address->type=3;
	ip_address->length=6;
	ip_address->data=0;
	//buff's ready!
	int send_count=send(session_socket,&buff,ETH_HEARER_LEN_WITHOUT_CRC+PPPOE_HEADER_LEN+ntohs(buff.pppoe_length),0);
	if (send_count==-1) {
		printf("error in sending IPCP_CFG_REQ! client %i\n%s\n",my_connection->index,strerror(errno));
		exit(1);
	}
}

	
int IPCP_handle (struct Connection_info *my_connection,struct pppoe_packet *buff) {
	struct ppp_frame_for_pppoe *ppp_frame=(struct ppp_frame_for_pppoe *)buff->payload;
	struct LCP_packet *ipcp_packet=(struct LCP_packet *)ppp_frame->Information;
	if(ipcp_packet->Code==1 || ipcp_packet->Code==3) {
		if(ipcp_packet->Code==1) ipcp_packet->Code=2;
		if(ipcp_packet->Code==3) {
			ipcp_packet->Code=1;
			ipcp_packet->Identifier++;
		}
		unsigned char temp[MAC_LEN];
		memcpy(temp,buff->eth_src_mac,MAC_LEN);
		memcpy(buff->eth_src_mac,buff->eth_dst_mac,MAC_LEN);
		memcpy(buff->eth_dst_mac,temp,MAC_LEN);
		int send_count=send(session_socket,buff,ETH_HEARER_LEN_WITHOUT_CRC+PPPOE_HEADER_LEN+ntohs(buff->pppoe_length),0);		
		if (send_count==-1) {
		printf("error in sending IPCP_CFG_ACK or IPCP_CFG_REQ! client %i\n%s\n",my_connection->index,strerror(errno));
		exit(1);
		}
		return 1;
	}
	if(ipcp_packet->Code==2) {
		//we get a IPCP_CFG_ACK
		return 1;
	}
}
	
	
	
	
	
void session (struct Connection_info *my_connection) {
	if (my_connection->discovery_succeed && fork()==0) {
		
		unsigned char LCP_ID=send_CFG_REQ(my_connection);
		fd_set readable;
		FD_ZERO(&readable);
		FD_SET(session_socket,&readable);
	
	for(;;) {
		struct timeval tv;
		tv.tv_sec=TIME_OUT_SESSION;
		tv.tv_usec=0;
		int count;
		count=select(session_socket+1,&readable,NULL,NULL,&tv);
		if(count==-1) {
			printf("error in selecting!\n%s\n",strerror(errno));
			exit(1);
		}
		if(count==0) {
			printf("no packet hits the session socket!\n");
			exit(1);
		}
		if(count>0) {
			struct pppoe_packet buff;
			memset(&buff,0,sizeof(buff));
			ssize_t recv_count;
			struct ppp_frame_for_pppoe *ppp_frame=(struct ppp_frame_for_pppoe *)buff.payload;
			recv_count=recv(session_socket,&buff,sizeof(buff),0);
			if(recv_count==-1) {
				printf("error in receiving packets from session socket!\n%s\n",strerror(errno));
				exit(1);
			}
			if(buff.pppoe_session_id!=my_connection->pppoe_session_id) {
				continue;
			}
			if(ppp_frame->Protocol_Field==htons(LCP)) {
				LCP_handle(my_connection,&buff,LCP_ID);
			}
			if(ppp_frame->Protocol_Field==htons(CHAP)) {
				if(CHAP_handle(my_connection,&buff)==1) {
					send_IPCP_REQ(my_connection);
				}
			}
			if(ppp_frame->Protocol_Field==htons(IPCP)) {
				IPCP_handle(my_connection,&buff);
			}
		}
	}
	}
}
	

