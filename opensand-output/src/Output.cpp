/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file Output.cpp
 * @brief Methods of the Output static class, used by the application to
 *        interact with the output.
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 * @author Fabrice Hobaya <fhobaya@toulouse.viveris.com>
 * @author Alban FRICOT <africot@toulouse.viveris.com>
 */


#include <stdexcept>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include "Output.h"
#include "OutputEvent.h"
#include "OutputHandler.h"


class AlreadyExistsError : public std::runtime_error {
  public:
    explicit AlreadyExistsError(const std::string& what_arg) : std::runtime_error(what_arg) {}
};


class OutputUnit;


class OutputItem
{
  public:
    OutputItem(const std::string& name, const std::string& full_name) : name(name), full_name(full_name) {}
    virtual ~OutputItem() {}

    std::string getFullName() const { return full_name; }

    virtual void setLogLevel(log_level_t level) = 0;
    virtual void enableStats(bool enabled) = 0;
    virtual void gatherEnabledStats(std::vector<std::shared_ptr<BaseProbe>>& probes) const = 0;

  protected:
    std::string buildChildFullName(const std::string& name) const {
      std::string newFullName = full_name + "." + name;
      if (this->name.empty()) {
        newFullName = name;
      }
      return newFullName;
    }

    std::string name;
    std::string full_name;
};


class Output::OutputSection : public OutputItem
{
  public:
    OutputSection(const std::string& name, const std::string& full_name) : OutputItem(name, full_name) {}
    ~OutputSection() { children.clear(); }

    void setLogLevel(log_level_t level) {
      for (auto& child : children) {
        child.second->setLogLevel(level);
      }
    }
    void enableStats(bool enabled) {
      for (auto& child : children) {
        child.second->enableStats(enabled);
      }
    }

    void gatherEnabledStats(std::vector<std::shared_ptr<BaseProbe>>& probes) const {
      for (auto& child : children) {
        child.second->gatherEnabledStats(probes);
      }
    }

    std::shared_ptr<OutputSection> findSection(const std::string& name) {
      auto child = children.find(name);
      std::shared_ptr<OutputSection> unit;

      if (child == children.end()) {
        unit = std::make_shared<OutputSection>(name, buildChildFullName(name));
        children.emplace(name, unit);
      } else {
        unit = std::dynamic_pointer_cast<OutputSection>(child->second);
        if (unit == nullptr) {
          std::stringstream message;
          message << "Searching for section " << buildChildFullName(name) << " but found Unit instead!";
          throw AlreadyExistsError(message.str());
        }
      }
      return unit;
    }
    std::shared_ptr<OutputUnit> findUnit(const std::string& name) {
      auto child = children.find(name);
      std::shared_ptr<OutputUnit> unit;

      if (child == children.end()) {
        unit = std::make_shared<OutputUnit>(name, buildChildFullName(name));
        children.emplace(name, unit);
      } else {
        unit = std::dynamic_pointer_cast<OutputUnit>(child->second);
        if (unit == nullptr) {
          std::stringstream message;
          message << "Searching for unit " << buildChildFullName(name) << " but found Section instead!";
          throw AlreadyExistsError(message.str());
        }
      }
      return unit;
    }
    std::shared_ptr<OutputItem> find(const std::string& name) const {
      auto child = children.find(name);
      if (child == children.end()) {
        return nullptr;
      }
      return child->second;
    }

  private:
    std::unordered_map<std::string, std::shared_ptr<OutputItem>> children;
};


class OutputUnit : public OutputItem
{
  public:
    OutputUnit(const std::string& name, const std::string& full_name) : OutputItem(name, full_name), log(nullptr) {}

    ~OutputUnit() {
      log = nullptr;
      stats.clear();
    }

    void setLog(std::shared_ptr<OutputLog> log) {
      if (this->log != nullptr) {
        std::stringstream message;
        message << "Log " << full_name << " already created!";
        throw AlreadyExistsError(message.str());
      }
      this->log = log;
    }

    void setLogLevel(log_level_t level) {
      if (log != nullptr) {
        log->setDisplayLevel(level);
      }
    }
    void enableStats(bool enabled) {
      for (auto& stat : stats) {
        stat.second->enable(enabled);
      }
    }

    void gatherEnabledStats(std::vector<std::shared_ptr<BaseProbe>>& probes) const {
      for (auto& stat : stats) {
        if (stat.second->isEnabled()) {
          probes.push_back(stat.second);
        }
      }
    }

