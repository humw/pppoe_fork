void session(Connection_info *my_connection)
{
    fd_set readable;
    PADX_header packet;
    struct timeval tv;
    struct timeval *tvp = NULL;
    int maxFD = 0;
    int r;

    /* Prepare for select() */
    if (my_connection->session_sock > maxFD)   maxFD = my_connection->session_sock;
    maxFD++;

    /* Fill in the constant fields of the packet to save time */
    memcpy(packet.eth_dst_mac, my_connection->peer_mac, MAC_LEN);
    memcpy(packet.eth_src_mac, my_connection->my_mac, MAC_LEN);
    packet.eth_type= htons(ETHER_TYPE_PPP_SESSION);
    packet.ver = 1;
    packet.type = 1;
    packet.code = 0X00;
    packet.session = my_connection->pppoe_session_id;

    for (;;) {
	    tv.tv_sec = 2;
	    tv.tv_usec = 0;
	    tvp = &tv;
		FD_ZERO(&readable);
		FD_SET(0, &readable);     /* ppp packets come from stdin */
		FD_SET(my_connection->session_sock, &readable);
	while(1) {
	    r = select(maxFD, &readable, NULL, NULL, tvp);
	    if (r >= 0) break;
	}

	if (r == 0) { /* Inactivity timeout */
	    exit(10);
	}

	/* Handle ready sockets */
	if (FD_ISSET(0, &readable)) {
		packet_from_pppd(my_connection, &packet);
	}

	if (FD_ISSET(my_connection->session_sock, &readable)) {
		packet_from_session_sock();
		}
	   
	}
    }

void packet_from_pppd (Connection_info *my_connection,PADX_header *packet) {
	struct iovec vec[2];
	char to_be_discard[2];
	struct sockaddr_ll dst_addr;
	memset(&dst_addr,0,sizeof(dst_addr));
	dst_addr.sll_family=AF_PACKET;
	dst_addr.sll_ifindex=my_connection->my_ifindex;
	dst_addr.sll_halen=MAC_LEN;
	memcpy(dst_addr.sll_addr,my_connection->peer_mac,MAC_LEN);
	vec[0].iov_base=to_be_discard;
	vec[0].iov_len=2
	vec[1].iov_base=packet->payload;
	vec[1].iov_len=MAX_ETH_PACKET_LEN-14-6;
	int r=readv(0,vec,2);
	sendto(my_connection->session_sock,packet,r-2+6+14,0,(const struct sockaddr *)&dst_addr,sizeof(dst_addr));
}

void packet_from_session_sock (Connection_info *my_connection) {
	struct PADX_header buff;
	ssize_t count;
	count=recv(my_connection->session_sock,&buff,sizeof(buff),0);
	if (buff.pppoe_session_id == my_connection->pppoe_session_id) {
		struct iovec iov[2];
		char addctl[2];
		addctl[0]=0XFF;
		addctl[1]=0X03;
		iov[0].iov_base=addctl;
		iov[0].iov_len=2;
		iov[1].iov_base=buff.payload;
		iov[1].iov_len=count-14-6;
		writev(1,iov,2);
		}
}
	
	
	


