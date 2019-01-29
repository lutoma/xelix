#include <time.h>
#include <sched.h>
#include <stdbool.h>
#include <pico_stack.h>
#include <pico_ipv4.h>
#include <pico_icmp4.h>
#include <pico_dhcp_client.h>
#include <pico_dev_xelix.h>
#include <pico_socket.h>
#include <pico_ipv4.h>

#define BSIZE (1024 * 10)
uint32_t dhcpclient_xid;
static char recvbuf[BSIZE];


void telnet_cb(uint16_t ev, struct pico_socket *s) {
	if (ev & PICO_SOCK_EV_CONN) {
		struct pico_ip4 orig = {0};
		uint16_t port = 0;
		struct pico_socket *sock_a = pico_socket_accept(s, &orig, &port);

		char peer[30] = {0};
		pico_ipv4_to_string(peer, orig.addr);
		printf("Telnet connection from %s:%d.\n", peer, short_be(port));
	}

	if (ev & PICO_SOCK_EV_RD) {
		int r = pico_socket_read(s, recvbuf, BSIZE);
		recvbuf[r - 1] = 0;
		printf("Received %s\n", recvbuf);

		size_t buf_size = r >= 150 ? r : 150;
		char* buf = calloc(buf_size, 1);
		memcpy(buf, recvbuf, r);

		if(pico_socket_write(s, (const uint8_t *)buf, buf_size) == -1) {
			fprintf(stderr, "Could not write.\n");
		}

		free(buf);
	}
}

void callback_dhcpclient(void* cli, int code) {
	if(code & PICO_DHCP_ERROR) {
		printf("DHCP failed.\n");
		return;
	}

	struct pico_ip4 ipaddr = pico_dhcp_get_address(cli);
	char ip[16];
	pico_ipv4_to_string(ip, ipaddr.addr);
	printf("DHCP done, IP %s\n", ip);

	struct pico_socket* s = pico_socket_open(PICO_PROTO_IPV4, PICO_PROTO_TCP, &telnet_cb);
	if(!s) {
		fprintf(stderr, "Error opening.\n");
	}

	uint16_t listen_port = short_be(23);
	if(pico_socket_bind(s, &ipaddr.addr, &listen_port) < 0) {
		fprintf(stderr, "Error binding.\n");
	}

	if(pico_socket_listen(s, 40) < 0) {
		fprintf(stderr, "Error listening.\n");
	}
}

int main(void){
	pico_stack_init();
	struct pico_device* dev = pico_xelix_create("rtl1");
	if(!dev) {
		return 1;
	}

	pico_dhcp_initiate_negotiation(dev, &callback_dhcpclient, &dhcpclient_xid);

	while(true) {
		pico_stack_tick();
	}

	return 0;
}
