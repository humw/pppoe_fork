include "discovery.h"
char *NIC;
unsigned short int clients;
int ifindex;

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
		