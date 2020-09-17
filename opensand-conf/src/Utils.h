/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @file Utils.h
 * @brief Functions to help Model (un)serialization.
 */

#ifndef CONFIGURATION_UTILS_HPP
#define CONFIGURATION_UTILS_HPP

#define LIBXML_SCHEMAS_ENABLED
#include <libxml/xmlschemastypes.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <any>
#include <iostream>
#include <memory>
#include <iomanip>
#include <sstream>

/** unused macro to avoid compilation warning with unused parameters. */
#ifdef __GNUC__
#  define UNUSED(x) x __attribute__((unused))
#elif __LCLINT__
#  define UNUSED(x) /*@unused@*/ x
#else               /* !__GNUC__ && !__LCLINT__ */
#  define UNUSED(x) x
#endif              /* !__GNUC__ && !__LCLINT__ */

#define CONFIGURATION_FILES_VERSION "1.0"
#define CONFIGURATION_FILES_ENCODING "UTF-8"

#define CONFIGURATION_FLOAT_PRECISION 10
#define CONFIGURATION_DOUBLE_PRECISION 20
#define CONFIGURATION_LONG_DOUBLE_PRECISION 30

namespace Utils {
    static bool toXSD(const Model * model, const std::string& filename);
    static bool fromXSD(Environment ** environment, Model ** model, const std::string& filename);

    static bool toXML(const Model * model, const std::string& filename);
    static bool fromXML(Environment ** environment, Model ** model, const std::string& filename, const std::string& xsdFilename);

    static bool validate(const std::string& xsdPath, const std::string& xmlPath);
}

namespace {
    static std::string toString(std::shared_ptr<Parameter> parameter);
    static void fromString(std::shared_ptr<Parameter> parameter, const std::string& value);
    static std::string defaultToString(std::shared_ptr<Parameter> parameter);
    static void defaultFromString(std::shared_ptr<Parameter> parameter, const std::string& value);
}

namespace {
    struct Common {
        static std::string xmlGetContent(xmlNodePtr node) {
            char * content = (char *) xmlNodeGetContent(node);
            std::string contentString = content;

            xmlFree(content);

            return contentString;
        }

        static std::string xmlGetPropValue(xmlNodePtr node, const std::string& propertyName) {
            char * description = (char *) xmlGetProp(node, BAD_CAST propertyName.c_str());
            std::string descriptionString = description;

            xmlFree(description);

            return descriptionString;
        }

        static std::string xmlGetMetatype(xmlNodePtr node) {
            // <xs:annotation>
            xmlNodePtr current = node->children->next;
            // <xs:complexType>
            current = current->next->next;
            // <xs:...>
            current = current->children->next;

            std::string nodeName = (char *) current->name;

            if(nodeName == "simpleContent") {
                return "P";
            } else if(nodeName == "sequence") {
                return "C";
            } else if(nodeName == "choice") {
                return "L";
            } else {
                return "";
            }
        }

        static std::string xsdGetId(xmlNodePtr node) {
            // <xs:annotation>
            xmlNodePtr current = node->children->next;
            // <xs:documentation>
            current = current->children->next;
            // <id>
            current = current->children->next->next->next->next->next->next->next;

            return xmlGetContent(current);
        }

        static std::string xsdGetVersion(xmlNodePtr node) {
            // <xs:annotation>
            xmlNodePtr current = node->children->next;
            // <xs:documentation>
            current = current->children->next;
            // <version>
            current = current->children->next->next->next->next->next;

            return xmlGetContent(current);
        }

        static std::string xsdGetName(xmlNodePtr node) {
            // <xs:annotation>
            xmlNodePtr current = node->children->next;
            // <xs:documentation>
            current = current->children->next;
            // <description>
            current = current->children->next;
            // <name>
            current = current->next->next;

            return xmlGetContent(current);
        }

        static std::string xsdGetType(xmlNodePtr node) {
            // <xs:annotation>
            xmlNodePtr current = node->children->next;
            // <xs:documentation>
            current = current->children->next;
            // <description>
            current = current->children->next;
            // <name>
            current = current->next->next;
            // <refPath>
            current = current->next->next;
            // <refValue>
            current = current->next->next;
            // <visibility>
            current = current->next->next;
            // <unit>
            current = current->next->next;
            // <type>
            current = current->next->next;

            return xmlGetContent(current);
        }

        static std::string xsdGetDescription(xmlNodePtr node) {
            // <xs:annotation>
            xmlNodePtr current = node->children->next;
            // <xs:documentation>
            current = current->children->next;
            // <description>
            current = current->children->next;

            return xmlGetContent(current);
        }

        static std::string xsdGetRefPath(xmlNodePtr node) {
            // <xs:annotation>
            xmlNodePtr current = node->children->next;
            // <xs:documentation>
            current = current->children->next;
            // <description>
            current = current->children->next;
            // <name>
            current = current->next->next;
            // <refPath>
            current = current->next->next;

            return xmlGetContent(current);
        }

        static std::string xsdGetRefValue(xmlNodePtr node) {
            // <xs:annotation>
            xmlNodePtr current = node->children->next;
            // <xs:documentation>
            current = current->children->next;
            // <description>
            current = current->children->next;
            // <name>
            current = current->next->next;
            // <refPath>
            current = current->next->next;
            // <refValue>
            current = current->next->next;

            return xmlGetContent(current);
        }

        static std::string xsdGetVisibility(xmlNodePtr node) {
            // <xs:annotation>
            xmlNodePtr current = node->children->next;
            // <xs:documentation>
            current = current->children->next;
            // <description>
            current = current->children->next;
            // <name>
            current = current->next->next;
            // <refPath>
            current = current->next->next;
            // <refValue>
            current = current->next->next;
            // <visibility>
            current = current->next->next;

            return xmlGetContent(current);
        }

        static std::string xsdGetUnit(xmlNodePtr node) {
            // <xs:annotation>
            xmlNodePtr current = node->children->next;
            // <xs:documentation>
            current = current->children->next;
            // <description>
            current = current->children->next;
            // <name>
            current = current->next->next;
            // <refPath>
            current = current->next->next;
            // <refValue>
            current = current->next->next;
            // <visibility>
            current = current->next->next;
            // <unit>
            current = current->next->next;

            return xmlGetContent(current);
        }

        static std::string xsdGetDefault(xmlNodePtr node) {
            // <xs:annotation>
            xmlNodePtr current = node->children->next;
            // <xs:documentation>
            current = current->children->next;
            // <description>
            current = current->children->next;
            // <name>
            current = current->next->next;
            // <refPath>
            current = current->next->next;
            // <refValue>
            current = current->next->next;
            // <visibility>
            current = current->next->next;
            // <unit>
            current = current->next->next;
            // <type>
            current = current->next->next;
            // <default>
            current = current->next->next;

            return xmlGetContent(current);
        }

