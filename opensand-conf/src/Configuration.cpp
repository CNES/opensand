/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 Viveris Technologies
 * Copyright © 2020 TAS
 * Copyright © 2020 CNES
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
 * @file Configuration.h
 * @brief Functions to help MetaModel and DataModel (un)serialization.
 */

#define LIBXML_SCHEMAS_ENABLED
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlschemastypes.h>
#include <iostream>
#include <cstdio>
#include <locale>
#include <codecvt>

#include <vector>
#include <tuple>

#include "Configuration.h"
#include "MetaTypesList.h"

// FIXME: use xmlNs structure to handle 'xs:' prefix

#define CONFIGURATION_FILES_VERSION "1.0"
#define CONFIGURATION_FILES_ENCODING "UTF-8"

#define CONFIGURATION_FLOAT_PRECISION 10
#define CONFIGURATION_DOUBLE_PRECISION 20
#define CONFIGURATION_LONG_DOUBLE_PRECISION 30

// libxml2 extended functions
std::vector<xmlNodePtr> getChildNodes(xmlNodePtr &node, const std::string &ns, const std::string &name);
xmlNodePtr getUniqueChildNode(xmlNodePtr &node, const std::string &ns, const std::string &name);
std::vector<xmlNodePtr> getChildNodesWithAttribute(xmlNodePtr &node, const std::string &ns, const std::string &name, const std::string &attribute, const std::string &value);
xmlNodePtr getUniqueChildNodeWithAttribute(xmlNodePtr &node, const std::string &ns, const std::string &name, const std::string &attribute, const std::string &value);
std::string getAttribute(xmlNodePtr &node, const std::string &name);
std::string getNodeContent(xmlNodePtr &node);

// toXSD functions
std::vector<xmlNodePtr> enumerationsToXSD(std::shared_ptr<OpenSANDConf::MetaTypesList> types);
xmlNodePtr rootToXSD(std::shared_ptr<OpenSANDConf::MetaComponent> element);

// fromXSD functions
bool addEnumTypeFromXSD(std::shared_ptr<OpenSANDConf::MetaModel> model, xmlNodePtr node);
bool loadRootFromXSD(std::shared_ptr<OpenSANDConf::MetaModel> model, xmlNodePtr node);

// toXML functions
xmlNodePtr rootToXML(std::shared_ptr<OpenSANDConf::DataComponent> element);

// fromXML functions
bool loadRootFromXML(std::shared_ptr<OpenSANDConf::DataModel> datamodel, xmlNodePtr node);

bool OpenSANDConf::toXSD(std::shared_ptr<OpenSANDConf::MetaModel> model, const std::string &filepath)
{
	int code;

	// Initialize XSD document
	auto doc = xmlNewDoc(BAD_CAST CONFIGURATION_FILES_VERSION);
	auto schema = xmlNewNode(nullptr, BAD_CAST "xs:schema");

	// FIXME: use xmlNs structure to handle 'xs:' prefix
	xmlNewProp(schema, BAD_CAST "xmlns:xs", BAD_CAST "http://www.w3.org/2001/XMLSchema");
	xmlNewProp(schema, BAD_CAST "elementFormDefault", BAD_CAST "qualified");
	xmlDocSetRootElement(doc, schema);

	auto node = xmlNewNode(nullptr, BAD_CAST "xs:element");
	xmlNewProp(node, BAD_CAST "name", BAD_CAST "model");
	xmlAddChild(schema, node);

	auto annot = xmlNewNode(nullptr, BAD_CAST "xs:annotation");
	xmlAddChild(node, annot);

	auto docu = xmlNewNode(nullptr, BAD_CAST "xs:documentation");
	xmlAddChild(annot, docu);
	xmlNewProp(docu, BAD_CAST "xml:lang", BAD_CAST "en");

	auto version = xmlNewNode(nullptr, BAD_CAST "version");
	xmlAddChild(docu, version);
	xmlNodeSetContent(version, BAD_CAST model->getVersion().c_str());

	auto comp = xmlNewNode(nullptr, BAD_CAST "xs:complexType");
	xmlAddChild(node, comp);
	auto seq = xmlNewNode(nullptr, BAD_CAST "xs:sequence");
	xmlAddChild(comp, seq);

	auto attr = xmlNewNode(nullptr, BAD_CAST "xs:attribute");
	xmlNewProp(attr, BAD_CAST "name", BAD_CAST "version");
	xmlNewProp(attr, BAD_CAST "type", BAD_CAST "xs:string");
	xmlNewProp(attr, BAD_CAST "use", BAD_CAST "required");
	xmlAddChild(comp, attr);

	// Append enumeration type schemas
	auto enums = enumerationsToXSD(model->getTypesDefinition());
	for (auto enode : enums)
	{
		xmlAddChild(schema, enode);
	}
	enums.clear();

	// Append root schema
	auto root = rootToXSD(model->getRoot());
	if (root == nullptr)
	{
		xmlFreeDoc(doc);
		xmlCleanupParser();
		xmlMemoryDump();
		return false;
	}
	xmlAddChild(seq, root);

	// Write XSD file
	code = xmlSaveFormatFileEnc(filepath.c_str(), doc, CONFIGURATION_FILES_ENCODING, 1);
	xmlFreeDoc(doc);
	xmlCleanupParser();
	xmlMemoryDump();
	return (code != -1);
}

