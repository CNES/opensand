#include "CommandThread.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <opensand_conf/uti_debug.h>

#include "EnvPlane.h"
#include "messages.h"

CommandThread::CommandThread(int sock_fd)
{
	this->sock_fd = sock_fd;
}

int CommandThread::start()
{
	pthread_t thread;
	if (pthread_create(&thread, NULL, CommandThread::_run, this) < 0) {
		UTI_ERROR("Unable to start the command listener thread : %s", strerror(errno));
		return 1;
	}
	
	return 0;
}

void* CommandThread::_run(void* arg)
{
	CommandThread* self = (CommandThread*)arg;
	self->run();
	
	return NULL;
}

void CommandThread::run()
{
	char buffer[4096];
	
	for (;;) {
		uint8_t command_id = receive_message(this->sock_fd, buffer, sizeof(buffer));
		
		if (command_id == 0) {
			return;
		}
		
		if (command_id == MSG_CMD_ENABLE_PROBE || command_id == MSG_CMD_DISABLE_PROBE) {
			uint8_t probe_id = buffer[5];
			bool enabling = (command_id == MSG_CMD_ENABLE_PROBE);
			
			EnvPlane::set_probe_state(probe_id, enabling);
			continue;
		}
		
		UTI_ERROR("Received a message with unknown command ID %d\n", command_id);
	}
}