        /**
         * @brief Returns a TypeID from its serialized value.
         * @param typeName The TypeID string value.
         * @return The corresponding TypeID value. If no value found, returns TypeID::COUNT.
         */
        static TypeID getTypeIDFromString(const std::string& typeName) {
            TypeID typeId = COUNT;

            if(typeName == "boolean") {
                typeId = BOOLEAN;
            } else if(typeName == "byte") {
                typeId = BYTE;
            } else if(typeName == "char") {
                typeId = CHAR;
            } else if(typeName == "double") {
                typeId = DOUBLE;
            } else if(typeName == "float") {
                typeId = FLOAT;
            } else if(typeName == "int") {
                typeId = INT;
            } else if(typeName == "long") {
                typeId = LONG;
            } else if(typeName == "longdouble") {
                typeId = LONGDOUBLE;
            } else if(typeName == "short") {
                typeId = SHORT;
            } else if(typeName == "string") {
                typeId = STRING;
            }

            return typeId;
        }
    };

    struct XML {
        static xmlNodePtr toXML(const Model * model) {
            xmlNodePtr rootNode = xmlNewNode(nullptr, BAD_CAST "Configuration");

            xmlNewProp(rootNode, BAD_CAST "version", BAD_CAST model->getVersion().c_str());
            xmlNewProp(rootNode, BAD_CAST "id", BAD_CAST model->getId().c_str());

            xmlNodePtr modelNode = xmlNewNode(nullptr, BAD_CAST "DataModel");
            xmlAddChild(rootNode, modelNode);

            xmlNodePtr modelRootNode = toXML(model->getRootComponent(), false);
            xmlAddChild(modelNode, modelRootNode);

            return rootNode;
        }

        static xmlNodePtr listToXML(const std::shared_ptr<List>& list) {
            xmlNodePtr listNode = toXML(list, "L", false, "");

            for(const auto& component : list->getComponentList()) {
                std::string nodeName = list->getId() + "_item";
                xmlNodePtr componentNode = toXML(component, true, nodeName);
                xmlAddChild(listNode, componentNode);
            }

            return listNode;
        }

        static xmlNodePtr toXML(const std::shared_ptr<Component>& component, bool isListItem, const std::string nodeName = "") {
            xmlNodePtr componentNode = toXML(component, "C", isListItem, nodeName);

            for (const auto& parameter : component->getParameterList()) {
                if (parameter != nullptr) {
                    xmlNodePtr parameterNode = toXML(parameter);
                    xmlAddChild(componentNode, parameterNode);
                }
            }

            for (const auto& child : component->getComponentList()) {
                if (child != nullptr) {
                    xmlNodePtr childNode = toXML(child, false);
                    xmlAddChild(componentNode, childNode);
                }
            }

            for (const std::shared_ptr<List>& list : component->getListList()) {
                if (list != nullptr) {
                    xmlNodePtr listNode = listToXML(list);
                    xmlAddChild(componentNode, listNode);
                }
            }

            return componentNode;
        }

        static xmlNodePtr toXML(const std::shared_ptr<Parameter>& parameter) {
            xmlNodePtr parameterNode = toXML(parameter, "P", false);

            xmlNodeSetContent(parameterNode, BAD_CAST toString(parameter).c_str());

            return parameterNode;
        }

        static xmlNodePtr toXML(const std::shared_ptr<NamedElement>& namedElement, const std::string& nodeMetaType, bool isListItem, const std::string nodeName = "") {
            xmlNodePtr node = nullptr;

            if(isListItem) {
                node = xmlNewNode(nullptr, BAD_CAST nodeName.c_str());
            } else {
                node = xmlNewNode(nullptr, BAD_CAST namedElement->getId().c_str());
            }

            xmlNewProp(node, BAD_CAST "MT", BAD_CAST nodeMetaType.c_str());

            return node;
        }

        static bool fromXML(Environment * environment, Model * model, const std::string& filename) {
            xmlDocPtr document = xmlReadFile(filename.c_str(), nullptr, 0);

            if (document == nullptr) {
                std::cerr << "Failed to parse document." << std::endl;
                return false;
            }

            xmlNodePtr rootNode = xmlDocGetRootElement(document);

            auto id = Common::xmlGetPropValue(rootNode, "id");
            auto version = Common::xmlGetPropValue(rootNode, "version");

            bool noErrors = fromXML(environment, model, rootNode);

            xmlFreeDoc(document);
            xmlCleanupParser();
            xmlMemoryDump();

            return noErrors;
        }

        static bool fromXML(Environment * UNUSED(environment), Model * model, xmlNodePtr node) {
            bool noErrors = true;

            xmlNodePtr modelNode = node->children->next;
            noErrors &= modelFromXML(model, modelNode);

            return noErrors;
        }

        static bool modelFromXML(Model * model, xmlNodePtr modelNode) {
            bool noErrors = true;

            // modelNode->children->next :                 Model's root component
            // modelNode->children->next->children->next : Root component's first child
            xmlNodePtr current = modelNode->children->next->children->next;

            while(current != nullptr) {
                std::string metatype = Common::xmlGetPropValue(current, "MT");

                if(metatype == "P") {
                    noErrors &= parameterFromXML(model->getRootComponent(), current);
                } else if(metatype == "C") {
                    noErrors &= componentFromXML(model->getRootComponent(), current);
                } else if(metatype == "L") {
                    noErrors &= listFromXML(model->getRootComponent(), current);
                }

                current = current->next->next;
            }

            return noErrors;
        }

        static bool parameterFromXML(const std::shared_ptr<Component>& component, xmlNodePtr parameterNode) {
            bool noErrors = true;

            std::string id = (char *) parameterNode->name;

            auto p = component->getParameter(id);

            if(p == nullptr) {
                std::cerr << "Parameter \"" + id + "\" does not exist in the core DataModel !" << std::endl;
                return false;
            }

            fromString(p, Common::xmlGetContent(parameterNode));

            return noErrors;
        }

        static bool componentFromXML(const std::shared_ptr<Component>& component, xmlNodePtr componentNode, const std::string& itemId = "") {
            bool noErrors = true;

            std::string id = (char *) componentNode->name;

            if(itemId != "") {
                id = itemId;
            }

            std::shared_ptr<Component> childComponent = component->getComponent(id);

            if(childComponent == nullptr) {
                std::cerr << "Component \"id\" does not exist in the core DataModel !" << std::endl;
                return false;
            }

            xmlNodePtr current = nullptr;

            if(componentNode->children != nullptr) {
                current = componentNode->children->next;
            }

            while(current != nullptr) {
                std::string metatype = Common::xmlGetPropValue(current, "MT");

                if (metatype == "P") {
                    noErrors &= parameterFromXML(childComponent, current);
                } else if(metatype == "C") {
                    noErrors &= componentFromXML(childComponent, current);
                } else if(metatype == "L") {
                    noErrors &= listFromXML(childComponent, current);
                }

                current = current->next->next;
            }

            return noErrors;
        }

