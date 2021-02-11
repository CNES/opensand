#define CATCH_CONFIG_MAIN

#include "catch.hpp"

#include <memory>
#include <initializer_list>
#include <iostream>
#include <fstream>
#include <cstdio>

#include <Configuration.h>

using OpenSANDConf::MetaModel;
using OpenSANDConf::MetaEnumType;
using OpenSANDConf::MetaValueType;
using OpenSANDConf::MetaComponent;
using OpenSANDConf::MetaParameter;
using OpenSANDConf::MetaElement;

using OpenSANDConf::DataModel;
using OpenSANDConf::DataEnumType;
using OpenSANDConf::DataValueType;
using OpenSANDConf::DataValue;
using OpenSANDConf::DataComponent;
using OpenSANDConf::DataParameter;
using OpenSANDConf::DataList;

using OpenSANDConf::toXSD;
using OpenSANDConf::fromXSD;
using OpenSANDConf::toXML;
using OpenSANDConf::fromXML;

TEST_CASE("Model reference tests", "[Model][Reference]")
{
  std::string version = "1.0.0";
	auto model = std::make_shared<MetaModel>(version);
	auto root = model->getRoot();
	REQUIRE(root != nullptr);
	auto types = model->getTypesDefinition();
	REQUIRE(types != nullptr);

	SECTION("Check various type of reference parameter")
	{
		REQUIRE(types->addEnumType("enum1", "Enum 1", {"val1", "val2"}) != nullptr);

		REQUIRE(root->addParameter("b", "Boolean parameter", types->getType("bool")) != nullptr);
		REQUIRE(root->addParameter("d", "Double parameter", types->getType("double")) != nullptr);
		REQUIRE(root->addParameter("i", "Integer parameter", types->getType("int")) != nullptr);
		REQUIRE(root->addParameter("s", "String parameter", types->getType("string")) != nullptr);
		REQUIRE(root->addParameter("e", "Enum parameter", types->getType("enum1")) != nullptr);

		auto cpt = root->addComponent("c", "Component with reference");
		REQUIRE(cpt != nullptr);
		REQUIRE(model->createData() != nullptr);

		SECTION("Check boolean parameter as reference")
		{
			// Configure reference
			auto target = root->getParameter("b");
			REQUIRE(target != nullptr);
			REQUIRE(model->setReference(cpt, target));
			REQUIRE(cpt->getReferenceTarget() == target);
			REQUIRE(cpt->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<bool>>(cpt->getReferenceData());
			REQUIRE(expected != nullptr);

			// Check data creation failed if expected data is not set
			REQUIRE(expected->isSet() == false);
			REQUIRE(model->createData() == nullptr);

			// Check with a value
			REQUIRE(expected->set(true));
			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			REQUIRE(datamodel->getItemByPath(target->getPath()) != nullptr);
			auto datatarget = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(target->getPath()));
			REQUIRE(datatarget != nullptr);
			REQUIRE(datamodel->getItemByPath(cpt->getPath()) != nullptr);
			auto datacpt = std::dynamic_pointer_cast<DataComponent>(datamodel->getItemByPath(cpt->getPath()));
			REQUIRE(datacpt != nullptr);
			auto data = std::dynamic_pointer_cast<DataValue<bool>>(datatarget->getData());
			REQUIRE(data != nullptr);
			REQUIRE(datatarget->checkReference() == true);
			REQUIRE(datacpt->checkReference() == false);
			REQUIRE(data->set(false));
			REQUIRE(datacpt->checkReference() == false);
			REQUIRE(data->set(true));
			REQUIRE(datacpt->checkReference() == true);

			// Check with a second value
			REQUIRE(expected->set(false));
			auto datamodel2 = model->createData();
			REQUIRE(datamodel2 != nullptr);
			REQUIRE(datamodel2->getItemByPath(target->getPath()) != nullptr);
			auto datatarget2 = std::dynamic_pointer_cast<DataParameter>(datamodel2->getItemByPath(target->getPath()));
			REQUIRE(datatarget2 != nullptr);
			REQUIRE(datamodel2->getItemByPath(cpt->getPath()) != nullptr);
			auto datacpt2 = std::dynamic_pointer_cast<DataComponent>(datamodel2->getItemByPath(cpt->getPath()));
			REQUIRE(datacpt2 != nullptr);
			auto data2 = std::dynamic_pointer_cast<DataValue<bool>>(datatarget2->getData());
			REQUIRE(data2 != nullptr);
			REQUIRE(datatarget2->checkReference() == true);
			REQUIRE(datacpt2->checkReference() == false);
			REQUIRE(data2->set(true));
			REQUIRE(datacpt2->checkReference() == false);
			REQUIRE(data2->set(false));
			REQUIRE(datacpt2->checkReference() == true);

			REQUIRE(data->set(false));
			REQUIRE(datacpt->checkReference() == false);
			REQUIRE(data->set(true));
			REQUIRE(datacpt->checkReference() == true);
		}

		SECTION("Check integer parameter as reference")
		{
			// Configure reference
			auto target = root->getParameter("i");
			REQUIRE(target != nullptr);
			REQUIRE(model->setReference(cpt, target));
			REQUIRE(cpt->getReferenceTarget() == target);
			REQUIRE(cpt->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<int>>(cpt->getReferenceData());
			REQUIRE(expected != nullptr);

			// Check data creation failed if expected data is not set
			REQUIRE(expected->isSet() == false);
			REQUIRE(model->createData() == nullptr);

			// Check with a value
			REQUIRE(expected->set(42));
			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			REQUIRE(datamodel->getItemByPath(target->getPath()) != nullptr);
			auto datatarget = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(target->getPath()));
			REQUIRE(datatarget != nullptr);
			REQUIRE(datamodel->getItemByPath(cpt->getPath()) != nullptr);
			auto datacpt = std::dynamic_pointer_cast<DataComponent>(datamodel->getItemByPath(cpt->getPath()));
			REQUIRE(datacpt != nullptr);
			auto data = std::dynamic_pointer_cast<DataValue<int>>(datatarget->getData());
			REQUIRE(data != nullptr);
			REQUIRE(datatarget->checkReference() == true);
			REQUIRE(datacpt->checkReference() == false);
			REQUIRE(data->set(23));
			REQUIRE(datacpt->checkReference() == false);
			REQUIRE(data->set(42));
			REQUIRE(datacpt->checkReference() == true);

			// Check with a second value
			REQUIRE(expected->set(23));
			auto datamodel2 = model->createData();
			REQUIRE(datamodel2 != nullptr);
			REQUIRE(datamodel2->getItemByPath(target->getPath()) != nullptr);
			auto datatarget2 = std::dynamic_pointer_cast<DataParameter>(datamodel2->getItemByPath(target->getPath()));
			REQUIRE(datatarget2 != nullptr);
			REQUIRE(datamodel2->getItemByPath(cpt->getPath()) != nullptr);
			auto datacpt2 = std::dynamic_pointer_cast<DataComponent>(datamodel2->getItemByPath(cpt->getPath()));
			REQUIRE(datacpt2 != nullptr);
			auto data2 = std::dynamic_pointer_cast<DataValue<int>>(datatarget2->getData());
			REQUIRE(data2 != nullptr);
			REQUIRE(datatarget2->checkReference() == true);
			REQUIRE(datacpt2->checkReference() == false);
			REQUIRE(data2->set(42));
			REQUIRE(datacpt2->checkReference() == false);
			REQUIRE(data2->set(23));
			REQUIRE(datacpt2->checkReference() == true);

			REQUIRE(data->set(23));
			REQUIRE(datacpt->checkReference() == false);
			REQUIRE(data->set(42));
			REQUIRE(datacpt->checkReference() == true);
		}

		SECTION("Check double parameter as reference")
		{
			// Configure reference
			auto target = root->getParameter("d");
			REQUIRE(target != nullptr);
			REQUIRE(model->setReference(cpt, target));
			REQUIRE(cpt->getReferenceTarget() == target);
			REQUIRE(cpt->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<double>>(cpt->getReferenceData());
			REQUIRE(expected != nullptr);

			// Check data creation failed if expected data is not set
			REQUIRE(expected->isSet() == false);
			REQUIRE(model->createData() == nullptr);

			// Check with a value
			REQUIRE(expected->set(0.42));
			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			REQUIRE(datamodel->getItemByPath(target->getPath()) != nullptr);
			auto datatarget = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(target->getPath()));
			REQUIRE(datatarget != nullptr);
			REQUIRE(datamodel->getItemByPath(cpt->getPath()) != nullptr);
			auto datacpt = std::dynamic_pointer_cast<DataComponent>(datamodel->getItemByPath(cpt->getPath()));
			REQUIRE(datacpt != nullptr);
			auto data = std::dynamic_pointer_cast<DataValue<double>>(datatarget->getData());
			REQUIRE(data != nullptr);
			REQUIRE(datatarget->checkReference() == true);
			REQUIRE(datacpt->checkReference() == false);
			REQUIRE(data->set(0.23));
			REQUIRE(datacpt->checkReference() == false);
			REQUIRE(data->set(0.42));
			REQUIRE(datacpt->checkReference() == true);

			// Check with a second value
			REQUIRE(expected->set(0.23));
			auto datamodel2 = model->createData();
			REQUIRE(datamodel2 != nullptr);
			REQUIRE(datamodel2->getItemByPath(target->getPath()) != nullptr);
			auto datatarget2 = std::dynamic_pointer_cast<DataParameter>(datamodel2->getItemByPath(target->getPath()));
			REQUIRE(datatarget2 != nullptr);
			REQUIRE(datamodel2->getItemByPath(cpt->getPath()) != nullptr);
			auto datacpt2 = std::dynamic_pointer_cast<DataComponent>(datamodel2->getItemByPath(cpt->getPath()));
			REQUIRE(datacpt2 != nullptr);
			auto data2 = std::dynamic_pointer_cast<DataValue<double>>(datatarget2->getData());
			REQUIRE(data2 != nullptr);
			REQUIRE(datatarget2->checkReference() == true);
			REQUIRE(datacpt2->checkReference() == false);
			REQUIRE(data2->set(0.42));
			REQUIRE(datacpt2->checkReference() == false);
			REQUIRE(data2->set(0.23));
			REQUIRE(datacpt2->checkReference() == true);

			REQUIRE(data->set(0.23));
			REQUIRE(datacpt->checkReference() == false);
			REQUIRE(data->set(0.42));
			REQUIRE(datacpt->checkReference() == true);
		}

		SECTION("Check string parameter as reference")
		{
			// Configure reference
			auto target = root->getParameter("s");
			REQUIRE(target != nullptr);
			REQUIRE(model->setReference(cpt, target));
			REQUIRE(cpt->getReferenceTarget() == target);
			REQUIRE(cpt->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<std::string>>(cpt->getReferenceData());
			REQUIRE(expected != nullptr);

			// Check data creation failed if expected data is not set
			REQUIRE(expected->isSet() == false);
			REQUIRE(model->createData() == nullptr);

			// Check with a value
			REQUIRE(expected->set("test"));
			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			REQUIRE(datamodel->getItemByPath(target->getPath()) != nullptr);
			auto datatarget = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(target->getPath()));
			REQUIRE(datatarget != nullptr);
			REQUIRE(datamodel->getItemByPath(cpt->getPath()) != nullptr);
			auto datacpt = std::dynamic_pointer_cast<DataComponent>(datamodel->getItemByPath(cpt->getPath()));
			REQUIRE(datacpt != nullptr);
			auto data = std::dynamic_pointer_cast<DataValue<std::string>>(datatarget->getData());
			REQUIRE(data != nullptr);
			REQUIRE(datatarget->checkReference() == true);
			REQUIRE(datacpt->checkReference() == false);
			REQUIRE(data->set("invalid"));
			REQUIRE(datacpt->checkReference() == false);
			REQUIRE(data->set("test"));
			REQUIRE(datacpt->checkReference() == true);

			// Check with a second value
			REQUIRE(expected->set("test2"));
			auto datamodel2 = model->createData();
			REQUIRE(datamodel2 != nullptr);
			REQUIRE(datamodel2->getItemByPath(target->getPath()) != nullptr);
			auto datatarget2 = std::dynamic_pointer_cast<DataParameter>(datamodel2->getItemByPath(target->getPath()));
			REQUIRE(datatarget2 != nullptr);
			REQUIRE(datamodel2->getItemByPath(cpt->getPath()) != nullptr);
			auto datacpt2 = std::dynamic_pointer_cast<DataComponent>(datamodel2->getItemByPath(cpt->getPath()));
			REQUIRE(datacpt2 != nullptr);
			auto data2 = std::dynamic_pointer_cast<DataValue<std::string>>(datatarget2->getData());
			REQUIRE(data2 != nullptr);
			REQUIRE(datatarget2->checkReference() == true);
			REQUIRE(datacpt2->checkReference() == false);
			REQUIRE(data2->set("test"));
			REQUIRE(datacpt2->checkReference() == false);
			REQUIRE(data2->set("test2"));
			REQUIRE(datacpt2->checkReference() == true);

			REQUIRE(data->set("test2"));
			REQUIRE(datacpt->checkReference() == false);
			REQUIRE(data->set("test"));
			REQUIRE(datacpt->checkReference() == true);
		}

		SECTION("Check enumeration parameter as reference")
		{
			// Configure reference
			auto target = root->getParameter("e");
			REQUIRE(target != nullptr);
			REQUIRE(model->setReference(cpt, target));
			REQUIRE(cpt->getReferenceTarget() == target);
			REQUIRE(cpt->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<std::string>>(cpt->getReferenceData());
			REQUIRE(expected != nullptr);

			// Check data creation failed if expected data is not set
			REQUIRE(expected->isSet() == false);
			REQUIRE(model->createData() == nullptr);
			REQUIRE(expected->set("invalid") == false);

			// Check with a value
			REQUIRE(expected->set("val1"));
			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			REQUIRE(datamodel->getItemByPath(target->getPath()) != nullptr);
			auto datatarget = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(target->getPath()));
			REQUIRE(datatarget != nullptr);
			REQUIRE(datamodel->getItemByPath(cpt->getPath()) != nullptr);
			auto datacpt = std::dynamic_pointer_cast<DataComponent>(datamodel->getItemByPath(cpt->getPath()));
			REQUIRE(datacpt != nullptr);
			auto data = std::dynamic_pointer_cast<DataValue<std::string>>(datatarget->getData());
			REQUIRE(data != nullptr);
			REQUIRE(datatarget->checkReference() == true);
			REQUIRE(datacpt->checkReference() == false);
			REQUIRE(data->set("val2"));
			REQUIRE(datacpt->checkReference() == false);
			REQUIRE(data->set("val1"));
			REQUIRE(datacpt->checkReference() == true);

			// Check with a second value
			REQUIRE(expected->set("val2"));
			auto datamodel2 = model->createData();
			REQUIRE(datamodel2 != nullptr);
			REQUIRE(datamodel2->getItemByPath(target->getPath()) != nullptr);
			auto datatarget2 = std::dynamic_pointer_cast<DataParameter>(datamodel2->getItemByPath(target->getPath()));
			REQUIRE(datatarget2 != nullptr);
			REQUIRE(datamodel2->getItemByPath(cpt->getPath()) != nullptr);
			auto datacpt2 = std::dynamic_pointer_cast<DataComponent>(datamodel2->getItemByPath(cpt->getPath()));
			REQUIRE(datacpt2 != nullptr);
			auto data2 = std::dynamic_pointer_cast<DataValue<std::string>>(datatarget2->getData());
			REQUIRE(data2 != nullptr);
			REQUIRE(datatarget2->checkReference() == true);
			REQUIRE(datacpt2->checkReference() == false);
			REQUIRE(data2->set("val1"));
			REQUIRE(datacpt2->checkReference() == false);
			REQUIRE(data2->set("val2"));
			REQUIRE(datacpt2->checkReference() == true);

			REQUIRE(data->set("val2"));
			REQUIRE(datacpt->checkReference() == false);
			REQUIRE(data->set("val1"));
			REQUIRE(datacpt->checkReference() == true);
		}
	}

	SECTION("Check various referenced element")
	{
		REQUIRE(types->addEnumType("enum1", "Enum 1", {"val1", "val2"}) != nullptr);

		REQUIRE(root->addParameter("e", "Enum parameter (level 1)", types->getType("enum1")) != nullptr);
		REQUIRE(root->addParameter("s", "String parameter (level 1)", types->getType("string")) != nullptr);
		auto cpt = root->addComponent("c", "Component (level 1)");
		REQUIRE(cpt != nullptr);
		REQUIRE(cpt->addParameter("e2", "Enum parameter (level 2)", types->getType("enum1")) != nullptr);
		REQUIRE(cpt->addParameter("s2", "String parameter (level 2)", types->getType("string")) != nullptr);
		auto cpt2 = cpt->addComponent("c2", "Component (level 2)");
		REQUIRE(cpt2 != nullptr);
		REQUIRE(cpt2->addParameter("e3", "Enum parameter (level 3)", types->getType("enum1")) != nullptr);
		REQUIRE(cpt2->addParameter("s3", "String parameter (level 3)", types->getType("string")) != nullptr);
		auto lst3 = cpt2->addList("l3", "List (level 3)", "Item");
		REQUIRE(lst3 != nullptr);
		auto ptn3 = lst3->getPattern();
		REQUIRE(ptn3 != nullptr);
		REQUIRE(ptn3->addParameter("e4", "Enum parameter (level 4)", types->getType("enum1")) != nullptr);
		REQUIRE(ptn3->addParameter("s4", "String parameter (level 4)", types->getType("string")) != nullptr);
		auto lst3b = cpt2->addList("l3b", "List 2 (level 3)", "Item");
		REQUIRE(lst3b != nullptr);
		auto ptn3b = lst3b->getPattern();
		REQUIRE(ptn3b != nullptr);
		REQUIRE(ptn3b->addParameter("e4", "Enum parameter (level 4)", types->getType("enum1")) != nullptr);
		REQUIRE(ptn3b->addParameter("s4", "String parameter (level 4)", types->getType("string")) != nullptr);
		auto lst4 = ptn3->addList("l4", "List (level 4)", "Item");
		REQUIRE(lst4 != nullptr);
		auto ptn4 = lst4->getPattern();
		REQUIRE(ptn4 != nullptr);
		REQUIRE(ptn4->addParameter("e5", "Enum parameter (level 5)", types->getType("enum1")) != nullptr);
		REQUIRE(ptn4->addParameter("s5", "String parameter (level 5)", types->getType("string")) != nullptr);
		auto cpt5 = ptn4->addComponent("c5", "Component (level 5)");
		REQUIRE(cpt5 != nullptr);
		REQUIRE(cpt5->addParameter("e6", "Enum parameter (level 6)", types->getType("enum1")) != nullptr);
		REQUIRE(cpt5->addParameter("s6", "String parameter (level 6)", types->getType("string")) != nullptr);

		REQUIRE(model->getItemByPath("/c/c2/l3") != nullptr);
		REQUIRE(model->getItemByPath("/c/c2/l3/*") != nullptr);
		REQUIRE(model->getItemByPath("/c/c2/l3/*/l4") != nullptr);

		REQUIRE(model->getItemByPath(lst3->getPath()) != nullptr);
		REQUIRE(model->getItemByPath(ptn3->getPath()) != nullptr);
		REQUIRE(model->getItemByPath(lst4->getPath()) != nullptr);

		SECTION("Reference composite element (target level <= element level)")
		{
			auto target = root->getParameter("e");
			auto element = cpt2->getParameter("s3");
			auto targetpath = target->getPath();
			auto elementpath = element->getPath();
      std::string elementpath2 = "";
			auto res2 = false;

			// Configure reference
			REQUIRE(target != nullptr);
			REQUIRE(element != nullptr);
			REQUIRE(model->setReference(element, target));
			REQUIRE(element->getReferenceTarget() == target);
			REQUIRE(element->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<std::string>>(element->getReferenceData());
			REQUIRE(expected != nullptr);

			// Check data creation failed if expected data is not set
			REQUIRE(expected->isSet() == false);
			REQUIRE(model->createData() == nullptr);
			REQUIRE(expected->set("invalid") == false);

			// Check with a value
			REQUIRE(expected->set("val1"));
			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			auto datalst3 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3"));
			REQUIRE(datalst3 != nullptr);
			REQUIRE(datalst3->addItem() != nullptr);
			REQUIRE(datalst3->addItem() != nullptr);
			auto datalst3b = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3b"));
			REQUIRE(datalst3b != nullptr);
			REQUIRE(datalst3b->addItem() != nullptr);
			auto datalst4_0 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3/0/l4"));
			REQUIRE(datalst4_0 != nullptr);
			REQUIRE(datalst4_0->addItem() != nullptr);
			REQUIRE(datalst4_0->addItem() != nullptr);
			auto datalst4_1 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3/1/l4"));
			REQUIRE(datalst4_1 != nullptr);
			REQUIRE(datalst4_1->addItem() != nullptr);
			REQUIRE(datalst4_1->addItem() != nullptr);
			REQUIRE(datamodel->getItemByPath(targetpath) != nullptr);
			auto datatarget = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(targetpath));
			REQUIRE(datatarget != nullptr);
			auto dataelement = datamodel->getItemByPath(elementpath);
			REQUIRE(dataelement != nullptr);
			auto data = std::dynamic_pointer_cast<DataValue<std::string>>(datatarget->getData());
			REQUIRE(data != nullptr);
			REQUIRE(datatarget->checkReference() == true);
			REQUIRE(dataelement->checkReference() == false);
			REQUIRE(data->set("val2"));
			REQUIRE(dataelement->checkReference() == false);
			REQUIRE(data->set("val1"));
			REQUIRE(dataelement->checkReference() == true);
			if(!elementpath2.empty())
			{
				auto dataelement2 = datamodel->getItemByPath(elementpath2);
				REQUIRE(dataelement2 != nullptr);
				REQUIRE(dataelement2->checkReference() == res2);
			}
		}

		SECTION("Reference composite element (target level > element level)")
		{
			auto target = cpt2->getParameter("e3");
			auto element = root->getParameter("s");
			auto targetpath = target->getPath();
			auto elementpath = element->getPath();
      std::string elementpath2 = "";
			auto res2 = false;

			// Configure reference
			REQUIRE(target != nullptr);
			REQUIRE(element != nullptr);
			REQUIRE(model->setReference(element, target));
			REQUIRE(element->getReferenceTarget() == target);
			REQUIRE(element->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<std::string>>(element->getReferenceData());
			REQUIRE(expected != nullptr);

			// Check data creation failed if expected data is not set
			REQUIRE(expected->isSet() == false);
			REQUIRE(model->createData() == nullptr);
			REQUIRE(expected->set("invalid") == false);

			// Check with a value
			REQUIRE(expected->set("val1"));
			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			auto datalst3 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3"));
			REQUIRE(datalst3 != nullptr);
			REQUIRE(datalst3->addItem() != nullptr);
			REQUIRE(datalst3->addItem() != nullptr);
			auto datalst3b = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3b"));
			REQUIRE(datalst3b != nullptr);
			REQUIRE(datalst3b->addItem() != nullptr);
			auto datalst4_0 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3/0/l4"));
			REQUIRE(datalst4_0 != nullptr);
			REQUIRE(datalst4_0->addItem() != nullptr);
			REQUIRE(datalst4_0->addItem() != nullptr);
			auto datalst4_1 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3/1/l4"));
			REQUIRE(datalst4_1 != nullptr);
			REQUIRE(datalst4_1->addItem() != nullptr);
			REQUIRE(datalst4_1->addItem() != nullptr);
			REQUIRE(datamodel->getItemByPath(targetpath) != nullptr);
			auto datatarget = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(targetpath));
			REQUIRE(datatarget != nullptr);
			auto dataelement = datamodel->getItemByPath(elementpath);
			REQUIRE(dataelement != nullptr);
			auto data = std::dynamic_pointer_cast<DataValue<std::string>>(datatarget->getData());
			REQUIRE(data != nullptr);
			REQUIRE(datatarget->checkReference() == true);
			REQUIRE(dataelement->checkReference() == false);
			REQUIRE(data->set("val2"));
			REQUIRE(dataelement->checkReference() == false);
			REQUIRE(data->set("val1"));
			REQUIRE(dataelement->checkReference() == true);
			if(!elementpath2.empty())
			{
				auto dataelement2 = datamodel->getItemByPath(elementpath2);
				REQUIRE(dataelement2 != nullptr);
				REQUIRE(dataelement2->checkReference() == res2);
			}
		}

		SECTION("Reference composite element (element in list pattern)")
		{
			auto target = root->getParameter("e");
			auto element = ptn3->getParameter("s4");
			auto targetpath = target->getPath();
			auto elementpath = "/c/c2/l3/1/s4";
      std::string elementpath2 = "/c/c2/l3/0/s4";
			auto res2 = true;

			// Configure reference
			REQUIRE(target != nullptr);
			REQUIRE(element != nullptr);
			REQUIRE(model->setReference(element, target));
			REQUIRE(element->getReferenceTarget() == target);
			REQUIRE(element->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<std::string>>(element->getReferenceData());
			REQUIRE(expected != nullptr);

			// Check data creation failed if expected data is not set
			REQUIRE(expected->isSet() == false);
			REQUIRE(model->createData() == nullptr);
			REQUIRE(expected->set("invalid") == false);

			// Check with a value
			REQUIRE(expected->set("val1"));
			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			auto datalst3 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3"));
			REQUIRE(datalst3 != nullptr);
			REQUIRE(datalst3->addItem() != nullptr);
			REQUIRE(datalst3->addItem() != nullptr);
			auto datalst3b = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3b"));
			REQUIRE(datalst3b != nullptr);
			REQUIRE(datalst3b->addItem() != nullptr);
			auto datalst4_0 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3/0/l4"));
			REQUIRE(datalst4_0 != nullptr);
			REQUIRE(datalst4_0->addItem() != nullptr);
			REQUIRE(datalst4_0->addItem() != nullptr);
			auto datalst4_1 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3/1/l4"));
			REQUIRE(datalst4_1 != nullptr);
			REQUIRE(datalst4_1->addItem() != nullptr);
			REQUIRE(datalst4_1->addItem() != nullptr);
			REQUIRE(datamodel->getItemByPath(targetpath) != nullptr);
			auto datatarget = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(targetpath));
			REQUIRE(datatarget != nullptr);
			auto dataelement = datamodel->getItemByPath(elementpath);
			REQUIRE(dataelement != nullptr);
			auto data = std::dynamic_pointer_cast<DataValue<std::string>>(datatarget->getData());
			REQUIRE(data != nullptr);
			REQUIRE(datatarget->checkReference() == true);
			REQUIRE(dataelement->checkReference() == false);
			REQUIRE(data->set("val2"));
			REQUIRE(dataelement->checkReference() == false);
			REQUIRE(data->set("val1"));
			REQUIRE(dataelement->checkReference() == true);
			if(!elementpath2.empty())
			{
				auto dataelement2 = datamodel->getItemByPath(elementpath2);
				REQUIRE(dataelement2 != nullptr);
				REQUIRE(dataelement2->checkReference() == res2);
			}
		}

		SECTION("Reference composite element (target in list pattern)")
		{
			auto target = ptn3->getParameter("e4");
			auto element = root->getParameter("s");

			// Configure reference
			REQUIRE(target != nullptr);
			REQUIRE(element != nullptr);
			REQUIRE(model->setReference(element, target) == false);
		}

		SECTION("Reference composite element (target and element in the same list pattern)")
		{
			auto target = ptn3->getParameter("e4");
			auto element = ptn3->getParameter("s4");
			auto targetpath = "/c/c2/l3/1/e4";
			auto elementpath = "/c/c2/l3/1/s4";
      std::string elementpath2 = "/c/c2/l3/0/s4";
			auto res2 = false;

			// Configure reference
			REQUIRE(target != nullptr);
			REQUIRE(element != nullptr);
			REQUIRE(model->setReference(element, target));
			REQUIRE(element->getReferenceTarget() == target);
			REQUIRE(element->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<std::string>>(element->getReferenceData());
			REQUIRE(expected != nullptr);

			// Check data creation failed if expected data is not set
			REQUIRE(expected->isSet() == false);
			REQUIRE(model->createData() == nullptr);
			REQUIRE(expected->set("invalid") == false);

			// Check with a value
			REQUIRE(expected->set("val1"));
			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			auto datalst3 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3"));
			REQUIRE(datalst3 != nullptr);
			REQUIRE(datalst3->addItem() != nullptr);
			REQUIRE(datalst3->addItem() != nullptr);
			auto datalst3b = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3b"));
			REQUIRE(datalst3b != nullptr);
			REQUIRE(datalst3b->addItem() != nullptr);
			auto datalst4_0 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3/0/l4"));
			REQUIRE(datalst4_0 != nullptr);
			REQUIRE(datalst4_0->addItem() != nullptr);
			REQUIRE(datalst4_0->addItem() != nullptr);
			auto datalst4_1 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3/1/l4"));
			REQUIRE(datalst4_1 != nullptr);
			REQUIRE(datalst4_1->addItem() != nullptr);
			REQUIRE(datalst4_1->addItem() != nullptr);
			REQUIRE(datamodel->getItemByPath(targetpath) != nullptr);
			auto datatarget = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(targetpath));
			REQUIRE(datatarget != nullptr);
			auto dataelement = datamodel->getItemByPath(elementpath);
			REQUIRE(dataelement != nullptr);
			auto data = std::dynamic_pointer_cast<DataValue<std::string>>(datatarget->getData());
			REQUIRE(data != nullptr);
			REQUIRE(datatarget->checkReference() == true);
			REQUIRE(dataelement->checkReference() == false);
			REQUIRE(data->set("val2"));
			REQUIRE(dataelement->checkReference() == false);
			REQUIRE(data->set("val1"));
			REQUIRE(dataelement->checkReference() == true);
			if(!elementpath2.empty())
			{
				auto dataelement2 = datamodel->getItemByPath(elementpath2);
				REQUIRE(dataelement2 != nullptr);
				REQUIRE(dataelement2->checkReference() == res2);
			}
		}

		SECTION("Reference composite element (target and element in the same list pattern which is itself in a list pattern)")
		{
			auto target = ptn4->getParameter("e5");
			auto element = ptn4->getParameter("s5");
			auto targetpath = "/c/c2/l3/1/l4/0/e5";
			auto elementpath = "/c/c2/l3/1/l4/0/s5";
      std::string elementpath2 = "/c/c2/l3/1/l4/1/s5";
			auto res2 = false;

			// Configure reference
			REQUIRE(target != nullptr);
			REQUIRE(element != nullptr);
			REQUIRE(model->setReference(element, target));
			REQUIRE(element->getReferenceTarget() == target);
			REQUIRE(element->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<std::string>>(element->getReferenceData());
			REQUIRE(expected != nullptr);

			// Check data creation failed if expected data is not set
			REQUIRE(expected->isSet() == false);
			REQUIRE(model->createData() == nullptr);
			REQUIRE(expected->set("invalid") == false);

			// Check with a value
			REQUIRE(expected->set("val1"));
			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			auto datalst3 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3"));
			REQUIRE(datalst3 != nullptr);
			REQUIRE(datalst3->addItem() != nullptr);
			REQUIRE(datalst3->addItem() != nullptr);
			auto datalst3b = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3b"));
			REQUIRE(datalst3b != nullptr);
			REQUIRE(datalst3b->addItem() != nullptr);
			auto datalst4_0 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3/0/l4"));
			REQUIRE(datalst4_0 != nullptr);
			REQUIRE(datalst4_0->addItem() != nullptr);
			REQUIRE(datalst4_0->addItem() != nullptr);
			auto datalst4_1 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3/1/l4"));
			REQUIRE(datalst4_1 != nullptr);
			REQUIRE(datalst4_1->addItem() != nullptr);
			REQUIRE(datalst4_1->addItem() != nullptr);
			REQUIRE(datamodel->getItemByPath(targetpath) != nullptr);
			auto datatarget = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(targetpath));
			REQUIRE(datatarget != nullptr);
			auto dataelement = datamodel->getItemByPath(elementpath);
			REQUIRE(dataelement != nullptr);
			auto data = std::dynamic_pointer_cast<DataValue<std::string>>(datatarget->getData());
			REQUIRE(data != nullptr);
			REQUIRE(datatarget->checkReference() == true);
			REQUIRE(dataelement->checkReference() == false);
			REQUIRE(data->set("val2"));
			REQUIRE(dataelement->checkReference() == false);
			REQUIRE(data->set("val1"));
			REQUIRE(dataelement->checkReference() == true);
			if(!elementpath2.empty())
			{
				auto dataelement2 = datamodel->getItemByPath(elementpath2);
				REQUIRE(dataelement2 != nullptr);
				REQUIRE(dataelement2->checkReference() == res2);
			}
		}

		SECTION("Reference composite element (target and element in the different list pattern at the same level)")
		{
			auto target = ptn3->getParameter("e4");
			auto element = ptn3b->getParameter("s4");

			// Configure reference
			REQUIRE(target != nullptr);
			REQUIRE(element != nullptr);
			REQUIRE(model->setReference(element, target) == false);
		}

		SECTION("Reference composite element (target in a list and element in a list which is in the pattern of the same list as the target)")
		{
			auto target = ptn3->getParameter("e4");
			auto element = ptn4->getParameter("s5");
			auto targetpath = "/c/c2/l3/1/e4";
			auto elementpath = "/c/c2/l3/1/l4/0/s5";
      std::string elementpath2 = "/c/c2/l3/1/l4/1/s5";
			auto res2 = true;

			// Configure reference
			REQUIRE(target != nullptr);
			REQUIRE(element != nullptr);
			REQUIRE(model->setReference(element, target));
			REQUIRE(element->getReferenceTarget() == target);
			REQUIRE(element->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<std::string>>(element->getReferenceData());
			REQUIRE(expected != nullptr);

			// Check data creation failed if expected data is not set
			REQUIRE(expected->isSet() == false);
			REQUIRE(model->createData() == nullptr);
			REQUIRE(expected->set("invalid") == false);

			// Check with a value
			REQUIRE(expected->set("val1"));
			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			auto datalst3 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3"));
			REQUIRE(datalst3 != nullptr);
			REQUIRE(datalst3->addItem() != nullptr);
			REQUIRE(datalst3->addItem() != nullptr);
			auto datalst3b = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3b"));
			REQUIRE(datalst3b != nullptr);
			REQUIRE(datalst3b->addItem() != nullptr);
			auto datalst4_0 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3/0/l4"));
			REQUIRE(datalst4_0 != nullptr);
			REQUIRE(datalst4_0->addItem() != nullptr);
			REQUIRE(datalst4_0->addItem() != nullptr);
			auto datalst4_1 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/c2/l3/1/l4"));
			REQUIRE(datalst4_1 != nullptr);
			REQUIRE(datalst4_1->addItem() != nullptr);
			REQUIRE(datalst4_1->addItem() != nullptr);
			REQUIRE(datamodel->getItemByPath(targetpath) != nullptr);
			auto datatarget = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(targetpath));
			REQUIRE(datatarget != nullptr);
			auto dataelement = datamodel->getItemByPath(elementpath);
			REQUIRE(dataelement != nullptr);
			auto data = std::dynamic_pointer_cast<DataValue<std::string>>(datatarget->getData());
			REQUIRE(data != nullptr);
			REQUIRE(datatarget->checkReference() == true);
			REQUIRE(dataelement->checkReference() == false);
			REQUIRE(data->set("val2"));
			REQUIRE(dataelement->checkReference() == false);
			REQUIRE(data->set("val1"));
			REQUIRE(dataelement->checkReference() == true);
			if(!elementpath2.empty())
			{
				auto dataelement2 = datamodel->getItemByPath(elementpath2);
				REQUIRE(dataelement2 != nullptr);
				REQUIRE(dataelement2->checkReference() == res2);
			}
		}

		SECTION("Reference composite element (element in a list and target in a list which is in the pattern of the same list as the element)")
		{
			auto target = ptn4->getParameter("e5");
			auto element = ptn3->getParameter("s4");

			// Configure reference
			REQUIRE(target != nullptr);
			REQUIRE(element != nullptr);
			REQUIRE(model->setReference(element, target) == false);
		}
	}

	SECTION("Check validity of datamodel")
	{
		REQUIRE(types->addEnumType("enum1", "Enum 1", {"val1", "val2"}) != nullptr);

		REQUIRE(root->addParameter("e", "Enum parameter (level 1)", types->getType("enum1")) != nullptr);
		REQUIRE(root->addParameter("s", "String parameter (level 1)", types->getType("string")) != nullptr);
		auto cpt = root->addComponent("c", "Component (level 1)");
		REQUIRE(cpt != nullptr);
		REQUIRE(cpt->addParameter("e2", "Enum parameter (level 2)", types->getType("enum1")) != nullptr);
		REQUIRE(cpt->addParameter("s2", "String parameter (level 2)", types->getType("string")) != nullptr);
		auto lst2 = cpt->addList("l2", "List (level 2)", "Item");
		REQUIRE(lst2 != nullptr);
		auto ptn2 = lst2->getPattern();
		REQUIRE(ptn2 != nullptr);
		REQUIRE(ptn2->addParameter("e3", "Enum parameter (level 3)", types->getType("enum1")) != nullptr);
		REQUIRE(ptn2->addParameter("s3", "String parameter (level 3)", types->getType("string")) != nullptr);

		SECTION("Check validity of a datamodel with no reference")
		{
      std::vector<std::string> value_paths = {
				"/e",
				"/s",
				"/c/e2",
				"/c/s2",
				"/c/l2/0/e3",
				"/c/l2/0/s3",
				"/c/l2/1/e3",
				"/c/l2/1/s3",
			};
      std::vector<std::string> direct_referenced_paths = {
			};
      std::vector<std::string> indirect_referenced_paths = {
			};
      std::shared_ptr<MetaParameter> target = nullptr;
      std::shared_ptr<MetaElement> element = nullptr;
      std::string targetpath = "";

			// Add reference
			if(target != nullptr)
			{
				REQUIRE(element != nullptr);
				REQUIRE(model->setReference(element, target));
				REQUIRE(element->getReferenceTarget() == target);
				REQUIRE(element->getReferenceData() != nullptr);
				auto expected = std::dynamic_pointer_cast<DataValue<std::string>>(element->getReferenceData());
				REQUIRE(expected != nullptr);
				REQUIRE(expected->isSet() == false);
				REQUIRE(model->createData() == nullptr);
				REQUIRE(expected->set("val1") == true);
			}

			// Create datamodel
			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			auto datalst2 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/l2"));
			REQUIRE(datalst2 != nullptr);
			REQUIRE(datalst2->addItem() != nullptr);
			REQUIRE(datalst2->addItem() != nullptr);
			REQUIRE(datamodel->validate() == false);

			// Fill datamodel
			for(auto path: value_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
				REQUIRE(data->set("val2"));
			}
			for(auto path: direct_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == false);
			}
			for(auto path: indirect_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
			}
			REQUIRE(datamodel->validate() == true);
			if(!targetpath.empty())
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(targetpath));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->set("val1"));
				REQUIRE(datamodel->validate() == false);
			}
		}

		SECTION("Check validity of a datamodel with a lower element with reference")
		{
      std::vector<std::string> value_paths = {
				"/e",
				"/s",
				"/c/e2",
				"/c/l2/0/e3",
				"/c/l2/0/s3",
				"/c/l2/1/e3",
				"/c/l2/1/s3",
			};
      std::vector<std::string> direct_referenced_paths = {
				"/c/s2",
			};
      std::vector<std::string> indirect_referenced_paths = {
			};
      std::shared_ptr<MetaParameter> target = std::dynamic_pointer_cast<MetaParameter>(model->getItemByPath("/e"));
      std::shared_ptr<MetaElement> element = model->getItemByPath("/c/s2");
      std::string targetpath = target->getPath();

			// Add reference
			if(target != nullptr)
			{
				REQUIRE(element != nullptr);
				REQUIRE(model->setReference(element, target));
				REQUIRE(element->getReferenceTarget() == target);
				REQUIRE(element->getReferenceData() != nullptr);
				auto expected = std::dynamic_pointer_cast<DataValue<std::string>>(element->getReferenceData());
				REQUIRE(expected != nullptr);
				REQUIRE(expected->isSet() == false);
				REQUIRE(model->createData() == nullptr);
				REQUIRE(expected->set("val1") == true);
			}

			// Create datamodel
			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			auto datalst2 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/l2"));
			REQUIRE(datalst2 != nullptr);
			REQUIRE(datalst2->addItem() != nullptr);
			REQUIRE(datalst2->addItem() != nullptr);
			REQUIRE(datamodel->validate() == false);

			// Fill datamodel
			for(auto path: value_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
				REQUIRE(data->set("val2"));
			}
			for(auto path: direct_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == false);
			}
			for(auto path: indirect_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
			}
			REQUIRE(datamodel->validate() == true);
			if(!targetpath.empty())
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(targetpath));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->set("val1"));
				REQUIRE(datamodel->validate() == false);
			}
		}

		SECTION("Check validity of a datamodel with a upper element with reference")
		{
      std::vector<std::string> value_paths = {
				"/e",
				"/s",
				"/c/e2",
				"/c/s2",
			};
      std::vector<std::string> direct_referenced_paths = {
			};
      std::vector<std::string> indirect_referenced_paths = {
				"/c/l2/0/e3",
				"/c/l2/0/s3",
				"/c/l2/1/e3",
				"/c/l2/1/s3",
			};
      std::shared_ptr<MetaParameter> target = std::dynamic_pointer_cast<MetaParameter>(model->getItemByPath("/e"));
      std::shared_ptr<MetaElement> element = model->getItemByPath("/c");
      std::string targetpath = target->getPath();

			// Add reference
			if(target != nullptr)
			{
				REQUIRE(element != nullptr);
				REQUIRE(model->setReference(element, target));
				REQUIRE(element->getReferenceTarget() == target);
				REQUIRE(element->getReferenceData() != nullptr);
				auto expected = std::dynamic_pointer_cast<DataValue<std::string>>(element->getReferenceData());
				REQUIRE(expected != nullptr);
				REQUIRE(expected->isSet() == false);
				REQUIRE(model->createData() == nullptr);
				REQUIRE(expected->set("val1") == true);
			}

			// Create datamodel
			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			auto datalst2 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/l2"));
			REQUIRE(datalst2 != nullptr);
			REQUIRE(datalst2->addItem() != nullptr);
			REQUIRE(datalst2->addItem() != nullptr);
			REQUIRE(datamodel->validate() == false);

			// Fill datamodel
			for(auto path: value_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
				REQUIRE(data->set("val2"));
			}
			for(auto path: direct_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == false);
			}
			for(auto path: indirect_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
			}
			REQUIRE(datamodel->validate() == true);
			if(!targetpath.empty())
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(targetpath));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->set("val1"));
				REQUIRE(datamodel->validate() == false);
			}
		}

		SECTION("Check validity of a datamodel with an element in a list item with reference")
		{
      std::vector<std::string> value_paths = {
				"/e",
				"/s",
				"/c/e2",
				"/c/s2",
				"/c/l2/0/e3",
				"/c/l2/1/e3",
			};
      std::vector<std::string> direct_referenced_paths = {
				"/c/l2/0/s3",
				"/c/l2/1/s3",
			};
      std::vector<std::string> indirect_referenced_paths = {
			};
      std::shared_ptr<MetaParameter> target = std::dynamic_pointer_cast<MetaParameter>(model->getItemByPath("/e"));
      std::shared_ptr<MetaElement> element = model->getItemByPath("/c/l2/*/s3");
      std::string targetpath = target->getPath();

			// Add reference
			if(target != nullptr)
			{
				REQUIRE(element != nullptr);
				REQUIRE(model->setReference(element, target));
				REQUIRE(element->getReferenceTarget() == target);
				REQUIRE(element->getReferenceData() != nullptr);
				auto expected = std::dynamic_pointer_cast<DataValue<std::string>>(element->getReferenceData());
				REQUIRE(expected != nullptr);
				REQUIRE(expected->isSet() == false);
				REQUIRE(model->createData() == nullptr);
				REQUIRE(expected->set("val1") == true);
			}

			// Create datamodel
			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			auto datalst2 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/l2"));
			REQUIRE(datalst2 != nullptr);
			REQUIRE(datalst2->addItem() != nullptr);
			REQUIRE(datalst2->addItem() != nullptr);
			REQUIRE(datamodel->validate() == false);

			// Fill datamodel
			for(auto path: value_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
				REQUIRE(data->set("val2"));
			}
			for(auto path: direct_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == false);
			}
			for(auto path: indirect_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
			}
			REQUIRE(datamodel->validate() == true);
			if(!targetpath.empty())
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(targetpath));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->set("val1"));
				REQUIRE(datamodel->validate() == false);
			}
		}

		SECTION("Check validity of a datamodel with an element in a list item with reference in list pattern")
		{
      std::vector<std::string> value_paths = {
				"/e",
				"/s",
				"/c/e2",
				"/c/s2",
				"/c/l2/0/e3",
				"/c/l2/1/e3",
			};
      std::vector<std::string> direct_referenced_paths = {
				"/c/l2/0/s3",
				"/c/l2/1/s3",
			};
      std::vector<std::string> indirect_referenced_paths = {
			};
      std::shared_ptr<MetaParameter> target = std::dynamic_pointer_cast<MetaParameter>(model->getItemByPath("/c/l2/*/e3"));
      std::shared_ptr<MetaElement> element = model->getItemByPath("/c/l2/*/s3");
      std::string targetpath = "/c/l2/1/e3";

			// Add reference
			if(target != nullptr)
			{
				REQUIRE(element != nullptr);
				REQUIRE(model->setReference(element, target));
				REQUIRE(element->getReferenceTarget() == target);
				REQUIRE(element->getReferenceData() != nullptr);
				auto expected = std::dynamic_pointer_cast<DataValue<std::string>>(element->getReferenceData());
				REQUIRE(expected != nullptr);
				REQUIRE(expected->isSet() == false);
				REQUIRE(model->createData() == nullptr);
				REQUIRE(expected->set("val1") == true);
			}

			// Create datamodel
			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			auto datalst2 = std::dynamic_pointer_cast<DataList>(datamodel->getItemByPath("/c/l2"));
			REQUIRE(datalst2 != nullptr);
			REQUIRE(datalst2->addItem() != nullptr);
			REQUIRE(datalst2->addItem() != nullptr);
			REQUIRE(datamodel->validate() == false);

			// Fill datamodel
			for(auto path: value_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
				REQUIRE(data->set("val2"));
			}
			for(auto path: direct_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == false);
			}
			for(auto path: indirect_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
			}
			REQUIRE(datamodel->validate() == true);
			if(!targetpath.empty())
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(targetpath));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<std::string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->set("val1"));
				REQUIRE(datamodel->validate() == false);
			}
		}
	}
	
}

struct MyListener : Catch::TestEventListenerBase
{
    using TestEventListenerBase::TestEventListenerBase;

    virtual void testCaseEnded( Catch::TestCaseStats const& testCaseStats ) override {
        if(testCaseStats.totals.testCases.allPassed())
        {
            std::cout << "[PASSED] " << testCaseStats.testInfo.name << std::endl;
        }
        else
        {
            std::cout << "[FAILED] " << testCaseStats.testInfo.name << std::endl;
        }
    }
};

CATCH_REGISTER_LISTENER( MyListener )
