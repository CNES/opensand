#ifndef _MESSAGES_H
#define _MESSAGES_H

#include <string>
#include <vector>

#include <stdint.h>

#define MSG_CMD_REGISTER 1
#define MSG_CMD_ACK 2
#define MSG_CMD_SEND_PROBES 3
#define MSG_CMD_SEND_EVENT 4
#define MSG_CMD_ENABLE_PROBE 5
#define MSG_CMD_DISABLE_PROBE 6

#define DAEMON_SOCK_NAME "sand-daemon.socket"
#define SELF_SOCK_NAME "program-%d.socket"

void msg_header_register(std::string& message, pid_t pid, uint8_t num_probes, uint8_t num_events);
void msg_header_send_probes(std::string& message, uint32_t timestamp);
void msg_header_send_event(std::string& message, uint8_t event_id);

uint8_t receive_message(int sock_fd, char* message_data, size_t max_length);

#endif