std::shared_ptr<OpenSANDConf::MetaModel> OpenSANDConf::fromXSD(const std::string &filepath)
{
	xmlDocPtr doc;
	std::shared_ptr<OpenSANDConf::MetaModel> model = nullptr;
	std::string version;
	xmlNodePtr schema, modelnode, node1, node2;
	xmlNodePtr rootnode;
	std::vector<xmlNodePtr> nodes;
	std::vector<xmlNodePtr> enumsnodes;

	// Read document
	doc = xmlReadFile(filepath.c_str(), CONFIGURATION_FILES_ENCODING, 0);
	if (doc == nullptr)
	{
		return nullptr;
	}
	schema = xmlDocGetRootElement(doc);
	if (schema == nullptr)
	{
		return nullptr;
	}

	// Find model node
	modelnode = getUniqueChildNode(schema, "xs", "element");
	if (modelnode == nullptr || xmlChildElementCount(modelnode) != 2)
	{
		goto error;
	}
	node1 = getUniqueChildNode(modelnode, "xs", "annotation");
	if (node1 == nullptr)
	{
		goto error;
	}
	node2 = getUniqueChildNode(node1, "xs", "documentation");
	if (node2 == nullptr)
	{
		goto error;
	}
	node1 = getUniqueChildNode(node2, "", "version");
	if (node1 == nullptr)
	{
		goto error;
	}
	version = getNodeContent(node1);
	node2 = getUniqueChildNode(modelnode, "xs", "complexType");
	if (node2 == nullptr)
	{
		goto error;
	}
	node1 = getUniqueChildNode(node2, "xs", "sequence");
	if (node1 == nullptr)
	{
		goto error;
	}
	if (getUniqueChildNode(node2, "xs", "attribute") == nullptr)
	{
		goto error;
	}
	rootnode = getUniqueChildNodeWithAttribute(node1, "xs", "element", "name", "root");
	if (rootnode == nullptr)
	{
		goto error;
	}
	model = std::make_shared<OpenSANDConf::MetaModel>(version);

	// Find enums nodes
	enumsnodes = getChildNodes(schema, "xs", "simpleType");
	if (enumsnodes.size() + 1 != xmlChildElementCount(schema))
	{
		goto error;
	}
	for (auto enode : enumsnodes)
	{
		if (!addEnumTypeFromXSD(model, enode))
		{
			model = nullptr;
			break;
		}
	}

	// Parse root node
	if (model != nullptr && !loadRootFromXSD(model, rootnode))
	{
		model = nullptr;
	}

error:
	nodes.clear();
	xmlFreeDoc(doc);
	xmlCleanupParser();
	xmlMemoryDump();

	return model;
}

bool OpenSANDConf::toXML(std::shared_ptr<OpenSANDConf::DataModel> datamodel, const std::string &filepath)
{
	int code;

	// Initialize XML document
	auto doc = xmlNewDoc(BAD_CAST CONFIGURATION_FILES_VERSION);
	auto node = xmlNewNode(nullptr, BAD_CAST "model");
	xmlDocSetRootElement(doc, node);
	xmlNewProp(node, BAD_CAST "version", BAD_CAST datamodel->getVersion().c_str());

	// Append root schema
	auto root = rootToXML(datamodel->getRoot());
	if (root == nullptr)
	{
		xmlFreeDoc(doc);
		xmlCleanupParser();
		xmlMemoryDump();
		return false;
	}
	xmlAddChild(node, root);

	// Write XSD file
	code = xmlSaveFormatFileEnc(filepath.c_str(), doc, CONFIGURATION_FILES_ENCODING, 1);
	xmlFreeDoc(doc);
	xmlCleanupParser();
	xmlMemoryDump();
	return (code != -1);
}

