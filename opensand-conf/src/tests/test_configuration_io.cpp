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

std::string readFile(const std::string &filepath);

TEST_CASE("Model read/write from/to file", "[Model][Read/Write]")
{
  std::string version = "1.0.0";
	auto model = std::make_shared<MetaModel>(version);
	auto root = model->getRoot();
	REQUIRE(root != nullptr);
	auto types = model->getTypesDefinition();
	REQUIRE(types != nullptr);

	// Build meta model
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

	std::shared_ptr<DataParameter> p;
	auto datamodel = model->createData();
	REQUIRE(datamodel != nullptr);
	auto dataroot = datamodel->getRoot();
	REQUIRE(dataroot != nullptr);

	// Build data model
	p = dataroot->getParameter("e");
	REQUIRE(p != nullptr);
	REQUIRE(p->getData()->fromString("val1"));
	p = dataroot->getParameter("s");
	REQUIRE(p != nullptr);
	REQUIRE(p->getData()->fromString("val2"));

	auto datacpt = dataroot->getComponent("c");
	REQUIRE(datacpt != nullptr);
	p = datacpt->getParameter("e2");
	REQUIRE(p != nullptr);
	REQUIRE(p->getData()->fromString("val1"));
	p = datacpt->getParameter("s2");
	REQUIRE(p != nullptr);
	REQUIRE(p->getData()->fromString("val2"));

	auto datacpt2 = datacpt->getComponent("c2");
	REQUIRE(datacpt2 != nullptr);
	p = datacpt2->getParameter("e3");
	REQUIRE(p != nullptr);
	REQUIRE(p->getData()->fromString("val1"));
	p = datacpt2->getParameter("s3");
	REQUIRE(p != nullptr);
	REQUIRE(p->getData()->fromString("val2"));

	auto datalst3 = datacpt2->getList("l3");
	REQUIRE(datalst3 != nullptr);
	for(auto i = 0; i < 3; ++i)
	{
		auto c3 = datalst3->addItem();
		REQUIRE(c3 != nullptr);
		p = c3->getParameter("e4");
		REQUIRE(p != nullptr);
		REQUIRE(p->getData()->fromString("val1"));
		p = c3->getParameter("s4");
		REQUIRE(p != nullptr);
		REQUIRE(p->getData()->fromString("val2"));

		auto datalst4 = c3->getList("l4");
		REQUIRE(datalst4 != nullptr);
		for(auto j = 0; j < 2; ++j)
		{
			auto c4 = datalst4->addItem();
			REQUIRE(c4 != nullptr);
			p = c4->getParameter("e5");
			REQUIRE(p != nullptr);
			REQUIRE(p->getData()->fromString("val1"));
			p = c4->getParameter("s5");
			REQUIRE(p != nullptr);
			REQUIRE(p->getData()->fromString("val2"));

			auto c5 = c4->getComponent("c5");
			REQUIRE(c5 != nullptr);
			p = c5->getParameter("e6");
			REQUIRE(p != nullptr);
			REQUIRE(p->getData()->fromString("val1"));
			p = c5->getParameter("s6");
			REQUIRE(p != nullptr);
			REQUIRE(p->getData()->fromString("val2"));
		}
	}
	REQUIRE(datalst3->addItem() != nullptr);

	auto datalst3b = datacpt2->getList("l3b");
	REQUIRE(datalst3b != nullptr);
	for(auto i = 0; i < 5; ++i)
	{
		REQUIRE(datalst3b->addItem() != nullptr);
	}

	SECTION("Read/Write model")
	{
		std::string path = "my_model.xsd";
		std::string path2 = "my_model2.xsd";
		remove(path.c_str());
		remove(path2.c_str());

		// Test writing model to XSD
		REQUIRE(toXSD(model, path) == true);

		// Test reading model from XSD
		auto model2 = fromXSD(path);
		REQUIRE(model2 != nullptr);

		// Compare XSD files
		auto content = readFile(path);
		REQUIRE(toXSD(model2, path2) == true);
		auto content2 = readFile(path2);
		REQUIRE(content == content2);
	}

	SECTION("Read/Write model with reference")
	{
		std::string path = "my_model_ref.xsd";
		std::string path2 = "my_model_ref2.xsd";
		remove(path.c_str());
		remove(path2.c_str());

		// Add reference 1
		auto target = root->getParameter("e");
		REQUIRE(target != nullptr);
		auto element = cpt2->getParameter("s3");
		REQUIRE(element != nullptr);
		REQUIRE(model->setReference(element, target));
		REQUIRE(element->getReferenceTarget() == target);
		REQUIRE(element->getReferenceData() != nullptr);
		auto expected = std::dynamic_pointer_cast<DataValue<std::string>>(element->getReferenceData());
		REQUIRE(expected != nullptr);
		REQUIRE(expected->set("val1"));

		// Add reference 2
		auto target2 = std::dynamic_pointer_cast<MetaParameter>(model->getItemByPath("/c/e2"));
		REQUIRE(target2 != nullptr);
		auto element2 = model->getItemByPath("/c/c2/l3/*/s4");
		REQUIRE(element2 != nullptr);
		REQUIRE(model->setReference(element2, target2));
		REQUIRE(element2->getReferenceTarget() == target2);
		REQUIRE(element2->getReferenceData() != nullptr);
		auto expected2 = std::dynamic_pointer_cast<DataValue<std::string>>(element2->getReferenceData());
		REQUIRE(expected2 != nullptr);
		REQUIRE(expected2->set("val1"));

		// Test writing model to XSD
		REQUIRE(toXSD(model, path) == true);

		// Test reading model from XSD
		auto model2 = fromXSD(path);
		REQUIRE(model2 != nullptr);

		// Compare XSD files
		auto content = readFile(path);
		REQUIRE(toXSD(model2, path2) == true);
		auto content2 = readFile(path2);
		REQUIRE(content == content2);
	}

	SECTION("Read/Write data model")
	{
		std::string path = "my_datamodel.xml";
		std::string path2 = "my_datamodel2.xml";
		remove(path.c_str());
		remove(path2.c_str());

		// Test writing datamodel to XML
		REQUIRE(toXML(datamodel, path) == true);

		// Test reading datamodel from XML
		auto datamodel2 = fromXML(model, path);
		REQUIRE(datamodel2 != nullptr);
		REQUIRE(toXML(datamodel2, path2) == true);
		auto content = readFile(path);
		auto content2 = readFile(path2);
		REQUIRE(content == content2);
	}
}

std::string readFile(const std::string &filepath)
{
	std::ifstream ifs(filepath);
	std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
	return content;
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
