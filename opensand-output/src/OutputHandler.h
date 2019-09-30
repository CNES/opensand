#pragma once

#include <fstream>
#include <mutex>
#include <string>
#include <memory>
#include <stdexcept>
#include <sys/types.h>
#include <netinet/in.h>


class HandlerCreationFailedError : public std::runtime_error {
  public:
    explicit HandlerCreationFailedError(const std::string& what_arg);
};


class BaseProbe;


class StatHandler {
  public:
    virtual void emitStats(const std::vector<std::pair<std::string, std::string>>& probesValues) = 0;
    virtual void configure(const std::vector<std::shared_ptr<BaseProbe>>& probes) = 0;
};


class FileStatHandler : public StatHandler {
  public:
    FileStatHandler(const std::string& fileName, const std::string& originFolder);
    ~FileStatHandler();

    void emitStats(const std::vector<std::pair<std::string, std::string>>& probesValues);
    void configure(const std::vector<std::shared_ptr<BaseProbe>>& probes);

  private:
    std::string buildFullPath() const;

    std::ofstream file;
    unsigned long filesOpened;
    std::string folder;
    std::string filename;
};


class SocketStatHandler : public StatHandler {
  public:
    SocketStatHandler(const std::string& address, unsigned short port, bool useTCP = false);
    ~SocketStatHandler();

    void emitStats(const std::vector<std::pair<std::string, std::string>>& probesValues);
    void configure(const std::vector<std::shared_ptr<BaseProbe>>& probes);

  private:
    int socketFd;
    struct sockaddr_in remote;
    bool useTcp;

    std::vector<std::string> statNames;
};


class LogHandler {
  public:
    virtual void emitLog(const std::string& logName, const std::string& level, const std::string& message) = 0;

  protected:
    std::mutex lock;
};


class FileLogHandler : public LogHandler {
  public:
    FileLogHandler(const std::string& fileName, const std::string& originFolder);
    ~FileLogHandler();

    void emitLog(const std::string& logName, const std::string& level, const std::string& message);

  private:
    std::ofstream file;
};


class SocketLogHandler : public LogHandler {
  public:
    SocketLogHandler(const std::string& address, unsigned short port, bool useTCP = false);
    ~SocketLogHandler();

    void emitLog(const std::string& logName, const std::string& level, const std::string& message);

  private:
    int socketFd;
    struct sockaddr_in remote;
    bool useTcp;
};