std::shared_ptr<OpenSANDConf::DataModel> OpenSANDConf::fromXML(std::shared_ptr<OpenSANDConf::MetaModel> model,
															   const std::string &filepath)
{
	xmlDocPtr doc;
	xmlSchemaPtr schema;
	std::shared_ptr<OpenSANDConf::DataModel> datamodel;
	xmlNodePtr node;
	std::string version;

	// Get XSD schema
	std::string tmppath(tmpnam(nullptr));
	if (!toXSD(model, tmppath))
	{
		std::cout << "Error: Translation from XML to XSD failed." << std::endl;
		return nullptr;
	}

	auto sctxt = xmlSchemaNewParserCtxt(tmppath.c_str());
	if (sctxt == nullptr)
	{
		std::cout << "Error: XML Schema parser context creation failed." << std::endl;
		remove(tmppath.c_str());
		return nullptr;
	}

	schema = xmlSchemaParse(sctxt);
	xmlSchemaFreeParserCtxt(sctxt);
	remove(tmppath.c_str());
	if (schema == nullptr)
	{
		std::cout << "Error: XML Schema parsing failed." << std::endl;
		return nullptr;
	}

	// Read and check document
	doc = xmlReadFile(filepath.c_str(), CONFIGURATION_FILES_ENCODING, 0);
	if (doc == nullptr)
	{
		std::cout << "Error: Reading XML file failed." << std::endl;
		xmlSchemaFree(schema);
		return nullptr;
	}

	xmlSchemaValidCtxtPtr vctxt = xmlSchemaNewValidCtxt(schema);
	auto valid = xmlSchemaValidateDoc(vctxt, doc);
	xmlSchemaFreeValidCtxt(vctxt);
	xmlSchemaFree(schema);
	if (valid < 0)
	{
		std::cout << "Error: XML Schema validation failed." << std::endl;
		goto error;
	}

	// Load document
	node = xmlDocGetRootElement(doc);
	if (node == nullptr || xmlStrcmp(node->name, BAD_CAST "model") != 0)
	{
		std::cout << "Error: Root element is missing or not 'model'." << std::endl;
		goto error;
	}

	// Check version
	version = getAttribute(node, "version");
	if (version != model->getVersion())
	{
		std::cout << "Error: Version mismatch. Expected " << model->getVersion() << ", got " << version << "." << std::endl;
		goto error;
	}

	// Initiate datamodel
	datamodel = model->createData();
	if (datamodel == nullptr)
	{
		std::cout << "Error: Data model creation failed." << std::endl;
		goto error;
	}

	// Parse root node
	if (model != nullptr && !loadRootFromXML(datamodel, node))
	{
		std::cout << "Error: Loading root from XML failed." << std::endl;
		datamodel = nullptr;
	}

error:
	xmlFreeDoc(doc);
	xmlCleanupParser();
	xmlMemoryDump();

	return datamodel;
}

//================================================================
// libxml2 extended functions
//================================================================
xmlNodePtr checkUniqueNode(std::vector<xmlNodePtr> &nodes)
{
	if (nodes.size() != 1)
	{
		return nullptr;
	}
	return nodes[0];
}

std::vector<xmlNodePtr> getChildNodes(xmlNodePtr &node, const std::string &ns, const std::string &name)
{
	std::vector<xmlNodePtr> nodes;
	for (xmlNodePtr child = node->children; child != nullptr; child = child->next)
	{
		std::string tmp = child->ns != nullptr ? std::string((char *)child->ns->prefix) : "NULL";
		if (child->type != XML_ELEMENT_NODE || xmlStrcmp(child->name, BAD_CAST name.c_str()) != 0)
		{
			continue;
		}
		if (ns != "" && (child->ns == nullptr || xmlStrcmp(child->ns->prefix, BAD_CAST ns.c_str()) != 0))
		{
			continue;
		}
		nodes.push_back(child);
	}
	return nodes;
}

xmlNodePtr getUniqueChildNode(xmlNodePtr &node, const std::string &ns, const std::string &name)
{
	auto nodes = getChildNodes(node, ns, name);
	return checkUniqueNode(nodes);
}

std::vector<xmlNodePtr> getChildNodesWithAttribute(xmlNodePtr &node,
												   const std::string &ns,
												   const std::string &name,
												   const std::string &attribute,
												   const std::string &value)
{
	std::vector<xmlNodePtr> nodes;
	for (xmlNodePtr child = node->children; child != nullptr; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE || xmlStrcmp(child->name, BAD_CAST name.c_str()) != 0)
		{
			continue;
		}
		if (ns != "" && (child->ns == nullptr || xmlStrcmp(child->ns->prefix, BAD_CAST ns.c_str()) != 0))
		{
			continue;
		}
		auto attr = xmlGetNoNsProp(child, BAD_CAST attribute.c_str());
		if (attr == nullptr || xmlStrcmp(attr, BAD_CAST value.c_str()) != 0)
		{
			continue;
		}
		nodes.push_back(child);
	}
	return nodes;
}

xmlNodePtr getUniqueChildNodeWithAttribute(xmlNodePtr &node,
										   const std::string &ns,
										   const std::string &name,
										   const std::string &attribute,
										   const std::string &value)
{
	auto nodes = getChildNodesWithAttribute(node, ns, name, attribute, value);
	return checkUniqueNode(nodes);
}

std::string getAttribute(xmlNodePtr &node, const std::string &name)
{
	auto tmp = xmlGetProp(node, BAD_CAST name.c_str());
	if (tmp != nullptr)
	{
		try
		{
			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> conv;
			std::wstring wcontent = conv.from_bytes((const char *)tmp);
			std::string content = conv.to_bytes(wcontent);
			xmlFree(tmp);
			return content;
		}
		catch (const std::range_error &e)
		{
			xmlFree(tmp);
		}
	}

	return "";
}

