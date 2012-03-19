
#include <iostream>

#include <IpAddress.h>
#include <Ipv4Address.h>
#include <Ipv6Address.h>

int main(int argc, char *argv[])
{
	IpAddress *ipv4_addr_1, *ipv4_addr_2, *ipv4_addr_3;
	IpAddress *ipv6_addr_1, *ipv6_addr_2;
	bool failure;

	failure = false;

	ipv4_addr_1 = new Ipv4Address(192, 168, 0, 1);
	ipv4_addr_2 = new Ipv4Address(192, 168, 0, 2);
	ipv4_addr_3 = new Ipv4Address(192, 0, 0, 2);

	ipv6_addr_1 = new Ipv6Address(0x20, 0x01, 0x06, 0x60,
											0x66, 0x02, 0x01, 0x02,
											0x00, 0x00, 0x00, 0x00,
											0x00, 0x00, 0x00, 0x01);
	ipv6_addr_2 = new Ipv6Address(0x20, 0x01, 0x06, 0x60,
											0x66, 0x02, 0x01, 0x02,
											0x00, 0x00, 0x00, 0x00,
											0x00, 0x00, 0x00, 0x0a);

#define match(addr1, addr2, mask, attended_result) \
	do { \
		bool result; \
		result = (addr1)->matchAddressWithMask((addr2), (mask)); \
		std::cout << "match " << (addr1)->str() << "/" << (mask) << " with " \
		          << (addr2)->str() << "/" << (mask) << " => " \
		          << (result ? "yes" : "no") << " (attended = " \
		          << (attended_result ? "yes" : "no") << ")" \
		          << std::endl; \
		if(result != (attended_result)) \
			failure = true; \
	} while(0)

	match(ipv4_addr_1, ipv4_addr_2, 0, true);
	match(ipv4_addr_1, ipv4_addr_2, 8, true);
	match(ipv4_addr_1, ipv4_addr_2, 16, true);
	match(ipv4_addr_1, ipv4_addr_2, 24, true);
	match(ipv4_addr_1, ipv4_addr_2, 29, true);
	match(ipv4_addr_1, ipv4_addr_2, 30, true);
	match(ipv4_addr_1, ipv4_addr_2, 31, false);
	match(ipv4_addr_1, ipv4_addr_2, 32, false);

	match(ipv4_addr_1, ipv4_addr_3, 0, true);
	match(ipv4_addr_1, ipv4_addr_3, 7, true);
	match(ipv4_addr_1, ipv4_addr_3, 8, true);
	match(ipv4_addr_1, ipv4_addr_3, 9, false);
	match(ipv4_addr_1, ipv4_addr_3, 16, false);
	match(ipv4_addr_1, ipv4_addr_3, 24, false);
	match(ipv4_addr_1, ipv4_addr_3, 32, false);

	match(ipv6_addr_1, ipv6_addr_2, 64, true);
	match(ipv6_addr_1, ipv6_addr_2, 80, true);
	match(ipv6_addr_1, ipv6_addr_2, 96, true);
	match(ipv6_addr_1, ipv6_addr_2, 112, true);
	match(ipv6_addr_1, ipv6_addr_2, 124, true);
	match(ipv6_addr_1, ipv6_addr_2, 125, false);
	match(ipv6_addr_1, ipv6_addr_2, 126, false);
	match(ipv6_addr_1, ipv6_addr_2, 127, false);
	match(ipv6_addr_1, ipv6_addr_2, 128, false);

	delete ipv4_addr_1;
	delete ipv4_addr_2;
	delete ipv4_addr_3;

	delete ipv6_addr_1;
	delete ipv6_addr_2;

	return (failure ? 1 : 0);
}
