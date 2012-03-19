#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <pwd.h>

int main()
{
	struct ifreq ifr;
	int fd, err;
    int owner;
    int group;

    struct passwd *pwd;

    printf("create interface tun for platine\n");

    pwd = getpwnam("platine");
    owner = pwd->pw_uid;
    group = pwd->pw_gid;

	fd = open("/dev/net/tun", O_RDWR);
	if(fd < 0)
		return fd;

	memset(&ifr, 0, sizeof(ifr));

	/* Flags: IFF_TUN   - TUN device (no Ethernet headers)
	 *        IFF_TAP   - TAP device
	 *        IFF_NO_PI - Do not provide packet information
	 */

	/* create TUN interface */
	snprintf(ifr.ifr_name, IFNAMSIZ, "platine");
	ifr.ifr_flags = IFF_TUN;

	err = ioctl(fd, TUNSETIFF, (void *) &ifr);
	if(err < 0)
	{
        perror("TUNSETIFF");
		close(fd);
		return err;
	}

	err = ioctl(fd, TUNSETPERSIST, 1);
	if(err < 0)
	{
        perror("TUNSETPERSIST");
		close(fd);
		return err;
	}

    err = ioctl(fd, TUNSETOWNER, owner);
	if(err < 0)
	{
        perror("TUNSETOWNER");
		close(fd);
		return err;
	}

    err = ioctl(fd, TUNSETOWNER, owner);
	if(err < 0)
	{
        perror("TUNSETGROUP");
		close(fd);
		return err;
	}

    printf("interface %s created\n", ifr.ifr_name);

	return 0;
}