std::string getNodeContent(xmlNodePtr &node)
{
	auto tmp = xmlNodeGetContent(node);
	if (tmp != nullptr)
	{
		try
		{
			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> conv;
			std::wstring wcontent = conv.from_bytes((const char *)tmp);
			std::string content = conv.to_bytes(wcontent);
			xmlFree(tmp);
			return content;
		}
		catch (const std::range_error &e)
		{
			xmlFree(tmp);
		}
	}

	return "";
}

//================================================================
// toXSD functions
//================================================================
bool addXSDChild(xmlNodePtr &node, const std::vector<std::shared_ptr<OpenSANDConf::MetaElement>> elements);

std::vector<xmlNodePtr> enumerationsToXSD(std::shared_ptr<OpenSANDConf::MetaTypesList> types)
{
	std::vector<xmlNodePtr> nodes;
	for (auto element : types->getEnumTypes())
	{
		auto node = xmlNewNode(nullptr, BAD_CAST "xs:simpleType");
		xmlNewProp(node, BAD_CAST "name", BAD_CAST element->getId().c_str());

		auto annot = xmlNewNode(nullptr, BAD_CAST "xs:annotation");
		xmlAddChild(node, annot);

		auto docu = xmlNewNode(nullptr, BAD_CAST "xs:documentation");
		xmlAddChild(annot, docu);
		xmlNewProp(docu, BAD_CAST "xml:lang", BAD_CAST "en");

		auto pname = xmlNewNode(nullptr, BAD_CAST "name");
		xmlNodeSetContent(pname, BAD_CAST element->getName().c_str());
		xmlAddChild(docu, pname);

		auto pdesc = xmlNewNode(nullptr, BAD_CAST "description");
		xmlNodeSetContent(pdesc, BAD_CAST element->getDescription().c_str());
		xmlAddChild(docu, pdesc);

		auto rest = xmlNewNode(nullptr, BAD_CAST "xs:restriction");
		xmlAddChild(node, rest);
		xmlNewProp(rest, BAD_CAST "base", BAD_CAST "xs:string");

		for (auto v : element->getValues())
		{
			auto val = xmlNewNode(nullptr, BAD_CAST "xs:enumeration");
			xmlAddChild(rest, val);
			xmlNewProp(val, BAD_CAST "value", BAD_CAST v.c_str());
		}
		auto val = xmlNewNode(nullptr, BAD_CAST "xs:enumeration");
		xmlAddChild(rest, val);
		xmlNewProp(val, BAD_CAST "value", BAD_CAST "");

		nodes.push_back(node);
	}
	return nodes;
}

xmlNodePtr createXSDNode(const std::string &id,
						 const std::string &type,
						 std::shared_ptr<OpenSANDConf::MetaElement> element)
{
	if (id == "" || type == "")
	{
		return nullptr;
	}

	auto node = xmlNewNode(nullptr, BAD_CAST "xs:element");
	xmlNewProp(node, BAD_CAST "name", BAD_CAST id.c_str());

	auto annot = xmlNewNode(nullptr, BAD_CAST "xs:annotation");
	xmlAddChild(node, annot);

	auto docu = xmlNewNode(nullptr, BAD_CAST "xs:documentation");
	xmlAddChild(annot, docu);
	xmlNewProp(docu, BAD_CAST "xml:lang", BAD_CAST "en");

	// Add properties
	auto ptype = xmlNewNode(nullptr, BAD_CAST "type");
	xmlNodeSetContent(ptype, BAD_CAST type.c_str());
	xmlAddChild(docu, ptype);

	auto pname = xmlNewNode(nullptr, BAD_CAST "name");
	xmlNodeSetContent(pname, BAD_CAST element->getName().c_str());
	xmlAddChild(docu, pname);

	auto pdesc = xmlNewNode(nullptr, BAD_CAST "description");
	xmlNodeSetContent(pdesc, BAD_CAST element->getDescription().c_str());
	xmlAddChild(docu, pdesc);

	auto padv = xmlNewNode(nullptr, BAD_CAST "advanced");
	xmlNodeSetContent(padv, BAD_CAST(element->isAdvanced() ? "true" : "false"));
	xmlAddChild(docu, padv);

	auto prdonly = xmlNewNode(nullptr, BAD_CAST "readonly");
	xmlNodeSetContent(prdonly, BAD_CAST(element->isReadOnly() ? "true" : "false"));
	xmlAddChild(docu, prdonly);

	// Add reference
	auto ref = element->getReferenceTarget();
	auto pref = xmlNewNode(nullptr, BAD_CAST "reference");
	if (ref != nullptr)
	{
		xmlNodeSetContent(pref, BAD_CAST ref->getPath().c_str());
	}
	xmlAddChild(docu, pref);

	auto data = element->getReferenceData();
	auto pexp = xmlNewNode(nullptr, BAD_CAST "expected");
	if (data != nullptr)
	{
		xmlNodeSetContent(pexp, BAD_CAST(data->isSet() ? data->toString().c_str() : ""));
	}
	xmlAddChild(docu, pexp);

	return node;
}

