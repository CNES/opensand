#include "messages.h"

#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/un.h>

#include <opensand_conf/uti_debug.h>

#include "EnvPlane.h"

static const uint32_t MAGIC_NUMBER = 0x5A7D0001;

#define PACKED __attribute__((packed))

struct msg_base_header {
	uint32_t magic;
	uint8_t cmd_type;
} PACKED;

// FIXME: Add some padding after cmd_type to correctly align the following items?

struct msg_register_struct {
	uint32_t magic;
	uint8_t cmd_type;
	uint32_t pid;
	uint8_t num_probes;
	uint8_t num_events;
} PACKED;

struct msg_send_event_struct {
	uint32_t magic;
	uint8_t cmd_type;
	uint8_t event_id;
} PACKED;

void msg_header_register(std::string& message, pid_t pid, uint8_t num_probes, uint8_t num_events)
{
	msg_register_struct header;
	header.magic = htonl(MAGIC_NUMBER);
	header.cmd_type = MSG_CMD_REGISTER;
	header.pid = htonl(pid);
	header.num_probes = num_probes;
	header.num_events = num_events;

	message.append((const char*)&header, sizeof(header));
}

void msg_header_send_probes(std::string& message)
{
	msg_base_header header;
	header.magic = htonl(MAGIC_NUMBER);
	header.cmd_type = MSG_CMD_SEND_PROBES;

	message.append((const char*)&header, sizeof(header));
}

void msg_header_send_event(std::string& message, uint8_t event_id)
{
	msg_send_event_struct header;
	header.magic = htonl(MAGIC_NUMBER);
	header.cmd_type = MSG_CMD_SEND_EVENT;
	header.event_id = event_id;

	message.append((const char*)&header, sizeof(header));
}

uint8_t receive_message(int sock_fd, char* message_data, size_t max_length)
{
	sockaddr_un address;
	socklen_t address_len = sizeof(address);
	ssize_t got = recvfrom(sock_fd, message_data, max_length, 0, (struct sockaddr*)&address, &address_len);

	if (got == 0) {
		// The socket was probably closed
		return 0;
	}

	if (address.sun_family != AF_UNIX || strncmp(address.sun_path, EnvPlane::daemon_sock_addr()->sun_path,
		sizeof(address.sun_path)) != 0) {
		UTI_NOTICE("Got unexpected message from “%s”\n", address.sun_path);
		return 0;
	}

	if (got < 0) {
		UTI_ERROR("Error during message reception: %s\n", strerror(errno));
		return 0;
	}

	if (got < (signed)sizeof(msg_base_header)) {
		UTI_ERROR("Got too short message from daemon!\n");
		return 0;
	}

	if (got > (signed)max_length) {
		UTI_ERROR("Message length overflow (%d > %d), please increase the message buffer size.",
			got, max_length);
		return 0;
	}

	const msg_base_header* head = (const msg_base_header*)message_data;
	if (head->magic != htonl(MAGIC_NUMBER)) {
		UTI_ERROR("Got message with bad magic number %08x\n", ntohl(head->magic));
		return 0;
	}

	return head->cmd_type;
}