    void setStat(const std::string& name, std::shared_ptr<BaseProbe> probe) {
      auto child = stats.find(name);
      if (child != stats.end()) {
        std::stringstream message;
        message << "Stat " << name << " already exists in unit " << full_name;
        throw AlreadyExistsError(message.str());
      }

      stats.emplace(name, probe);
    }
    template<typename T>
    std::shared_ptr<Probe<T>> getStat(const std::string& name) const {
      auto stat = stats.find(name);
      if (stat == stats.end()) {
        return nullptr;
      }
      return std::dynamic_pointer_cast<Probe<T>>(stat->second);
    }
    std::shared_ptr<OutputLog> getLog() const { return log; }

  private:
    std::shared_ptr<OutputLog> log;
    std::unordered_map<std::string, std::shared_ptr<BaseProbe>> stats;
};


inline std::vector<std::string> splitName(const std::string& name) {
  std::istringstream line(name);
  std::string token;
  std::vector<std::string> parts;

  while (std::getline(line, token, '.')) {
    parts.push_back(token);
  }
  return parts;
}


inline void logException(std::shared_ptr<OutputLog>& log, const std::exception& exc) {
  if (log != nullptr) {
    log->sendLog(LEVEL_ERROR, "%s", exc.what());
  }
  std::cerr << exc.what() << std::endl;
}


Output::Output()
{
  root = std::make_shared<OutputSection>("", "");
  privateLog = registerLog(LEVEL_WARNING, "output");
  defaultLog = registerLog(LEVEL_WARNING, "default");
}


Output::~Output()
{
  privateLog = nullptr;
  defaultLog = nullptr;
  root = nullptr;
  enabledProbes.clear();
  logHandlers.clear();
  probeHandlers.clear();
}


std::shared_ptr<Output> Output::Get()
{
  static std::shared_ptr<Output> instance{new Output()};
  return instance;
}


std::shared_ptr<OutputEvent> Output::registerEvent(const std::string& identifier)
{
  if (privateLog != nullptr) {
    privateLog->sendLog(LEVEL_INFO, "Registering event '%s'", identifier.c_str());
  }

  OutputLock acquire{lock};
  std::vector<std::string> parts = splitName(identifier);

  std::string unitName = parts.back();
  parts.pop_back();

  try {
    std::shared_ptr<OutputUnit> logUnit = getOrCreateSection(parts)->findUnit(unitName);
    std::shared_ptr<OutputEvent> existingEvent = std::dynamic_pointer_cast<OutputEvent>(logUnit->getLog());
    if (existingEvent != nullptr) {
        return existingEvent;
    }
    std::shared_ptr<OutputEvent> event{new OutputEvent(logUnit->getFullName())};
    logUnit->setLog(event);

    for (auto& handler : logHandlers) {
      event->addHandler(handler);
    }

    return event;
  } catch (const AlreadyExistsError& exc) {
    logException(privateLog, exc);
    return nullptr;
  }
}

std::shared_ptr<OutputLog> Output::registerLog(log_level_t display_level, const std::string& name)
{
  if (privateLog != nullptr) {
    privateLog->sendLog(LEVEL_INFO, "Registering log '%s'", name.c_str());
  }

  OutputLock acquire{lock};
  std::vector<std::string> parts = splitName(name);

  std::string unitName = parts.back();
  parts.pop_back();

  try {
    std::shared_ptr<OutputUnit> logUnit = getOrCreateSection(parts)->findUnit(unitName);
    std::shared_ptr<OutputLog> existingLog = logUnit->getLog();
    if (existingLog != nullptr) {
        existingLog->setDisplayLevel(display_level);
        return existingLog;
    }
    std::shared_ptr<OutputLog> log{new OutputLog(display_level, logUnit->getFullName())};
    logUnit->setLog(log);

    for (auto& handler : logHandlers) {
      log->addHandler(handler);
    }

    return log;
  } catch (const AlreadyExistsError& exc) {
    logException(privateLog, exc);
    return nullptr;
  }
}


std::shared_ptr<OutputEvent> Output::registerEvent(const char *identifier, ...)
{
  std::va_list args;
  va_start(args, identifier);
  std::string eventName = formatMessage(identifier, args);
  va_end(args);

  return Output::registerEvent(eventName);
}


std::shared_ptr<OutputLog> Output::registerLog(log_level_t default_display_level, const char* name, ...)
{
  std::va_list args;
  va_start(args, name);
  std::string logName = formatMessage(name, args);
  va_end(args);

  return Output::registerLog(default_display_level, logName);
}