xmlNodePtr elementToXSD(std::shared_ptr<OpenSANDConf::MetaComponent> element)
{
	// Create node
	auto node = createXSDNode(element->getId(), "component", element);
	if (node == nullptr)
	{
		return nullptr;
	}

	// Add child
	if (!addXSDChild(node, element->getItems()))
	{
		xmlFreeNode(node);
		node = nullptr;
	}

	return node;
}

xmlNodePtr elementToXSD(std::shared_ptr<OpenSANDConf::MetaList> element)
{
	// Create list node
	auto node = createXSDNode(element->getId(), "list", element);
	if (node == nullptr)
	{
		return nullptr;
	}
	auto comp = xmlNewNode(nullptr, BAD_CAST "xs:complexType");
	xmlAddChild(node, comp);
	// auto lst = xmlNewNode(nullptr, BAD_CAST "xs:choice");
	auto lst = xmlNewNode(nullptr, BAD_CAST "xs:sequence");
	xmlAddChild(comp, lst);

	// Create pattern node
	auto ptnnode = elementToXSD(element->getPattern());
	if (ptnnode == nullptr)
	{
		xmlFreeNode(node);
		return nullptr;
	}
	xmlSetProp(ptnnode, BAD_CAST "name", BAD_CAST "item");
	xmlNewProp(ptnnode, BAD_CAST "minOccurs", BAD_CAST "0");
	xmlNewProp(ptnnode, BAD_CAST "maxOccurs", BAD_CAST "unbounded");
	xmlAddChild(lst, ptnnode);

	return node;
}

xmlNodePtr elementToXSD(std::shared_ptr<OpenSANDConf::MetaParameter> element)
{
	std::vector<xmlNodePtr> childnodes;

	// Create node
	auto typei = element->getType()->getId();
	auto node = createXSDNode(element->getId(), typei, element);

	// Add additional properties
	childnodes = getChildNodes(node, "", "xs:annotation");
	if (childnodes.size() != 1)
	{
		xmlFreeNode(node);
		return nullptr;
	}
	auto annot = childnodes[0];
	if (annot == nullptr)
	{
		xmlFreeNode(node);
		return nullptr;
	}
	childnodes = getChildNodes(annot, "", "xs:documentation");
	if (childnodes.size() != 1)
	{
		xmlFreeNode(node);
		return nullptr;
	}
	auto docu = childnodes[0];
	if (docu == nullptr)
	{
		xmlFreeNode(node);
		return nullptr;
	}
	auto punit = xmlNewNode(nullptr, BAD_CAST "unit");
	xmlNodeSetContent(punit, BAD_CAST element->getUnit().c_str());
	xmlAddChild(docu, punit);

	// Add additional type description
	std::string xstype = typei;
	if (typei == "string")
	{
		xstype = "xs:string";
	}
	else if (typei == "bool")
	{
		xstype = "xs:boolean";
	}
	else if (typei == "byte")
	{
		xstype = "xs:byte";
	}
	else if (typei == "short")
	{
		xstype = "xs:short";
	}
	else if (typei == "int")
	{
		xstype = "xs:int";
	}
	else if (typei == "long")
	{
		xstype = "xs:long";
	}
	else if (typei == "ubyte")
	{
		xstype = "xs:unsignedByte";
	}
	else if (typei == "ushort")
	{
		xstype = "xs:unsignedShort";
	}
	else if (typei == "uint")
	{
		xstype = "xs:unsignedInt";
	}
	else if (typei == "ulong")
	{
		xstype = "xs:unsignedLong";
	}
	else if (typei == "double" || typei == "float")
	{
		xstype = "xs:decimal";
	}
	xmlNewProp(node, BAD_CAST "type", BAD_CAST xstype.c_str());
	xmlNewProp(node, BAD_CAST "minOccurs", BAD_CAST "0");

	return node;
}

bool addXSDChild(xmlNodePtr &node, const std::vector<std::shared_ptr<OpenSANDConf::MetaElement>> elements)
{
	auto comp = xmlNewNode(nullptr, BAD_CAST "xs:complexType");
	auto seq = xmlNewNode(nullptr, BAD_CAST "xs:sequence");
	xmlAddChild(comp, seq);
	for (auto elt : elements)
	{
		xmlNodePtr eltnode = nullptr;
		if (std::dynamic_pointer_cast<OpenSANDConf::MetaParameter>(elt) != nullptr)
		{
			auto param = std::dynamic_pointer_cast<OpenSANDConf::MetaParameter>(elt);
			eltnode = elementToXSD(param);
		}
		else if (std::dynamic_pointer_cast<OpenSANDConf::MetaComponent>(elt) != nullptr)
		{
			auto comp = std::dynamic_pointer_cast<OpenSANDConf::MetaComponent>(elt);
			eltnode = elementToXSD(comp);
		}
		else if (std::dynamic_pointer_cast<OpenSANDConf::MetaList>(elt) != nullptr)
		{
			auto lst = std::dynamic_pointer_cast<OpenSANDConf::MetaList>(elt);
			eltnode = elementToXSD(lst);
		}
		if (eltnode == nullptr)
		{
			xmlFreeNode(comp);
			return false;
		}
		xmlAddChild(seq, eltnode);
	}
	xmlAddChild(node, comp);
	return true;
}

