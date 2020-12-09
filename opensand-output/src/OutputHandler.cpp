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
 * @file OutputHandler.cpp
 * @brief Implementation of Handlers to send outputs outside of OpenSAND.
 * @author Mathias Ettinger   <mathias.ettinger@viveris.fr>
 */


#include <algorithm>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <experimental/filesystem>

#include "OutputHandler.h"
#include "BaseProbe.h"


HandlerCreationFailedError::HandlerCreationFailedError(const std::string& what_arg) : std::runtime_error(what_arg)
{
};


inline unsigned long long getTimestamp() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}


struct getDate {
  getDate() {
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(ms);
    date_time = s.count();
    date_milli = ms.count() % 1000;
  }

  std::time_t date_time;
  std::size_t date_milli;
};


std::ostream& operator<<(std::ostream& os, const getDate& date) {
  const char prevFill = os.fill();
  const std::streamsize prevWidth = os.width();
  os << std::put_time(std::localtime(&date.date_time), "%F %T.") << std::setfill('0') << std::setw(3) << date.date_milli << std::setfill(prevFill) << std::setw(prevWidth);
  return os;
}


FileStatHandler::FileStatHandler(const std::string& fileName, const std::string& originFolder) : filesOpened(0), folder(originFolder), filename(fileName) {
  std::experimental::filesystem::create_directories(folder);
  file.open(buildFullPath());
}


FileStatHandler::~FileStatHandler() {
  file.flush();
  file.close();
}


std::string FileStatHandler::buildFullPath() const {
  std::stringstream filenameBuilder;
  filenameBuilder << folder << '/' << filename << '_' << filesOpened << ".csv";
  return filenameBuilder.str();
}


void FileStatHandler::emitStats(const std::vector<std::pair<std::string, std::string>>& probesValues)
{
  file << getDate();
  for (auto& probe : probesValues) {
    file << ";" << probe.second;
  }
  file << "\n";
  file.flush();
}


void FileStatHandler::configure(const std::vector<std::shared_ptr<BaseProbe>>& probes)
{
  if (filesOpened++) {
    file.flush();
    file.close();
    file.open(buildFullPath());
  }

  file << "Date";
  for (auto& probe : probes) {
    file << ";" << probe->getName() << " (" << probe->getUnit() << ")";
  }
  file << "\n";
  file.flush();
}


LogHandler::LogHandler(const std::string& entityName) : entityName(entityName) {
}


void LogHandler::prepareMessage(std::ostream& formatter, const std::string& logName, const std::string& level, const std::string& message) {
  formatter << "[" << getDate() << "][" << level << "][" << entityName << "][" << logName << "]";
  if (message[message.size() - 1] != '\n')
  {
    formatter << message;
  }
  else
  {
    formatter << message.substr(0, message.size() -1);
  }
}


FileLogHandler::FileLogHandler(const std::string& fileName, const std::string& originFolder) : LogHandler(fileName) {
  std::experimental::filesystem::create_directories(originFolder);

  file.open(originFolder + '/' + fileName + ".log");
}


FileLogHandler::~FileLogHandler() {
  file.flush();
  file.close();
}


void FileLogHandler::emitLog(const std::string& logName, const std::string& level, const std::string& message) {
  std::lock_guard<std::mutex> acquire{lock};
  prepareMessage(file, logName, level, message);
  file << std::endl;
}


SocketStatHandler::SocketStatHandler(const std::string& address, unsigned short port, bool useTCP) : useTcp(useTCP) {
  remote.sin_family = AF_INET;
  remote.sin_port = htons(port);
  if (inet_pton(AF_INET, address.c_str(), &remote.sin_addr) < 0) {
    std::stringstream message;
    message << "Cannot set " << address << " as socket remote host.";
    throw HandlerCreationFailedError(message.str());
  }

  if ((socketFd = socket(AF_INET, useTCP ? SOCK_STREAM : SOCK_DGRAM, 0)) < 0) {
    throw HandlerCreationFailedError("Cannot open socket.");
  }

  if (useTCP) {
    if (connect(socketFd, (struct sockaddr*)(&remote), sizeof(remote)) < 0) {
      close(socketFd);
      throw HandlerCreationFailedError("Cannot connect socket to remote host.");
    }
  }
}


SocketStatHandler::~SocketStatHandler() {
  close(socketFd);
}


void SocketStatHandler::emitStats(const std::vector<std::pair<std::string, std::string>>& probesValues)
{
  bool needSending = false;
  std::stringstream formatter;
  formatter << getTimestamp();

  for (auto& probe : probesValues) {
    if (probe.second.empty()) {
      continue;
    }
    needSending = true;
    formatter << " " << probe.first << " " << probe.second;
  }

  if (!needSending) {
    return;
  }

  std::string msg = formatter.str();

  if (useTcp) {
    send(socketFd, msg.c_str(), msg.length(), 0);
  } else {
    sendto(socketFd, msg.c_str(), msg.length(), 0, (struct sockaddr*)(&remote), sizeof(remote));
  }
}


void SocketStatHandler::configure(const std::vector<std::shared_ptr<BaseProbe>>&)
{
  // Do nothing
}


SocketLogHandler::SocketLogHandler(const std::string& entityName, const std::string& address, unsigned short port, bool useTCP) : LogHandler(entityName), useTcp(useTCP) {
  remote.sin_family = AF_INET;
  remote.sin_port = htons(port);
  if (inet_pton(AF_INET, address.c_str(), &remote.sin_addr) < 0) {
    std::stringstream message;
    message << "Cannot set " << address << " as socket remote host.";
    throw HandlerCreationFailedError(message.str());
  }

  if ((socketFd = socket(AF_INET, useTCP ? SOCK_STREAM : SOCK_DGRAM, 0)) < 0) {
    throw HandlerCreationFailedError("Cannot open socket.");
  }

  if (useTCP) {
    if (connect(socketFd, (struct sockaddr*)(&remote), sizeof(remote)) < 0) {
      close(socketFd);
      throw HandlerCreationFailedError("Cannot connect socket to remote host.");
    }
  }
}


SocketLogHandler::~SocketLogHandler() {
  close(socketFd);
}


void SocketLogHandler::emitLog(const std::string& logName, const std::string& level, const std::string& message) {
  std::stringstream formatter;
  prepareMessage(formatter, logName, level, message);
  std::string msg = formatter.str();

  if (useTcp) {
    std::lock_guard<std::mutex> acquire{lock};
    send(socketFd, msg.c_str(), msg.length(), 0);
  } else {
    std::lock_guard<std::mutex> acquire{lock};
    sendto(socketFd, msg.c_str(), msg.length(), 0, (struct sockaddr*)(&remote), sizeof(remote));
  }
}