        static bool listFromXML(const std::shared_ptr<Component>& component, xmlNodePtr listNode) {
            bool noErrors = true;

            std::string id = (char *) listNode->name;
            xmlNodePtr patternNode = nullptr;

            if(listNode->children != nullptr) {
                patternNode = listNode->children->next;
            }

            std::shared_ptr<List> list = component->getList(id);

            if(list == nullptr) {
                std::cerr << "List \"" << id << "\" does not exist in the core DataModel !" << std::endl;
                return false;
            }

            list->clear();

            xmlNodePtr current = patternNode;

            while(current != nullptr) {
                std::string metatype = Common::xmlGetPropValue(current, "MT");
                std::string itemId = (char *) current->name;

                auto item = list->addItem();

                if (metatype == "P") {
                    noErrors &= parameterFromXML(list, current);
                } else if (metatype == "C") {
                    noErrors &= componentFromXML(list, current, item->getId());
                } else if (metatype == "L") {
                    noErrors &= listFromXML(list, current);
                }

                current = current->next->next;
            }

            return noErrors;
        }
    };

    struct XSD {
        static xmlNodePtr toXSD(const Model * model) {
            xmlNodePtr schemaNode = xmlNewNode(nullptr, BAD_CAST "xs:schema");
            xmlNewProp(schemaNode, BAD_CAST "xmlns:xs", BAD_CAST "http://www.w3.org/2001/XMLSchema");
            xmlNewProp(schemaNode, BAD_CAST "elementFormDefault", BAD_CAST "qualified");

            xmlNodePtr modelNode = xmlNewNode(nullptr, BAD_CAST "xs:element");
            xmlNewProp(modelNode, BAD_CAST "name", BAD_CAST "DataModel");
            xmlAddChild(schemaNode, modelNode);
            xmlNodePtr complexTypeNode = xmlNewNode(nullptr, BAD_CAST "xs:complexType");
            xmlAddChild(modelNode, complexTypeNode);
            xmlNodePtr sequenceNode = xmlNewNode(nullptr, BAD_CAST "xs:sequence");
            xmlAddChild(complexTypeNode, sequenceNode);
            xmlNodePtr rootElementNode = xmlNewNode(nullptr, BAD_CAST "xs:element");
            xmlNewProp(rootElementNode, BAD_CAST "ref", BAD_CAST "root");
            xmlAddChild(sequenceNode, rootElementNode);

            xmlNodePtr configurationNode = xmlNewNode(nullptr, BAD_CAST "xs:element");
            xmlNewProp(configurationNode, BAD_CAST "name", BAD_CAST "Configuration");
            xmlAddChild(schemaNode, configurationNode);

            xmlNodePtr annotationNode = xmlNewNode(nullptr, BAD_CAST "xs:annotation");
            xmlAddChild(configurationNode, annotationNode);
            xmlNodePtr documentationNode = xmlNewNode(nullptr, BAD_CAST "xs:documentation");
            xmlNewProp(documentationNode, BAD_CAST "xml:lang", BAD_CAST "en");
            xmlAddChild(annotationNode, documentationNode);
            xmlNodePtr descDocNode = xmlNewNode(nullptr, BAD_CAST "description");
            xmlNodeSetContent(descDocNode, BAD_CAST model->getDescription().c_str());
            xmlAddChild(documentationNode, descDocNode);
            xmlNodePtr nameDocNode = xmlNewNode(nullptr, BAD_CAST "name");
            xmlNodeSetContent(nameDocNode, BAD_CAST model->getName().c_str());
            xmlAddChild(documentationNode, nameDocNode);
            xmlNodePtr versionDocNode = xmlNewNode(nullptr, BAD_CAST "version");
            xmlNodeSetContent(versionDocNode, BAD_CAST model->getVersion().c_str());
            xmlAddChild(documentationNode, versionDocNode);
            xmlNodePtr idDocNode = xmlNewNode(nullptr, BAD_CAST "id");
            xmlNodeSetContent(idDocNode, BAD_CAST model->getId().c_str());
            xmlAddChild(documentationNode, idDocNode);

            complexTypeNode = xmlNewNode(nullptr, BAD_CAST "xs:complexType");
            xmlAddChild(configurationNode, complexTypeNode);
            sequenceNode = xmlNewNode(nullptr, BAD_CAST "xs:sequence");
            xmlAddChild(complexTypeNode, sequenceNode);
            xmlNodePtr modelElementNode = xmlNewNode(nullptr, BAD_CAST "xs:element");
            xmlNewProp(modelElementNode, BAD_CAST "ref", BAD_CAST "DataModel");
            xmlAddChild(sequenceNode, modelElementNode);

            xmlNodePtr idNode = xmlNewNode(nullptr, BAD_CAST "xs:attribute");
            xmlNewProp(idNode, BAD_CAST "name", BAD_CAST "id");
            xmlNewProp(idNode, BAD_CAST "type", BAD_CAST "xs:string");
            xmlAddChild(complexTypeNode, idNode);

            xmlNodePtr versionNode = xmlNewNode(nullptr, BAD_CAST "xs:attribute");
            xmlNewProp(versionNode, BAD_CAST "name", BAD_CAST "version");
            xmlNewProp(versionNode, BAD_CAST "type", BAD_CAST "xs:string");
            xmlAddChild(complexTypeNode, versionNode);

            xmlNodePtr modelRootNode = toXSD(model->getRootComponent());
            xmlAddChild(schemaNode, modelRootNode);

            for(auto& enumType : model->getEnvironment()->getEnumTypeList()) {
                xmlNodePtr enumNode = toXSD(enumType);
                xmlAddChild(schemaNode, enumNode);
            }

            return schemaNode;
        }

        static xmlNodePtr toXSD(const std::shared_ptr<Enum>& enumType) {
            xmlNodePtr enumNode = xmlNewNode(nullptr, BAD_CAST "xs:simpleType");
            xmlNewProp(enumNode, BAD_CAST "name", BAD_CAST enumType->getId().c_str());

            xmlNodePtr annotationNode = xmlNewNode(nullptr, BAD_CAST "xs:annotation");
            xmlAddChild(enumNode, annotationNode);
            xmlNodePtr documentationNode = xmlNewNode(nullptr, BAD_CAST "xs:documentation");
            xmlNewProp(documentationNode, BAD_CAST "xml:lang", BAD_CAST "en");
            xmlAddChild(annotationNode, documentationNode);
            xmlNodePtr descDocNode = xmlNewNode(nullptr, BAD_CAST "description");
            xmlNodeSetContent(descDocNode, BAD_CAST enumType->getDescription().c_str());
            xmlAddChild(documentationNode, descDocNode);
            xmlNodePtr nameDocNode = xmlNewNode(nullptr, BAD_CAST "name");
            xmlNodeSetContent(nameDocNode, BAD_CAST enumType->getName().c_str());
            xmlAddChild(documentationNode, nameDocNode);

            xmlNodePtr restrictionNode = xmlNewNode(nullptr, BAD_CAST "xs:restriction");
            xmlNewProp(restrictionNode, BAD_CAST "base", BAD_CAST "xs:string");
            xmlAddChild(enumNode, restrictionNode);

            for(const auto& value : enumType->getValues()) {
                xmlNodePtr valueNode = xmlNewNode(nullptr, BAD_CAST "xs:enumeration");
                xmlNewProp(valueNode, BAD_CAST "value", BAD_CAST value.c_str());
                xmlAddChild(restrictionNode, valueNode);
            }

            return enumNode;
        }