xmlNodePtr rootToXSD(std::shared_ptr<OpenSANDConf::MetaComponent> element)
{
	// Create node
	auto node = createXSDNode("root", "component", element);

	// Add child
	if (node != nullptr && !addXSDChild(node, element->getItems()))
	{
		xmlFreeNode(node);
		node = nullptr;
	}

	return node;
}

//================================================================
// fromXSD functions
//================================================================
bool addEnumTypeFromXSD(std::shared_ptr<OpenSANDConf::MetaModel> model, xmlNodePtr node)
{
	unsigned int count;
	xmlNodePtr docu, n;
	std::string id, name, desc;
	std::vector<std::string> values;
	std::shared_ptr<OpenSANDConf::MetaEnumType> type = nullptr;

	// Parse data of enumeration type
	id = getAttribute(node, "name");
	if (xmlChildElementCount(node) != 2)
	{
		return false;
	}
	n = getUniqueChildNode(node, "xs", "annotation");
	if (n == nullptr)
	{
		return false;
	}
	docu = getUniqueChildNode(n, "xs", "documentation");
	if (docu == nullptr)
	{
		return false;
	}
	n = getUniqueChildNode(docu, "", "name");
	if (n == nullptr)
	{
		return false;
	}
	name = getNodeContent(n);
	n = getUniqueChildNode(docu, "", "description");
	if (n == nullptr)
	{
		return false;
	}
	desc = getNodeContent(n);
	n = getUniqueChildNodeWithAttribute(node, "xs", "restriction", "base", "xs:string");
	if (n == nullptr)
	{
		return false;
	}
	count = 0;
	for (auto m : getChildNodes(n, "xs", "enumeration"))
	{
		auto v = getAttribute(m, "value");
		++count;
		if (v != "")
		{
			values.push_back(v);
		}
	}
	if (count != xmlChildElementCount(n))
	{
		return false;
	}

	// Add enumeration type to model
	type = model->getTypesDefinition()->addEnumType(id, name, values);
	if (type != nullptr && desc != "")
	{
		type->setDescription(desc);
	}

	return (type != nullptr);
}

bool getElementFromXSD(xmlNodePtr node,
					   std::string &id,
					   std::string &type,
					   std::string &name,
					   std::string &description,
					   bool &advanced,
					   std::string &reference,
					   std::string &expected)
{
	xmlNodePtr docu, child;
	std::string tmp;
	id = getAttribute(node, "name");
	child = getUniqueChildNode(node, "xs", "annotation");
	if (child == nullptr || xmlChildElementCount(child) != 1)
	{
		return false;
	}
	docu = getUniqueChildNode(child, "xs", "documentation");
	if (docu == nullptr)
	{
		return false;
	}
	child = getUniqueChildNode(docu, "", "name");
	if (child == nullptr)
	{
		return false;
	}
	name = getNodeContent(child);
	child = getUniqueChildNode(docu, "", "type");
	if (child == nullptr)
	{
		return false;
	}
	type = getNodeContent(child);
	child = getUniqueChildNode(docu, "", "description");
	if (child == nullptr)
	{
		return false;
	}
	description = getNodeContent(child);
	child = getUniqueChildNode(docu, "", "advanced");
	if (child == nullptr)
	{
		return false;
	}
	tmp = getNodeContent(child);
	std::stringstream ss(tmp);
	ss >> advanced;
	child = getUniqueChildNode(docu, "", "reference");
	if (child == nullptr)
	{
		return false;
	}
	reference = getNodeContent(child);
	child = getUniqueChildNode(docu, "", "expected");
	if (child == nullptr)
	{
		return false;
	}
	expected = getNodeContent(child);

	return true;
}

xmlNodePtr getComponentContentXSDNode(xmlNodePtr node)
{
	xmlNodePtr child;
	child = getUniqueChildNode(node, "xs", "complexType");
	if (child == nullptr || xmlChildElementCount(child) != 1)
	{
		return nullptr;
	}
	return getUniqueChildNode(child, "xs", "sequence");
}

xmlNodePtr getPatternListXSDNode(xmlNodePtr node)
{
	xmlNodePtr child, child2;
	child = getUniqueChildNode(node, "xs", "complexType");
	if (child == nullptr || xmlChildElementCount(child) != 1)
	{
		return nullptr;
	}
	// child2 = getUniqueChildNode(child, "xs", "choice");
	child2 = getUniqueChildNode(child, "xs", "sequence");
	if (child2 == nullptr || xmlChildElementCount(child2) != 1)
	{
		return nullptr;
	}
	return getUniqueChildNode(child2, "xs", "element");
}

