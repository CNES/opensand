/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file OutputHandler.h
 * @brief Handlers that implements behaviour to send outputs outside of OpenSAND.
 * @author Mathias Ettinger   <mathias.ettinger@viveris.fr>
 */


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


class Handler {
  public:
    Handler(const std::string& entityName);

  protected:
    std::string entityName;
};


class StatHandler : public Handler {
  public:
    StatHandler(const std::string& entityName);
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
    SocketStatHandler(const std::string& entityName, const std::string& address, unsigned short port, bool useTCP = false);
    ~SocketStatHandler();

    void emitStats(const std::vector<std::pair<std::string, std::string>>& probesValues);
    void configure(const std::vector<std::shared_ptr<BaseProbe>>& probes);

  private:
    int socketFd;
    struct sockaddr_in remote;
    bool useTcp;

    std::vector<std::string> statNames;
};


class LogHandler : public Handler {
  public:
    LogHandler(const std::string& entityName);
    virtual void emitLog(const std::string& logName, const std::string& level, const std::string& message) = 0;

  protected:
    void prepareMessage(std::ostream& formatter, const std::string& logName, const std::string& level, const std::string& message);

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
    SocketLogHandler(const std::string& entityName, const std::string& address, unsigned short port, bool useTCP = false);
    ~SocketLogHandler();

    void emitLog(const std::string& logName, const std::string& level, const std::string& message);

  private:
    int socketFd;
    struct sockaddr_in remote;
    bool useTcp;
};
