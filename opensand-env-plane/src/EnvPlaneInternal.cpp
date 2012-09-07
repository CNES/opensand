#include "EnvPlaneInternal.h"

#include <vector>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <opensand_conf/uti_debug.h>

#include "CommandThread.h"
#include "messages.h"

EnvPlaneInternal::EnvPlaneInternal()
{
	this->enabled = false;
	this->initializing = false;

	memset(&this->daemon_sock_addr, 0, sizeof(this->daemon_sock_addr));
	memset(&this->self_sock_addr, 0, sizeof(this->self_sock_addr));

	this->sock = 0;
}

EnvPlaneInternal::~EnvPlaneInternal()
{
	for (std::size_t i = 0 ; i < this->probes.size() ; i++) {
		delete this->probes[i];
	}

	for (std::size_t i = 0 ; i < this->events.size() ; i++) {
		delete this->events[i];
	}

	if (this->sock != 0) {
		// Close the command socket
		// (will exit the command thread)
		shutdown(this->sock, SHUT_RDWR);
		close(this->sock);

		// Remove the socket file
		const char* path = this->self_sock_addr.sun_path;
		if (unlink(path) < 0) {
			UTI_ERROR("Unable to delete the socket \"%s\": %s", path, strerror(errno));
		}
	}
}

void EnvPlaneInternal::init(bool enabled, event_level min_level, const char* sock_prefix)
{
	UTI_PRINT(LOG_INFO, "Starting environment plane initialization (%s)\n", enabled ? "enabled" : "disabled");

	this->enabled = enabled;
	this->min_level = min_level;
	this->initializing = true;

	if (sock_prefix == NULL) {
		sock_prefix = "/var/run/sand-daemon";
	}

	this->daemon_sock_addr.sun_family = AF_UNIX;
	char* path = this->daemon_sock_addr.sun_path;
	snprintf(path, sizeof(this->daemon_sock_addr.sun_path), "%s/" DAEMON_SOCK_NAME, sock_prefix);

	this->self_sock_addr.sun_family = AF_UNIX;
	path = this->self_sock_addr.sun_path;
	snprintf(path, sizeof(this->self_sock_addr.sun_path), "%s/" SELF_SOCK_NAME, sock_prefix, getpid());

	UTI_DEBUG("Daemon socket address is \"%s\", own socket address is \"%s\".",
		this->daemon_sock_addr.sun_path, this->self_sock_addr.sun_path);
}

Event* EnvPlaneInternal::register_event(const char* identifier, event_level level)
{
	if (!this->enabled)
		return 0;

	assert(this->initializing);

	UTI_DEBUG("Registering event %s with level %d\n", identifier, level);

	uint8_t new_id = this->events.size();
	Event* event = new Event(new_id, identifier, level);
	this->events.push_back(event);

	return event;
}

bool EnvPlaneInternal::finish_init()
{

	if (!this->enabled)
		return true;

	assert(this->initializing);

	UTI_PRINT(LOG_INFO, "Opening environment plane communication socket\n");

	// Initialization of the UNIX socket

	this->sock = socket(AF_UNIX, SOCK_DGRAM, 0);

	if (this->sock == -1) {
		UTI_ERROR("Socket allocation failed: %s\n", strerror(errno));
		return false;
	}

	const char* path = this->self_sock_addr.sun_path;
	unlink(path);

	sockaddr_un address;
	memset(&address, 0, sizeof(address));
	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, path, sizeof(address.sun_path) - 1);
	if (bind(this->sock, (const sockaddr*)&address, sizeof(address)) < 0) {
		UTI_ERROR("Socket binding failed: %s\n", strerror(errno));
		return false;
	}

	// Send the initial probe list
	std::string message;

	msg_header_register(message, getpid(), this->probes.size(), this->events.size());

	for (std::size_t i = 0 ; i < this->probes.size() ; i++) {
		const char* name = this->probes[i]->name();

		message.append(1, (((int)this->probes[i]->is_enabled()) << 7) | this->probes[i]->storage_type_id());
		message.append(1, strlen(name));
		message.append(name);
	}

	for (std::size_t i = 0 ; i < this->events.size() ; i++) {
		const char* identifier = this->events[i]->identifier();

		message.append(1, this->events[i]->level());
		message.append(1, strlen(identifier));
		message.append(identifier);
	}

	if (sendto(this->sock, message.data(), message.size(), 0, (const sockaddr*)&this->daemon_sock_addr,
		sizeof(this->daemon_sock_addr)) < (signed)message.size()) {
		UTI_ERROR("Sending initial probe list failed: %s\n", strerror(errno));
		return false;
	}

	// Wait for the ACK response

	char buffer[32];
	alarm(10);
	uint8_t command_id = receive_message(this->sock, buffer, sizeof(buffer));
	alarm(0);
	if (command_id != MSG_CMD_ACK) {
		UTI_ERROR("Incorrect ACK response for initial probe list\n");
		return false;
	}

	// Start the command thread
	CommandThread* command_thread = new CommandThread(this->sock);
	command_thread->start();

	this->initializing = false;

	UTI_PRINT(LOG_INFO, "Environment plane initialized.\n");
	return true;
}

void EnvPlaneInternal::send_probes()
{
	if (!this->enabled)
		return;

	bool needs_sending = false;

	std::string message;
	msg_header_send_probes(message);

	for (std::size_t i = 0 ; i < this->probes.size() ; i++) {
		BaseProbe* probe = this->probes[i];
		if (probe->is_enabled() && probe->values_count != 0) {
			needs_sending = true;
			message.append(1, (uint8_t)i);
			probe->append_value_and_reset(message);
		}
	}

	if (!needs_sending)
		return;

	if (sendto(this->sock, message.data(), message.size(), 0, (const sockaddr*)&this->daemon_sock_addr,
		sizeof(this->daemon_sock_addr)) < (signed)message.size()) {
		UTI_ERROR("Sending probe values failed: %s\n", strerror(errno));
	}
}

void EnvPlaneInternal::send_event(Event* event, const char* message_text)
{
	if (!this->enabled || event->level() < this->min_level)
		return;

	std::string message;
	msg_header_send_event(message, event->id);
	message.append(message_text);

	if (sendto(this->sock, message.data(), message.size(), 0, (const sockaddr*)&this->daemon_sock_addr,
		sizeof(this->daemon_sock_addr)) < (signed)message.size()) {
		UTI_ERROR("Sending event failed: %s\n", strerror(errno));
	}
}

void EnvPlaneInternal::set_probe_state(uint8_t probe_id, bool enabled)
{
	UTI_DEBUG("%s probe %s\n", enabled ? "Enabling" : "Disabling", this->probes[probe_id]->name());
	this->probes[probe_id]->enabled = enabled;
}