bool Output::configureLocalOutput(const std::string& folder, const std::string& entityName)
{
  std::shared_ptr<FileLogHandler> logHandler;
  std::shared_ptr<FileStatHandler> statHandler;

  try {
    logHandler = std::make_shared<FileLogHandler>(entityName, folder);
    statHandler = std::make_shared<FileStatHandler>(entityName, folder);
  } catch (const HandlerCreationFailedError& exc) {
    logException(privateLog, exc);
    return false;
  }

  logHandlers.push_back(logHandler);
  privateLog->addHandler(logHandler);
  defaultLog->addHandler(logHandler);

  probeHandlers.push_back(statHandler);

  return true;
}


bool Output::configureRemoteOutput(const std::string& address,
                                   unsigned short statsPort,
                                   unsigned short logsPort)
{
  std::shared_ptr<SocketLogHandler> logHandler;
  std::shared_ptr<SocketStatHandler> statHandler;

  try {
    logHandler = std::make_shared<SocketLogHandler>(address, logsPort);
    statHandler = std::make_shared<SocketStatHandler>(address, statsPort);
  } catch (const HandlerCreationFailedError& exc) {
    logException(privateLog, exc);
    return false;
  }

  logHandlers.push_back(logHandler);
  privateLog->addHandler(logHandler);
  defaultLog->addHandler(logHandler);

  probeHandlers.push_back(statHandler);

  return true;
}


void Output::finalizeConfiguration(void)
{
  OutputLock acquire{lock};

  enabledProbes.clear();
  root->gatherEnabledStats(enabledProbes);

  for (auto& probe : enabledProbes) {
    // TODO: better than that
    probe->reset();
  }

  for (auto& handler : probeHandlers) {
    handler->configure(enabledProbes);
  }
}


void Output::sendProbes(void)
{
  OutputLock acquire{lock};

  std::vector<std::pair<std::string, std::string>> probesValues;
  for (auto& probe : enabledProbes) {
    const std::string name = probe->getName();
    const std::string value = probe->isEmpty() ? "" : probe->getData();
    probe->reset();
    probesValues.emplace_back(name, value);
  }

  for (auto& handler : probeHandlers) {
    handler->emitStats(probesValues);
  }
}


void Output::sendLog(log_level_t log_level, const char *msg_format, ...)
{
  std::va_list args;
  va_start(args, msg_format);
  defaultLog->vSendLog(log_level, msg_format, args);
  va_end(args);
}


void Output::setDisplayLevel(log_level_t log_level)
{
  defaultLog->setDisplayLevel(log_level);
}


std::shared_ptr<Output::OutputSection> Output::getOrCreateSection(const std::vector<std::string>& sectionNames)
{
  std::shared_ptr<OutputSection> currentSection = root;

  for (auto& name : sectionNames) {
    currentSection = currentSection->findSection(name);
  }

  return currentSection;
}


void Output::registerProbe(const std::string& name, std::shared_ptr<BaseProbe> probe)
{
  if (privateLog != nullptr) {
    privateLog->sendLog(LEVEL_INFO, "Registering probe '%s'", name.c_str());
  }

  std::vector<std::string> parts = splitName(name);

  std::string statName = parts.back();
  parts.pop_back();

  std::string unitName = parts.back();
  parts.pop_back();

  getOrCreateSection(parts)->findUnit(unitName)->setStat(statName, probe);
}


template<>
std::shared_ptr<Probe<int32_t>> Output::registerProbe(const std::string& name, const std::string& unit, bool enabled, sample_type_t type)
{
  std::shared_ptr<Probe<int32_t>> probe{new Probe<int32_t>(name, unit, enabled, type)};
  try {
    registerProbe(name, probe);
  } catch (const AlreadyExistsError& exc) {
    logException(privateLog, exc);
    return nullptr;
  }
  return probe;
}


template<>
std::shared_ptr<Probe<float>> Output::registerProbe(const std::string& name, const std::string& unit, bool enabled, sample_type_t type)
{
  std::shared_ptr<Probe<float>> probe{new Probe<float>(name, unit, enabled, type)};
  try {
    registerProbe(name, probe);
  } catch (const AlreadyExistsError& exc) {
    logException(privateLog, exc);
    return nullptr;
  }
  return probe;
}


template<>
std::shared_ptr<Probe<double>> Output::registerProbe(const std::string& name, const std::string& unit, bool enabled, sample_type_t type)
{
  std::shared_ptr<Probe<double>> probe{new Probe<double>(name, unit, enabled, type)};
  try {
    registerProbe(name, probe);
  } catch (const AlreadyExistsError& exc) {
    logException(privateLog, exc);
    return nullptr;
  }
  return probe;
}


//void Output::setLevels(const map<string, log_level_t> &levels,
                       //const map<string, log_level_t> &specific)
//{
	//instance->setLevels(levels, specific);
//}