        static xmlNodePtr toXSD(const std::shared_ptr<List>& list) {
            xmlNodePtr listNode = xmlNewNode(nullptr, BAD_CAST "xs:element");
            xmlNewProp(listNode, BAD_CAST "name", BAD_CAST list->getId().c_str());

            xmlNodePtr annotationNode = xmlNewNode(nullptr, BAD_CAST "xs:annotation");
            xmlAddChild(listNode, annotationNode);
            xmlNodePtr documentationNode = xmlNewNode(nullptr, BAD_CAST "xs:documentation");
            xmlNewProp(documentationNode, BAD_CAST "xml:lang", BAD_CAST "en");
            xmlAddChild(annotationNode, documentationNode);
            xmlNodePtr descDocNode = xmlNewNode(nullptr, BAD_CAST "description");
            xmlNodeSetContent(descDocNode, BAD_CAST list->getDescription().c_str());
            xmlAddChild(documentationNode, descDocNode);
            xmlNodePtr nameDocNode = xmlNewNode(nullptr, BAD_CAST "name");
            xmlNodeSetContent(nameDocNode, BAD_CAST list->getName().c_str());
            xmlAddChild(documentationNode, nameDocNode);
            xmlNodePtr refPathDocNode = xmlNewNode(nullptr, BAD_CAST "refPath");
            xmlNodeSetContent(refPathDocNode, BAD_CAST list->getReference().first.c_str());
            xmlAddChild(documentationNode, refPathDocNode);
            xmlNodePtr refValueDocNode = xmlNewNode(nullptr, BAD_CAST "refValue");
            xmlNodeSetContent(refValueDocNode, BAD_CAST list->getReference().second.c_str());
            xmlAddChild(documentationNode, refValueDocNode);
            xmlNodePtr visibilityNode = xmlNewNode(nullptr, BAD_CAST "visibility");
            xmlNodeSetContent(visibilityNode, BAD_CAST Visibility::toString(list->getVisibility()).c_str());
            xmlAddChild(documentationNode, visibilityNode);

            xmlNodePtr complexTypeNode = xmlNewNode(nullptr, BAD_CAST "xs:complexType");
            xmlAddChild(listNode, complexTypeNode);

            xmlNodePtr choiceNode = xmlNewNode(nullptr, BAD_CAST "xs:choice");
            xmlAddChild(complexTypeNode, choiceNode);

            xmlNodePtr metatypeNode = xmlNewNode(nullptr, BAD_CAST "xs:attribute");
            xmlNewProp(metatypeNode, BAD_CAST "type", BAD_CAST "xs:string");
            xmlNewProp(metatypeNode, BAD_CAST "name", BAD_CAST "MT");
            xmlAddChild(complexTypeNode, metatypeNode);

            std::string nodeName = list->getId() + "_item";
            xmlNodePtr patternNode = toXSD(list->getPattern(), nodeName);
            xmlNewProp(patternNode, BAD_CAST "minOccurs", BAD_CAST "0");
            xmlNewProp(patternNode, BAD_CAST "maxOccurs", BAD_CAST "unbounded");
            xmlAddChild(choiceNode, patternNode);

            return listNode;
        }

        static xmlNodePtr toXSD(const std::shared_ptr<Component>& component, const std::string& nodeName = "") {
            xmlNodePtr componentNode = xmlNewNode(nullptr, BAD_CAST "xs:element");

            if(nodeName != "") {
                xmlNewProp(componentNode, BAD_CAST "name", BAD_CAST nodeName.c_str());
            } else {
                xmlNewProp(componentNode, BAD_CAST "name", BAD_CAST component->getId().c_str());
            }

            xmlNodePtr annotationNode = xmlNewNode(nullptr, BAD_CAST "xs:annotation");
            xmlAddChild(componentNode, annotationNode);
            xmlNodePtr documentationNode = xmlNewNode(nullptr, BAD_CAST "xs:documentation");
            xmlNewProp(documentationNode, BAD_CAST "xml:lang", BAD_CAST "en");
            xmlAddChild(annotationNode, documentationNode);
            xmlNodePtr descDocNode = xmlNewNode(nullptr, BAD_CAST "description");
            xmlNodeSetContent(descDocNode, BAD_CAST component->getDescription().c_str());
            xmlAddChild(documentationNode, descDocNode);
            xmlNodePtr nameDocNode = xmlNewNode(nullptr, BAD_CAST "name");
            xmlNodeSetContent(nameDocNode, BAD_CAST component->getName().c_str());
            xmlAddChild(documentationNode, nameDocNode);
            xmlNodePtr refPathDocNode = xmlNewNode(nullptr, BAD_CAST "refPath");
            xmlNodeSetContent(refPathDocNode, BAD_CAST component->getReference().first.c_str());
            xmlAddChild(documentationNode, refPathDocNode);
            xmlNodePtr refValueDocNode = xmlNewNode(nullptr, BAD_CAST "refValue");
            xmlNodeSetContent(refValueDocNode, BAD_CAST component->getReference().second.c_str());
            xmlAddChild(documentationNode, refValueDocNode);
            xmlNodePtr visibilityNode = xmlNewNode(nullptr, BAD_CAST "visibility");
            xmlNodeSetContent(visibilityNode, BAD_CAST Visibility::toString(component->getVisibility()).c_str());
            xmlAddChild(documentationNode, visibilityNode);

            xmlNodePtr complexTypeNode = xmlNewNode(nullptr, BAD_CAST "xs:complexType");
            xmlAddChild(componentNode, complexTypeNode);

            xmlNodePtr sequenceNode = xmlNewNode(nullptr, BAD_CAST "xs:sequence");
            xmlAddChild(complexTypeNode, sequenceNode);
            xmlNodePtr metatypeNode = xmlNewNode(nullptr, BAD_CAST "xs:attribute");
            xmlNewProp(metatypeNode, BAD_CAST "type", BAD_CAST "xs:string");
            xmlNewProp(metatypeNode, BAD_CAST "name", BAD_CAST "MT");
            xmlAddChild(complexTypeNode, metatypeNode);

            for (const auto& parameter : component->getParameterList()) {
                if (parameter != nullptr) {
                    xmlNodePtr parameterNode = toXSD(parameter);
                    xmlAddChild(sequenceNode, parameterNode);
                }
            }

            for (const auto& child : component->getComponentList()) {
                if (child != nullptr) {
                    xmlNodePtr childNode = toXSD(child);
                    xmlAddChild(sequenceNode, childNode);
                }
            }

            for (const auto& list : component->getListList()) {
                if (list != nullptr) {
                    xmlNodePtr listNode = toXSD(list);
                    xmlAddChild(sequenceNode, listNode);
                }
            }

            return componentNode;
        }