bool loadParameterFromXSD(std::shared_ptr<OpenSANDConf::MetaParameter> param, xmlNodePtr node)
{
	xmlNodePtr docu, child;
	std::string tmp;
	child = getUniqueChildNode(node, "xs", "annotation");
	if (child == nullptr || xmlChildElementCount(child) != 1)
	{
		return false;
	}
	docu = getUniqueChildNode(child, "xs", "documentation");
	if (docu == nullptr)
	{
		return false;
	}
	child = getUniqueChildNode(docu, "", "unit");
	if (child == nullptr)
	{
		return false;
	}
	tmp = getNodeContent(child);
	param->setUnit(tmp);
	return true;
}

bool loadComponentFromXSD(std::shared_ptr<OpenSANDConf::MetaComponent> current,
						  xmlNodePtr node,
						  std::shared_ptr<OpenSANDConf::MetaModel> model,
						  std::vector<std::tuple<std::shared_ptr<OpenSANDConf::MetaElement>, std::string, std::string>> &references)
{
	xmlNodePtr sequence;
	std::vector<xmlNodePtr> nodes;
	std::shared_ptr<OpenSANDConf::MetaElement> elt = current;

	sequence = getComponentContentXSDNode(node);
	if (sequence == nullptr)
	{
		return false;
	}
	nodes = getChildNodes(sequence, "xs", "element");
	if (nodes.size() != xmlChildElementCount(sequence))
	{
		return false;
	}
	for (auto child : nodes)
	{
		bool adv;
		std::string id, type, name, desc;
		std::string ref = "";
		std::string exp = "";

		elt = nullptr;
		if (!getElementFromXSD(child, id, type, name, desc, adv, ref, exp))
		{
			break;
		}
		if (type == "component")
		{
			auto comp = current->addComponent(id, name, desc);
			if (comp == nullptr || !loadComponentFromXSD(comp, child, model, references))
			{
				break;
			}
			elt = comp;
		}
		else if (type == "list")
		{
			bool ptn_adv;
			std::string ptn_id, ptn_type, ptn_name, ptn_desc;
			std::string ptn_ref = "";
			std::string ptn_exp = "";
			xmlNodePtr ptn_node;
			ptn_node = getPatternListXSDNode(child);
			if (ptn_node == nullptr || !getElementFromXSD(ptn_node, ptn_id, ptn_type, ptn_name, ptn_desc, ptn_adv, ptn_ref, ptn_exp))
			{
				break;
			}
			auto lst = current->addList(id, name, ptn_name, desc, ptn_desc);
			if (lst == nullptr)
			{
				break;
			}
			auto ptn = lst->getPattern();
			if (ptn == nullptr || !loadComponentFromXSD(ptn, ptn_node, model, references))
			{
				break;
			}
			if (ptn_ref != "")
			{
				auto t = std::make_tuple(ptn, ptn_ref, ptn_exp);
				references.push_back(t);
			}
			elt = lst;
		}
		else
		{
			auto mtype = model->getTypesDefinition()->getType(type);
			if (mtype == nullptr)
			{
				break;
			}
			auto param = current->addParameter(id, name, mtype, desc);
			if (param == nullptr && loadParameterFromXSD(param, child))
			{
				break;
			}
			elt = param;
		}
		if (ref != "")
		{
			auto t = std::make_tuple(elt, ref, exp);
			references.push_back(t);
		}
	}
	return (elt != nullptr);
}

bool loadRootFromXSD(std::shared_ptr<OpenSANDConf::MetaModel> model, xmlNodePtr node)
{
	std::vector<xmlNodePtr> nodes;
	std::vector<std::tuple<std::shared_ptr<OpenSANDConf::MetaElement>, std::string, std::string>> references;

	bool adv;
	std::string id, type, name, desc;
	std::string ref = "";
	std::string exp = "";
	if (!getElementFromXSD(node, id, type, name, desc, adv, ref, exp))
	{
		return false;
	}
	if (type != "component" || id != "root" || name != "Root")
	{
		return false;
	}
	auto root = model->getRoot();
	root->setDescription(desc);
	root->setAdvanced(adv);
	if (ref != "")
	{
		auto t = std::make_tuple(root, ref, exp);
		references.push_back(t);
	}
	if (!loadComponentFromXSD(root, node, model, references))
	{
		return false;
	}
	for (auto t : references)
	{
		auto elt = std::get<0>(t);
		auto ref = std::get<1>(t);
		auto data = std::get<2>(t);

		auto target = std::dynamic_pointer_cast<OpenSANDConf::MetaParameter>(model->getItemByPath(ref));
		if (target == nullptr)
		{
			return false;
		}
		model->setReference(elt, target);
		if (data == "")
		{
			continue;
		}
		auto expected = elt->getReferenceData();
		if (expected == nullptr || !expected->fromString(data))
		{
			return false;
		}
	}
	return true;
}

//================================================================
// toXML functions
//================================================================
xmlNodePtr elementToXML(std::shared_ptr<OpenSANDConf::DataElement> element);

