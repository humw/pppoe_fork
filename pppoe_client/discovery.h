#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <linux/sockios.h>
#include <string.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>	
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "md5.h"







#define MAC_LEN 6 //the length of a mac address
#define MTU 1500
#define ETHER_TYPE_DISCOVERY 0x8863
#define ETHER_TYPE_PPP_SESSION 0x8864
#define MAX_ETH_PACKET_LEN 1514
#define ETH_HEARER_LEN_WITHOUT_CRC 14
#define PPPOE_HEADER_LEN 6

#define Service_Name 0x0101
#define Host_Uniq 0x0103
#define CODE_OF_PADI 0x09
#define CODE_OF_PADR 0x19
#define CODE_OF_PADS 0x65
#define CODE_OF_PPP_SESSION 0x00
#define LCP 0xc021
#define CHAP 0xc223
#define IPCP 0x8021

struct PADX_header {
	char eth_dst_mac[MAC_LEN];
	char eth_src_mac[MAC_LEN];
	unsigned short int eth_type;
	char pppoe_type:4;
	char pppoe_ver:4;
	char pppoe_code;
	unsigned short int pppoe_session_id;
	unsigned short int pppoe_length;
	char payload[MAX_ETH_PACKET_LEN-14-6];
};

struct Connection_info {
	char my_mac[6];
	char peer_mac[6];
	char *my_NIC;
	unsigned short int host_uniq;
	int my_ifindex;
	unsigned short int pppoe_session_id;
	int session_sock;
	unsigned int LCP_magic_number;
};

struct PPPOE_TAG {
	unsigned short int TAG_TYPE; // 2 bytes
	unsigned short int TAG_LENGTH;//2 bytes
	unsigned char TAG_VALUE[MAX_ETH_PACKET_LEN];
};

struct ppp_frame_for_PPPoE {
	char DESTINATION_ADDR[MAC_LEN];
	char SOURCE_ADDR[MAC_LEN];
	unsigned short int ETHER_TYPE;
	char pppoe_type:4;
	char pppoe_ver:4;
	char pppoe_code;
	unsigned short int pppoe_session_id;
	unsigned short int pppoe_length;
	unsigned short int Protocol_Field; //identifies the datagram encapsulated in the Information field
	char Information[MTU-6-2];
};

struct LCP_packet {
	unsigned char Code;
	unsigned char Identifier;
	unsigned short int Length; //indicates the length of the LCP packet, including the Code, Identifier, Length and Data fields
	char Data[MTU-6-2-1-1-2];
};

struct magic_number {
	unsigned char Type;
	unsigned char Length;
	unsigned int Data;
};

struct MRU {
	unsigned char Type;
	unsigned char Length;
	unsigned short int Data;
};

struct CHAP_value {
	unsigned char Value_size;
	unsigned char Value[16];
	unsigned char Name[4];
};

struct IPCP_ip_address {
	unsigned char type;
	unsigned char length;
	unsigned char data[4];
};
