include "discovery.h"
char *NIC;
unsigned short int clients;
int ifindex;
int discovery_socket;
int session_socket;
void main (int argc,char * * argv) {
	const char *options="i:c:";
	int option_character;
	while ( (option_character=getopt(argc,argv,options))!=-1)
		switch (option_character) {
			case 'i':
				NIC=optarg;
				break;
			case 'c':
				clients=atoi(optarg);
				break;
			default:
				useage();
				break;
			}
	srand((unsigned int)getpid()); //ready for rand
	unsigned short int count_client;
	struct Connection_info infos[clients];
	memset(infos,0,sizeof(infos));
	ifindex=get_ifindex(NIC);
	discovery_socket=socket(AF_PACKET,SOCK_RAW,htons(ETHER_TYPE_DISCOVERY));
	session_socket=socket(AF_PACKET,SOCK_RAW,htons(ETHER_TYPE_PPP_SESSION));
	if (discovery_socket == -1 || session_socket == -1) {
		printf("error in creating socket!\n%s\n",strerror(errno));
		exit(1);
		}
	struct sockaddr_ll sockaddr_used_to_bind_dis;
	memset(&sockaddr_used_to_bind_dis,0,sizeof(sockaddr_used_to_bind_dis));
	sockaddr_used_to_bind_dis.sll_protocol=ETHER_TYPE_DISCOVERY;
	sockaddr_used_to_bind_dis.sll_ifindex=ifindex;
	if (bind(discovery_socket,(const struct sockaddr *)&sockaddr_used_to_bind_dis),sizeof(sockaddr_used_to_bind_dis)==-1) {
		printf("failed in binding discovery socket\n%s\n",strerror(errno));
		exit(1);
		}
	struct sockaddr_ll sockaddr_used_to_bind_ses;
	memset(&sockaddr_used_to_bind_ses,0,sizeof(sockaddr_used_to_bind_ses));
	sockaddr_used_to_bind_ses.sll_protocol=ETHER_TYPE_PPP_SESSION;
	sockaddr_used_to_bind_ses.sll_ifindex=ifindex;
	if (bind(session_socket,(const struct sockaddr *)&sockaddr_used_to_bind_ses),sizeof(sockaddr_used_to_bind_ses)==-1) {
		printf("failed in binding session socket\n%s\n",strerror(errno));
		exit(1);
		}
	
	for (count_client=0;count_client<clients;count_client++) {
		infos[count_client].index=count_client+1;
		if (infos[count_client].discovery_succeed==0 || infos[count_client].dial_in_complete==0) {
			discovery(&infos[count_client]);
			session(&infos[count_client]);
			}
		if (infos[count_client].dial_in_complete==1) {
			continue;
			}
		}
}	
		