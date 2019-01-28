#include <time.h>
#include <sched.h>
#include <pico_stack.h>
#include <pico_ipv4.h>
#include <pico_icmp4.h>
#include <pico_dhcp_client.h>
#include <pico_dev_xelix.h>
#include <pico_socket.h>
#include <pico_ipv4.h>

#define BSIZE (1024 * 10)
static char recvbuf[BSIZE];
static int pos = 0, len = 0;
static int flag = 0;

int main(void){
	pico_stack_init();
	struct pico_device* dev = pico_xelix_create("rtl1");
	if(!dev) {
		return 1;
	}

	struct pico_ip4 ipaddr, netmask, gateway, route;
	pico_string_to_ipv4("10.111.1.42", &ipaddr.addr);
	pico_string_to_ipv4("255.255.255.0", &netmask.addr);
	pico_string_to_ipv4("10.111.1.1", &gateway.addr);
	pico_string_to_ipv4("0.0.0.0", &route.addr);

	pico_ipv4_link_add(dev, ipaddr, netmask);

	struct pico_ipv4_link* link = pico_ipv4_link_get(&ipaddr);
	pico_ipv4_route_add(route, route, gateway, 100, link);

	//pico_dhcp_initiate_negotiation(dev, &callback_dhcpclient, &dhcpclient_xid);
	//pico_icmp4_ping("172.22.119.4", NUM_PING, 500, 10000, 64, cb_ping);

	printf("Launched PicoTCP.\n");

	while (1) {
		pico_stack_tick();
	}

	return 0;
}