        static xmlNodePtr toXSD(const std::shared_ptr<Parameter>& parameter) {
            xmlNodePtr parameterNode = xmlNewNode(nullptr, BAD_CAST "xs:element");
            xmlNewProp(parameterNode, BAD_CAST "name", BAD_CAST parameter->getId().c_str());

            xmlNodePtr annotationNode = xmlNewNode(nullptr, BAD_CAST "xs:annotation");
            xmlAddChild(parameterNode, annotationNode);
            xmlNodePtr documentationNode = xmlNewNode(nullptr, BAD_CAST "xs:documentation");
            xmlNewProp(documentationNode, BAD_CAST "xml:lang", BAD_CAST "en");
            xmlAddChild(annotationNode, documentationNode);
            xmlNodePtr descDocNode = xmlNewNode(nullptr, BAD_CAST "description");
            xmlNodeSetContent(descDocNode, BAD_CAST parameter->getDescription().c_str());
            xmlAddChild(documentationNode, descDocNode);
            xmlNodePtr nameDocNode = xmlNewNode(nullptr, BAD_CAST "name");
            xmlNodeSetContent(nameDocNode, BAD_CAST parameter->getName().c_str());
            xmlAddChild(documentationNode, nameDocNode);
            xmlNodePtr refPathDocNode = xmlNewNode(nullptr, BAD_CAST "refPath");
            xmlNodeSetContent(refPathDocNode, BAD_CAST parameter->getReference().first.c_str());
            xmlAddChild(documentationNode, refPathDocNode);
            xmlNodePtr refValueDocNode = xmlNewNode(nullptr, BAD_CAST "refValue");
            xmlNodeSetContent(refValueDocNode, BAD_CAST parameter->getReference().second.c_str());
            xmlAddChild(documentationNode, refValueDocNode);
            xmlNodePtr visibilityNode = xmlNewNode(nullptr, BAD_CAST "visibility");
            xmlNodeSetContent(visibilityNode, BAD_CAST Visibility::toString(parameter->getVisibility()).c_str());
            xmlAddChild(documentationNode, visibilityNode);
            xmlNodePtr unitDocNode = xmlNewNode(nullptr, BAD_CAST "unit");
            xmlNodeSetContent(unitDocNode, BAD_CAST parameter->getUnit().c_str());
            xmlAddChild(documentationNode, unitDocNode);
            xmlNodePtr typeDocNode = xmlNewNode(nullptr, BAD_CAST "type");
            xmlNodeSetContent(typeDocNode, BAD_CAST parameter->getType()->getId().c_str());
            xmlAddChild(documentationNode, typeDocNode);
            xmlNodePtr defaultDocNode = xmlNewNode(nullptr, BAD_CAST "default");
            xmlNodeSetContent(defaultDocNode, BAD_CAST defaultToString(parameter).c_str());
            xmlAddChild(documentationNode, defaultDocNode);

            xmlNodePtr complexTypeNode = xmlNewNode(nullptr, BAD_CAST "xs:complexType");
            xmlAddChild(parameterNode, complexTypeNode);

            xmlNodePtr simpleContentNode = xmlNewNode(nullptr, BAD_CAST "xs:simpleContent");
            xmlAddChild(complexTypeNode, simpleContentNode);

            xmlNodePtr extensionNode = xmlNewNode(nullptr, BAD_CAST "xs:extension");
            xmlNewProp(extensionNode, BAD_CAST "base", BAD_CAST "xs:string");
            xmlAddChild(simpleContentNode, extensionNode);

            xmlNodePtr metatypeNode = xmlNewNode(nullptr, BAD_CAST "xs:attribute");
            xmlNewProp(metatypeNode, BAD_CAST "type", BAD_CAST "xs:string");
            xmlNewProp(metatypeNode, BAD_CAST "name", BAD_CAST "MT");
            xmlAddChild(extensionNode, metatypeNode);

            return parameterNode;
        }

        static xmlNodePtr toXSD(const std::shared_ptr<NamedElement>& namedElement, const std::string& nodeName) {
            xmlNodePtr node = xmlNewNode(nullptr, BAD_CAST nodeName.c_str());

            xmlNewProp(node, BAD_CAST "id", BAD_CAST namedElement->getId().c_str());
            xmlNewProp(node, BAD_CAST "name", BAD_CAST namedElement->getName().c_str());
            xmlNewProp(node, BAD_CAST "description", BAD_CAST namedElement->getDescription().c_str());

            return node;
        }

        static bool fromXSD(Environment ** environment, Model ** model, const std::string& filename) {
            xmlDocPtr document = xmlReadFile(filename.c_str(), nullptr, 0);

            if (document == nullptr) {
                std::cerr << "Failed to parse document." << std::endl;
                return false;
            }


            xmlNodePtr configurationNode = xmlDocGetRootElement(document); // = <xs:schema> node
            configurationNode = configurationNode->children->next; // = <xs:element name="DataModel"> node
            configurationNode = configurationNode->next->next; // = <xs:element name="Configuration" version="..."> node

            auto id = Common::xsdGetId(configurationNode);
            auto name = Common::xsdGetName(configurationNode);
            auto description = Common::xsdGetDescription(configurationNode);
            auto version = Common::xsdGetVersion(configurationNode);

            *environment = new Environment();
            *model = new Model(version, id, name, description);
            (*model)->setEnvironment(*environment);

            xmlNodePtr rootNode = configurationNode->next->next; // = <xs:element name="root"> node

            bool noErrors = fromXSD(*environment, *model, rootNode);

            xmlFreeDoc(document);
            xmlCleanupParser();
            xmlMemoryDump();

            return noErrors;
        }

        static bool fromXSD(Environment * environment, Model * model, xmlNodePtr node) {
            bool noErrors = true;

            // Check si nullptr nécessaire?..
            xmlNodePtr firstEnumNode = (node->next != nullptr) ? node->next->next : nullptr;

            noErrors &= environmentFromXSD(environment, firstEnumNode);
            noErrors &= modelFromXSD(model, node);

            return noErrors;
        }

        static bool environmentFromXSD(Environment * environment, xmlNodePtr firstEnumNode) {
            bool noErrors = true;

            xmlNodePtr current = firstEnumNode;

            while(current != nullptr) {
                auto id = Common::xmlGetPropValue(current, "name");
                auto name = Common::xsdGetName(current);
                auto description = Common::xsdGetDescription(current);

                auto enumeration = environment->addEnumType(id, name, description);

                xmlNodePtr currentValue = current->children->next->next->next;
                currentValue = (currentValue->children != nullptr) ? currentValue->children->next : nullptr;

                while(currentValue != nullptr) {
                    auto value = Common::xmlGetPropValue(currentValue, "value");
                    enumeration->addValue(value);

                    currentValue = currentValue->next->next;
                }

                current = current->next->next;
            }

            return noErrors;
        }

        static bool enumFromXSD(Environment * environment, xmlNodePtr enumNode) {
            bool noErrors = true;

            auto id = Common::xmlGetPropValue(enumNode, "id");
            auto name = Common::xmlGetPropValue(enumNode, "name");
            auto description = Common::xmlGetPropValue(enumNode, "description");

            auto enumType = environment->addEnumType(id, name, description);

            xmlNodePtr current = nullptr;

            if(enumNode->children != nullptr) {
                current = enumNode->children->next;
            }

            while(current != nullptr) {
                enumType->addValue(Common::xmlGetContent(current));
                current = current->next->next;
            }

            return noErrors;
        }

