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
 * @file Output.cpp
 * @brief Methods of the Output static class, used by the application to
 *				interact with the output.
 * @author Vincent Duvert			<vduvert@toulouse.viveris.com>
 * @author Fabrice Hobaya			<fhobaya@toulouse.viveris.com>
 * @author Alban FRICOT				<africot@toulouse.viveris.com>
 * @author Mathias Ettinger		<mathias.ettinger@viveris.fr>
 */


#include <stdexcept>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <optional>

#include "Output.h"
#include "OutputEvent.h"
#include "OutputHandler.h"


void printIndent(std::ostream& os, uint32_t indent) {
	for (uint32_t i = 0; i < indent; ++i) {
		os << "│   ";
	}
}


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
	virtual void print(std::ostream& os, uint32_t indent) const = 0;

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
	void print(std::ostream& os, uint32_t indent) const {
		for (auto& child : children) {
			printIndent(os, indent);
			os << "├───┬ " << child.first << '\n';
			child.second->print(os, indent + 1);
		}
		printIndent(os, indent);
		os << "╵\n";
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
	OutputUnit(const std::string& name, const std::string& full_name): OutputItem(name, full_name), log(nullptr) {}

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

	void print(std::ostream& os, uint32_t indent) const {
		if (this->log != nullptr) {
			printIndent(os, indent);
			os << "├── [LOG] [" << log->getDisplayLevelString() << "] " << log->getName() << '\n';
		}
		if (!this->stats.empty()) {
			for (auto& stat : stats) {
				auto probe = stat.second;
				if (probe->isEnabled())
				{
					printIndent(os, indent);
					os << "├── [PROBE] " << probe->getName();

					auto unit = probe->getUnit();
					if (!unit.empty())
					{
						os << " (" << unit << ")";
					}
					os << '\n';
				}
			}
		}
		printIndent(os, indent);
		os << "╵\n";
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
			stat.second->reset();
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
	std::shared_ptr<BaseProbe> getBaseStat(const std::string& name) const {
		auto stat = stats.find(name);
		if (stat == stats.end()) {
			return nullptr;
		}
		return stat->second;
	}
	std::shared_ptr<OutputLog> getLog() const { return log; }

 private:
	std::shared_ptr<OutputLog> log;
	std::unordered_map<std::string, std::shared_ptr<BaseProbe>> stats;
};


class OutputDesiredLogLevel
{
 public:
	 OutputDesiredLogLevel() : desired_level{} {};

	 std::shared_ptr<OutputDesiredLogLevel> getOrCreateChild(const std::string& name) {
		 auto child = children.find(name);
		 if (child == children.end()) {
			 auto new_child = std::make_shared<OutputDesiredLogLevel>();
			 child = children.emplace(name, new_child).first;
		 }
		 return child->second;
	 }

	 log_level_t definitiveLogLevel(log_level_t level) const {
		 return desired_level.value_or(level);
	 }

	 void setLogLevel(log_level_t level) {
		 desired_level = level;
	 }

 private:
	std::optional<log_level_t> desired_level;
	std::unordered_map<std::string, std::shared_ptr<OutputDesiredLogLevel>> children;
};


inline std::string normalizeName(const std::string& name) {
	std::string copy{name};
	std::transform(name.begin(), name.end(), copy.begin(),
	               [](unsigned char c){ return std::isspace(c) ? '_' : std::tolower(c); });
	return copy;
}


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
	desiredLogLevels = std::make_shared<OutputDesiredLogLevel>();
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
	std::string name = normalizeName(identifier);

	if (privateLog != nullptr) {
		privateLog->sendLog(LEVEL_INFO, "Registering event '%s'", name.c_str());
	}

	OutputLock acquire{lock};
	std::vector<std::string> parts = splitName(name);

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

std::shared_ptr<OutputLog> Output::registerLog(log_level_t display_level, const std::string& identifier)
{
	std::string name = normalizeName(identifier);

	if (privateLog != nullptr) {
		privateLog->sendLog(LEVEL_INFO, "Registering log '%s'", name.c_str());
	}

	OutputLock acquire{lock};
	std::vector<std::string> parts = splitName(name);

	std::shared_ptr<OutputDesiredLogLevel> desiredLogLevel = desiredLogLevels;
	log_level_t log_level = desiredLogLevel->definitiveLogLevel(display_level);
	for (auto&& part : parts) {
		desiredLogLevel = desiredLogLevel->getOrCreateChild(part);
		log_level = desiredLogLevel->definitiveLogLevel(log_level);
		desiredLogLevel->setLogLevel(log_level);
	}

	std::string unitName = parts.back();
	parts.pop_back();

	try {
		std::shared_ptr<OutputUnit> logUnit = getOrCreateSection(parts)->findUnit(unitName);
		std::shared_ptr<OutputLog> existingLog = logUnit->getLog();
		if (existingLog != nullptr) {
				existingLog->setDisplayLevel(log_level);
				return existingLog;
		}
		std::shared_ptr<OutputLog> log{new OutputLog(log_level, logUnit->getFullName())};
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


std::string Output::getEntityName() const
{
	if (this->entityName.empty()) {
		return "opensand";
	}
	return entityName;
}


bool Output::configureLocalOutput(const std::string& folder)
{
	std::string entityName = getEntityName();

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
	std::string entityName = getEntityName();

	std::shared_ptr<SocketLogHandler> logHandler;
	std::shared_ptr<SocketStatHandler> statHandler;

	try {
		logHandler = std::make_shared<SocketLogHandler>(entityName, address, logsPort);
		statHandler = std::make_shared<SocketStatHandler>(entityName, address, statsPort);
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


bool Output::configureTerminalOutput()
{
	std::string entityName = getEntityName();

	std::shared_ptr<StreamLogHandler> logHandler;
	try {
		logHandler = std::make_shared<StreamLogHandler>(entityName);
	} catch (const HandlerCreationFailedError& exc) {
		logException(privateLog, exc);
		return false;
	}

	logHandlers.push_back(logHandler);
	privateLog->addHandler(logHandler);
	defaultLog->addHandler(logHandler);
	return true;
}


void Output::finalizeConfiguration(void)
{
	OutputLock acquire{lock};

	enabledProbes.clear();
	root->gatherEnabledStats(enabledProbes);

	for (auto& handler : probeHandlers) {
		handler->configure(enabledProbes);
	}
}


void Output::sendProbes(void)
{
	OutputLock acquire{lock};

	std::vector<std::pair<std::string, std::string>> probesValues;
	for (auto& probe : enabledProbes) {
		probesValues.emplace_back(probe->getName(), probe->getData());
	}

	for (auto& handler : probeHandlers) {
		handler->emitStats(probesValues);
	}
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
std::shared_ptr<Probe<int32_t>> Output::registerProbe(const std::string& identifier, const std::string& unit, bool enabled, sample_type_t type)
{
	std::string name = normalizeName(identifier);

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
std::shared_ptr<Probe<float>> Output::registerProbe(const std::string& identifier, const std::string& unit, bool enabled, sample_type_t type)
{
	std::string name = normalizeName(identifier);

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
std::shared_ptr<Probe<double>> Output::registerProbe(const std::string& identifier, const std::string& unit, bool enabled, sample_type_t type)
{
	std::string name = normalizeName(identifier);

	std::shared_ptr<Probe<double>> probe{new Probe<double>(name, unit, enabled, type)};
	try {
		registerProbe(name, probe);
	} catch (const AlreadyExistsError& exc) {
		logException(privateLog, exc);
		return nullptr;
	}
	return probe;
}


void Output::setProbeState(const std::string& path, bool enabled) {
	std::vector<std::string> parts = splitName(normalizeName(path));
	if (parts.empty()) {
		root->enableStats(enabled);
	} else {
		std::string possibleStatName = parts.back();
		parts.pop_back();

		std::shared_ptr<OutputItem> currentItem = root;
		for (auto& name : parts) {
			std::shared_ptr<OutputSection> currentSection = std::dynamic_pointer_cast<OutputSection>(currentItem);
			if (currentSection == nullptr) { goto probe_not_found; }
			currentItem = currentSection->find(name);
		}

		std::shared_ptr<OutputSection> lastSection;
		std::shared_ptr<OutputUnit> lastUnit;
		if ((lastSection = std::dynamic_pointer_cast<OutputSection>(currentItem)) != nullptr) {
			currentItem = lastSection->find(possibleStatName);
			if (currentItem == nullptr) { goto probe_not_found; }
			currentItem->enableStats(enabled);
		} else if ((lastUnit = std::dynamic_pointer_cast<OutputUnit>(currentItem)) != nullptr) {
			std::shared_ptr<BaseProbe> probe = lastUnit->getBaseStat(possibleStatName);
			if (probe == nullptr) { goto probe_not_found; }
			probe->enable(enabled);
		} else {
			goto probe_not_found;
		}
	}

	return;

probe_not_found:
	if (privateLog != nullptr) {
		privateLog->sendLog(LEVEL_WARNING, "Cannot change probes states: %s is not a valid group or probe name.", path.c_str());
	}
}


void Output::setLogLevel(const std::string& path, log_level_t level) {
	std::vector<std::string> parts = splitName(normalizeName(path));

	std::shared_ptr<OutputItem> currentItem = root;
	for (auto& name : parts) {
		std::shared_ptr<OutputSection> currentSection = std::dynamic_pointer_cast<OutputSection>(currentItem);
		if (currentSection == nullptr) { goto log_not_found; }
		currentItem = currentSection->find(name);
	}

	if (currentItem == nullptr) { goto log_not_found; }
	currentItem->setLogLevel(level);
	return;

log_not_found:
	if (privateLog != nullptr) {
		privateLog->sendLog(LEVEL_WARNING, "Cannot change logs levels: %s is not a valid group or log name.", path.c_str());
	}
}


void Output::setLevels(const std::map<std::string, log_level_t> &levels)
{
	for (auto&& log_level : levels) {
		auto log_name = normalizeName(log_level.first);
		std::vector<std::string> parts = splitName(log_name);

		std::shared_ptr<OutputDesiredLogLevel> desiredLevel = desiredLogLevels;
		std::shared_ptr<OutputItem> currentItem = root;
		for (auto&& name : parts) {
			std::shared_ptr<OutputSection> currentSection = std::dynamic_pointer_cast<OutputSection>(currentItem);
			if (currentSection == nullptr) {
				currentItem = nullptr;
			} else {
				currentItem = currentSection->find(name);
			}
			desiredLevel = desiredLevel->getOrCreateChild(name);
		}

		desiredLevel->setLogLevel(log_level.second);
		if (currentItem != nullptr) {
			currentItem->setLogLevel(log_level.second);
		}
	}
}


std::ostream& operator << (std::ostream& os, const Output& o) {
	os << "liboutput configuration:\n";
	o.root->print(os, 0);
	return os;
}
