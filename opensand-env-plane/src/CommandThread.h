#ifndef _COMMAND_THREAD_H
#define _COMMAND_THREAD_H

class CommandThread {
public:
	CommandThread(int sock_fd);

	int start();

private:
	static void* _run(void* arg);

	~CommandThread();
	void run();
	
	int sock_fd;
};

#endif