        static bool modelFromXSD(Model * model, xmlNodePtr modelNode) {
            bool noErrors = true;

            // modelNode :                                             <xs:element name="root">
            // modelNode->children->next :                             <xs:annotation>
            // modelNode->children->next->next->next :                 <xs:complexType>
            // modelNode->children->next->next->next->children->next : <xs:sequence>
            xmlNodePtr current = modelNode->children->next->next->next->children->next->children->next;

            while(current != nullptr) {
                std::string metatype = Common::xmlGetMetatype(current);

                if(metatype == "P") {
                    noErrors &= parameterFromXSD(model->getRootComponent(), current);
                } else if(metatype == "C") {
                    noErrors &= componentFromXSD(model->getRootComponent(), current);
                } else if(metatype == "L") {
                    noErrors &= listFromXSD(model->getRootComponent(), current);
                }

                current = current->next->next;
            }

            return noErrors;
        }

        static bool parameterFromXSD(const std::shared_ptr<Component>& component, xmlNodePtr parameterNode) {
            bool noErrors = true;

            auto id = Common::xmlGetPropValue(parameterNode, "name");
            auto name = Common::xsdGetName(parameterNode);
            auto description = Common::xsdGetDescription(parameterNode);
            auto type = Common::xsdGetType(parameterNode);
            auto unit = Common::xsdGetUnit(parameterNode);
            auto defaultValue = Common::xsdGetDefault(parameterNode);
            auto refPath = Common::xsdGetRefPath(parameterNode);
            auto refValue = Common::xsdGetRefValue(parameterNode);
            auto visibility = Common::xsdGetVisibility(parameterNode);

            TypeID typeId = Common::getTypeIDFromString(type);
            std::shared_ptr<Type> metaType = nullptr;

            if(typeId != COUNT) {
                metaType = component->getModel()->getEnvironment()->getPrimitiveType(typeId);
            }

            // Type is a primitive data type (exists in the environment)
            if(metaType != nullptr) {
                auto p = component->addParameter(Common::getTypeIDFromString(type), id, name, description, unit);

                if(defaultValue != "") {
                    defaultFromString(p, defaultValue);
                }

                p->setReference(refPath, refValue);
                p->setVisibility(Visibility::fromString(visibility));
            }
            // Type is an enum data type
            else {
                auto p = component->addParameter(type, id, name, description, unit);

                if(defaultValue != "") {
                    defaultFromString(p, defaultValue);
                }

                p->setReference(refPath, refValue);
                p->setVisibility(Visibility::fromString(visibility));
            }

            return noErrors;
        }

        static bool componentFromXSD(const std::shared_ptr<Component>& component, xmlNodePtr componentNode) {
            bool noErrors = true;

            auto id = Common::xmlGetPropValue(componentNode, "name");
            auto name = Common::xsdGetName(componentNode);
            auto description = Common::xsdGetDescription(componentNode);
            auto refPath = Common::xsdGetRefPath(componentNode);
            auto refValue = Common::xsdGetRefValue(componentNode);
            auto visibility = Common::xsdGetVisibility(componentNode);

            std::shared_ptr<Component> childComponent = component->addComponent(id, name, description);
            childComponent->setReference(refPath, refValue);
            childComponent->setVisibility(Visibility::fromString(visibility));

            xmlNodePtr current = nullptr;

            if(componentNode->children->next->next->next->children->next->children != nullptr) {
                current = componentNode->children->next->next->next->children->next->children->next;
            }

            while(current != nullptr) {
                std::string metatype = Common::xmlGetMetatype(current);

                if (metatype == "P") {
                    noErrors &= parameterFromXSD(childComponent, current);
                } else if(metatype == "C") {
                    noErrors &= componentFromXSD(childComponent, current);
                } else if(metatype == "L") {
                    noErrors &= listFromXSD(childComponent, current);
                }

                current = current->next->next;
            }

            return noErrors;
        }

        static bool listFromXSD(const std::shared_ptr<Component>& component, xmlNodePtr listNode) {
            bool noErrors = true;

            xmlNodePtr patternNode = listNode->children->next->next->next->children->next->children->next;

            auto id = Common::xmlGetPropValue(listNode, "name");
            auto name = Common::xsdGetName(listNode);
            auto description = Common::xsdGetDescription(listNode);
            auto descriptionPattern = Common::xsdGetDescription(patternNode);
            auto refPath = Common::xsdGetRefPath(listNode);
            auto refValue = Common::xsdGetRefValue(listNode);
            auto visibility = Common::xsdGetVisibility(listNode);

            std::shared_ptr<List> list = component->addList(id, name, description, descriptionPattern);
            list->setReference(refPath, refValue);
            list->setVisibility(Visibility::fromString(visibility));

            xmlNodePtr current = nullptr;

            if(patternNode->children->next->next->next->children->next->children != nullptr) {
                current = patternNode->children->next->next->next->children->next->children->next;
            }

            while(current != nullptr) {
                std::string metatype = Common::xmlGetMetatype(current);

                if (metatype == "P") {
                    noErrors &= parameterFromXSD(list->getPattern(), current);
                } else if (metatype == "C") {
                    noErrors &= componentFromXSD(list->getPattern(), current);
                } else if (metatype == "L") {
                    noErrors &= listFromXSD(list->getPattern(), current);
                }

                current = current->next->next;
            }

            return noErrors;
        }
    };
}

namespace {
    template<typename T>
    struct StringUtils {
        static inline std::any fromString(const std::string& string) {
            std::stringstream stringStream(string);

            T typedValue;
            stringStream >> typedValue;

            return typedValue;
        }

        static inline std::string toString(const std::any& any) {
            return std::to_string(std::any_cast<T>(any));
        }
    };

    template<>
    struct StringUtils<Float> {
        static inline std::any fromString(const std::string& string) {
            std::ostringstream out;
            out << std::setprecision(CONFIGURATION_FLOAT_PRECISION) << std::stof(string);
            Float typedValue = std::stof(out.str());

            return typedValue;
        }

        static inline std::string toString(const std::any& any) {
            Float typedValue = std::any_cast<Float>(any);

            std::ostringstream stringStream;

            stringStream << std::fixed;
            stringStream << std::setprecision(CONFIGURATION_FLOAT_PRECISION);
            stringStream << typedValue;

            return stringStream.str();
        }
    };

    template<>
    struct StringUtils<Double> {
        static inline std::any fromString(const std::string& string) {
            std::ostringstream out;
            out << std::setprecision(CONFIGURATION_LONG_DOUBLE_PRECISION) << std::stod(string);
            Double typedValue = std::stod(out.str());

            return typedValue;
        }

        static inline std::string toString(const std::any& any) {
            Double typedValue = std::any_cast<Double>(any);

            std::ostringstream stringStream;

            stringStream << std::fixed;
            stringStream << std::setprecision(CONFIGURATION_DOUBLE_PRECISION);
            stringStream << typedValue;

            return stringStream.str();
        }
    };