xmlNodePtr componentToXML(std::shared_ptr<OpenSANDConf::DataComponent> element)
{
	// Create node
	auto node = xmlNewNode(nullptr, BAD_CAST element->getId().c_str());
	if (node == nullptr)
	{
		return nullptr;
	}

	// Add child
	for (auto elt : element->getItems())
	{
		auto eltnode = elementToXML(elt);
		if (eltnode == nullptr)
		{
			xmlFreeNode(node);
			node = nullptr;
			break;
		}
		auto content = getNodeContent(eltnode);
		if (content.size() || std::dynamic_pointer_cast<OpenSANDConf::DataParameter>(elt) == nullptr)
		{
			xmlAddChild(node, eltnode);
		}
	}

	return node;
}

xmlNodePtr listToXML(std::shared_ptr<OpenSANDConf::DataList> element)
{
	// Create list node
	auto node = xmlNewNode(nullptr, BAD_CAST element->getId().c_str());
	if (node == nullptr)
	{
		return nullptr;
	}

	// Create items nodes
	for (auto item : element->getItems())
	{
		auto itemnode = elementToXML(item);
		if (itemnode == nullptr)
		{
			xmlFreeNode(node);
			return nullptr;
		}
		auto content = getNodeContent(itemnode);
		if (content.size() || std::dynamic_pointer_cast<OpenSANDConf::DataParameter>(item) == nullptr)
		{
			xmlNodeSetName(itemnode, BAD_CAST "item");
			xmlAddChild(node, itemnode);
		}
	}

	return node;
}

xmlNodePtr parameterToXML(std::shared_ptr<OpenSANDConf::DataParameter> element)
{
	// Create node
	auto node = xmlNewNode(nullptr, BAD_CAST element->getId().c_str());

	// Set value
	auto data = element->getData();
	if (data->isSet())
	{
		xmlNodeSetContent(node, BAD_CAST data->toString().c_str());
	}

	return node;
}

xmlNodePtr elementToXML(std::shared_ptr<OpenSANDConf::DataElement> element)
{
	xmlNodePtr node = nullptr;
	if (std::dynamic_pointer_cast<OpenSANDConf::DataParameter>(element) != nullptr)
	{
		auto param = std::dynamic_pointer_cast<OpenSANDConf::DataParameter>(element);
		node = parameterToXML(param);
	}
	else if (std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(element) != nullptr)
	{
		auto comp = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(element);
		node = componentToXML(comp);
	}
	else if (std::dynamic_pointer_cast<OpenSANDConf::DataList>(element) != nullptr)
	{
		auto lst = std::dynamic_pointer_cast<OpenSANDConf::DataList>(element);
		node = listToXML(lst);
	}
	return node;
}

xmlNodePtr rootToXML(std::shared_ptr<OpenSANDConf::DataComponent> element)
{
	// Create node
	auto node = xmlNewNode(nullptr, BAD_CAST "root");
	if (node == nullptr)
	{
		return nullptr;
	}

	// Add child
	for (auto elt : element->getItems())
	{
		auto eltnode = elementToXML(elt);
		if (eltnode == nullptr)
		{
			xmlFreeNode(node);
			node = nullptr;
			break;
		}
		auto content = getNodeContent(eltnode);
		if (content.size() || std::dynamic_pointer_cast<OpenSANDConf::DataParameter>(elt) == nullptr)
		{
			xmlAddChild(node, eltnode);
		}
	}

	return node;
}

//================================================================
// fromXML functions
//================================================================
bool loadComponentFromXML(std::shared_ptr<OpenSANDConf::DataComponent> current, xmlNodePtr node)
{
	for (auto child = node->children; child != nullptr; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE)
		{
			continue;
		}

		std::string id((char *)(child->name));
		auto element = current->getItem(id);
		if (element == nullptr)
		{
			return false;
		}
		else if (std::dynamic_pointer_cast<OpenSANDConf::DataParameter>(element) != nullptr)
		{
			auto param = std::dynamic_pointer_cast<OpenSANDConf::DataParameter>(element);
			auto content = getNodeContent(child);
			if (content != "" && !param->getData()->fromString(content))
			{
				return false;
			}
		}
		else if (std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(element) != nullptr)
		{
			auto comp = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(element);
			if (!loadComponentFromXML(comp, child))
			{
				return false;
			}
		}
		else if (std::dynamic_pointer_cast<OpenSANDConf::DataList>(element) != nullptr)
		{
			auto lst = std::dynamic_pointer_cast<OpenSANDConf::DataList>(element);
			auto nodes = getChildNodes(child, "", "item");
			for (auto itemnode : nodes)
			{
				auto item = lst->addItem();
				if (item == nullptr || !loadComponentFromXML(item, itemnode))
				{
					return false;
				}
			}
		}
		else
		{
			return false;
		}
	}
	return true;
}

bool loadRootFromXML(std::shared_ptr<OpenSANDConf::DataModel> datamodel, xmlNodePtr node)
{
	auto rootnode = getUniqueChildNode(node, "", "root");
	return rootnode != nullptr && loadComponentFromXML(datamodel->getRoot(), rootnode);
}