    template<>
    struct StringUtils<LongDouble> {
        static inline std::any fromString(const std::string& string) {
            std::ostringstream out;
            out << std::setprecision(CONFIGURATION_LONG_DOUBLE_PRECISION) << std::stold(string);
            LongDouble typedValue = std::stold(out.str());

            return typedValue;
        }

        static inline std::string toString(const std::any& any) {
            LongDouble typedValue = std::any_cast<LongDouble>(any);

            std::ostringstream stringStream;

            stringStream << std::fixed;
            stringStream << std::setprecision(CONFIGURATION_LONG_DOUBLE_PRECISION);
            stringStream << typedValue;

            return stringStream.str();
        }
    };

    template<>
    struct StringUtils<Boolean> {
        static std::any fromString(const std::string& string) {
            if (string == "true") {
                return true;
            }
            else if (string == "false") {
                return false;
            }

            throw "Illegal \"fromString\" argument provided.";
        }

        static inline std::string toString(const std::any& any) {
            bool typedValue = std::any_cast<bool>(any);

            if (typedValue) {
                return "true";
            }

            return "false";
        }
    };

    template<>
    struct StringUtils<String> {
        static inline std::any fromString(const std::string& string) {
            return String(string);
        }

        static inline std::string toString(const std::any& any) {
            return std::any_cast<std::string>(any);
        }
    };

    template<>
    struct StringUtils<Byte> {
        static inline std::any fromString(const std::string& string) {
            if(string.length() > 0) {
                Byte typedValue = static_cast<Byte>(string.c_str()[0]);
                return typedValue;
            } else {
                throw "Illegal \"fromString\" argument provided.";
            }
        }

        static inline std::string toString(const std::any& any) {
            Byte typedValue = std::any_cast<Byte>(any);

            return std::string(1, typedValue);
        }
    };

    template<>
    struct StringUtils<Char> {
        static inline std::any fromString(const std::string& string) {
            if(string.length() > 0) {
                Char typedValue = static_cast<Char>(string.c_str()[0]);
                return typedValue;
            } else {
                throw "Illegal \"fromString\" argument provided.";
            }
        }

        static inline std::string toString(const std::any& any) {
            Char typedValue = std::any_cast<Char>(any);

            return std::string(1, typedValue);
        }
    };

    template<>
    struct StringUtils<std::any> {
        static inline std::any fromString(const std::string& UNUSED(string)) {
            throw "Impossible!";
        }

        static inline std::string toString(std::any const& UNUSED(any)) {
            throw "Impossible!";
        }
    };

    static std::string toString(std::shared_ptr<Parameter> parameter) {
        if(!parameter->isDefined()) {
            return "";
        }

        auto metaType = parameter->getType();

        if(metaType->getType() == PRIMITIVE) {
            // We have to check that "value" is instanceof the good TypeID
            switch(metaType->getTypeID()) {
                case BOOLEAN:
                    return StringUtils<Boolean>::toString(parameter->getValue<Boolean>());
                case BYTE:
                    return StringUtils<Byte>::toString(parameter->getValue<Byte>());
                case CHAR:
                    return StringUtils<Char>::toString(parameter->getValue<Char>());
                case DOUBLE:
                    return StringUtils<Double>::toString(parameter->getValue<Double>());
                case FLOAT:
                    return StringUtils<Float>::toString(parameter->getValue<Float>());
                case INT:
                    return StringUtils<Int>::toString(parameter->getValue<Int>());
                case LONG:
                    return StringUtils<Long>::toString(parameter->getValue<Long>());
                case LONGDOUBLE:
                    return StringUtils<LongDouble>::toString(parameter->getValue<LongDouble>());
                case SHORT:
                    return StringUtils<Short>::toString(parameter->getValue<Short>());
                case STRING:
                    return StringUtils<String>::toString(parameter->getValue<String>());
                default:
                    std::cerr << "Unhandled primitive data type." << std::endl;
                    return "";
            }
        } else if(metaType->getType() == ENUM) {
            return StringUtils<String>::toString(parameter->getValue<String>());
        }
    }

    static std::string defaultToString(std::shared_ptr<Parameter> parameter) {
        if(!parameter->isDefaultDefined()) {
            return "";
        }

        auto metaType = parameter->getType();

        if(metaType->getType() == PRIMITIVE) {
            // We have to check that "value" is instanceof the good TypeID
            switch(metaType->getTypeID()) {
                case BOOLEAN:
                    return StringUtils<Boolean>::toString(parameter->getDefaultValue<Boolean>());
                case BYTE:
                    return StringUtils<Byte>::toString(parameter->getDefaultValue<Byte>());
                case CHAR:
                    return StringUtils<Char>::toString(parameter->getDefaultValue<Char>());
                case DOUBLE:
                    return StringUtils<Double>::toString(parameter->getDefaultValue<Double>());
                case FLOAT:
                    return StringUtils<Float>::toString(parameter->getDefaultValue<Float>());
                case INT:
                    return StringUtils<Int>::toString(parameter->getDefaultValue<Int>());
                case LONG:
                    return StringUtils<Long>::toString(parameter->getDefaultValue<Long>());
                case LONGDOUBLE:
                    return StringUtils<LongDouble>::toString(parameter->getDefaultValue<LongDouble>());
                case SHORT:
                    return StringUtils<Short>::toString(parameter->getDefaultValue<Short>());
                case STRING:
                    return StringUtils<String>::toString(parameter->getDefaultValue<String>());
                default:
                    std::cerr << "Unhandled primitive data type." << std::endl;
                    return "";
            }
        } else if(metaType->getType() == ENUM) {
            return StringUtils<String>::toString(parameter->getDefaultValue<String>());
        }
    }

    static void fromString(std::shared_ptr<Parameter> parameter, const std::string& value) {
        if(value != "") {
            auto metaType = parameter->getType();

            if(metaType->getType() == PRIMITIVE) {
                switch(metaType->getTypeID()) {
                    case BOOLEAN:
                        parameter->setValue<Boolean>(std::any_cast<Boolean>(StringUtils<Boolean>::fromString(value)));
                        break;
                    case BYTE:
                        parameter->setValue<Byte>(std::any_cast<Byte>(StringUtils<Byte>::fromString(value)));
                        break;
                    case CHAR:
                        parameter->setValue<Char>(std::any_cast<Char>(StringUtils<Char>::fromString(value)));
                        break;
                    case DOUBLE:
                        parameter->setValue<Double>(std::any_cast<Double>(StringUtils<Double>::fromString(value)));
                        break;
                    case FLOAT:
                        parameter->setValue<Float>(std::any_cast<Float>(StringUtils<Float>::fromString(value)));
                        break;
                    case INT:
                        parameter->setValue<Int>(std::any_cast<Int>(StringUtils<Int>::fromString(value)));
                        break;
                    case LONG:
                        parameter->setValue<Long>(std::any_cast<Long>(StringUtils<Long>::fromString(value)));
                        break;
                    case LONGDOUBLE:
                        parameter->setValue<LongDouble>(std::any_cast<LongDouble>(StringUtils<LongDouble>::fromString(value)));
                        break;
                    case SHORT:
                        parameter->setValue<Short>(std::any_cast<Short>(StringUtils<Short>::fromString(value)));
                        break;
                    case STRING:
                        parameter->setValue<String>(std::any_cast<String>(StringUtils<String>::fromString(value)));
                        break;
                    default:
                        std::cerr << "Unhandled primitive data type." << std::endl;
                        break;
                }
            } else if(metaType->getType() == ENUM) {
                parameter->setValue<String>(std::any_cast<String>(StringUtils<String>::fromString(value)));
            }
        }
    }

    static void defaultFromString(std::shared_ptr<Parameter> parameter, const std::string& value) {
        if(value != "") {
            auto metaType = parameter->getType();

            if(metaType->getType() == PRIMITIVE) {
                switch(metaType->getTypeID()) {
                    case BOOLEAN:
                        parameter->setDefaultValue<Boolean>(std::any_cast<Boolean>(StringUtils<Boolean>::fromString(value)));
                        break;
                    case BYTE:
                        parameter->setDefaultValue<Byte>(std::any_cast<Byte>(StringUtils<Byte>::fromString(value)));
                        break;
                    case CHAR:
                        parameter->setDefaultValue<Char>(std::any_cast<Char>(StringUtils<Char>::fromString(value)));
                        break;
                    case DOUBLE:
                        parameter->setDefaultValue<Double>(std::any_cast<Double>(StringUtils<Double>::fromString(value)));
                        break;
                    case FLOAT:
                        parameter->setDefaultValue<Float>(std::any_cast<Float>(StringUtils<Float>::fromString(value)));
                        break;
                    case INT:
                        parameter->setDefaultValue<Int>(std::any_cast<Int>(StringUtils<Int>::fromString(value)));
                        break;
                    case LONG:
                        parameter->setDefaultValue<Long>(std::any_cast<Long>(StringUtils<Long>::fromString(value)));
                        break;
                    case LONGDOUBLE:
                        parameter->setDefaultValue<LongDouble>(std::any_cast<LongDouble>(StringUtils<LongDouble>::fromString(value)));
                        break;
                    case SHORT:
                        parameter->setDefaultValue<Short>(std::any_cast<Short>(StringUtils<Short>::fromString(value)));
                        break;
                    case STRING:
                        parameter->setDefaultValue<String>(std::any_cast<String>(StringUtils<String>::fromString(value)));
                        break;
                    default:
                        std::cerr << "Unhandled primitive data type." << std::endl;
                        break;
                }
            } else if(metaType->getType() == ENUM) {
                parameter->setDefaultValue<String>(std::any_cast<String>(StringUtils<String>::fromString(value)));
            }
        }
    }
}

namespace Utils {
    /**
     * @brief Serialize a Model or a DataModel to a file.
     * @param model The model you want to serialize.
     * @param filename The file you want to serialize the model to.
     * @param isDataModel If true, will serialize the model as a DataModel. If false, as a Model.
     * @return true If everything went right.
     */
    static bool toXSD(const Model * model, const std::string& filename) {
        xmlDocPtr document = xmlNewDoc(BAD_CAST CONFIGURATION_FILES_VERSION);

        xmlNodePtr rootNode = XSD::toXSD(model);

        xmlDocSetRootElement(document, rootNode);

        if(xmlSaveFormatFileEnc(filename.c_str(), document, CONFIGURATION_FILES_ENCODING, 1) == -1) {
            return false;
        }

        xmlFreeDoc(document);
        xmlCleanupParser();
        xmlMemoryDump();

        return true;
    }

    /**
     * @brief Load a MetaModel and its environment from a file.
     *
     * When loading the MetaModel, you should pass environment and model parameters
     * as nullptr. This method will instantiate the Environment and the Model by reading
     * the XSD file.
     *
     * @param environment A pointer to the environment pointer.
     * @param model A pointer to the model pointer.
     * @param filename The XSD file you want to load the MetaModel from.
     * @return true If everything went right.
     */
    static bool fromXSD(Environment ** environment, Model ** model, const std::string& filename) {
        return XSD::fromXSD(environment, model, filename);
    }

    /**
     * @brief Serialize a DataModel to a XML file.
     * @param model The model you want to serialize.
     * @param filename The file path you want to serialize the model to.
     * @return true If everything went right.
     */
    static bool toXML(const Model * model, const std::string& filename) {
        xmlDocPtr document = xmlNewDoc(BAD_CAST CONFIGURATION_FILES_VERSION);

        xmlNodePtr rootNode = XML::toXML(model);

        xmlDocSetRootElement(document, rootNode);

        if(xmlSaveFormatFileEnc(filename.c_str(), document, CONFIGURATION_FILES_ENCODING, 1) == -1) {
            return false;
        }

        xmlFreeDoc(document);
        xmlCleanupParser();
        xmlMemoryDump();

        return true;
    }

    /**
     * @brief Update a Model and its environment from a XML file.
     *
     * When updating the DataModel, we just update the existing Model's values by
     * reading the XML file. The environment and the model must have been created before.
     *
     * @param environment A pointer to the environment pointer.
     * @param model A pointer to the model pointer.
     * @param filename The XML file you want to update the DataModel from.
     * @param xsdFilename The XSD file which you want to check the XML file against.
     * @return true If everything went right.
     */
     static bool fromXML(Environment ** environment, Model ** model, const std::string& filename, const std::string& xsdFilename) {
        bool success = true;

        success &= validate(xsdFilename, filename);
        success &= XML::fromXML(*environment, *model, filename);

        return success;
    }

    /**
     * @brief Validates a XML file against a XSD file.
     * @param xsdPath The path to the XSD document.
     * @param xmlPath The path to the XML document.
     * @return true If the XML validated against the XSD.
     */
    static bool validate(const std::string& xsdPath, const std::string& xmlPath) {
        int check = 1;

        xmlDocPtr document;
        xmlSchemaPtr schema = nullptr;
        xmlSchemaParserCtxtPtr parserContext;

        xmlLineNumbersDefault(1);

        parserContext = xmlSchemaNewParserCtxt(xsdPath.c_str());

        xmlSchemaSetParserErrors(parserContext, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);
        schema = xmlSchemaParse(parserContext);
        xmlSchemaFreeParserCtxt(parserContext);
        //xmlSchemaDump(stdout, schema); //To print schema dump

        document = xmlReadFile(xmlPath.c_str(), NULL, 0);

        if (document != nullptr) {
            xmlSchemaValidCtxtPtr validContext = xmlSchemaNewValidCtxt(schema);

            xmlSchemaSetValidErrors(validContext, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);
            check = xmlSchemaValidateDoc(validContext, document);

            if(check < 0) {
                std::cerr << "Internal error occured while validating \"" << xmlPath << "\" !" << std::endl;
            }

            xmlSchemaFreeValidCtxt(validContext);
            xmlFreeDoc(document);
        } else {
            std::cerr << "Could not parse \"" << xmlPath << "\" !" << std::endl;
        }

        if(schema != nullptr) {
            xmlSchemaFree(schema);
        }

        xmlSchemaCleanupTypes();
        xmlCleanupParser();
        xmlMemoryDump();

        return (check == 0);
    }
}

#endif // CONFIGURATION_UTILS_HPP
