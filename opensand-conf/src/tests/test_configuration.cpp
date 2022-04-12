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

TEST_CASE("Truth test", "[Common]")
{
	CHECK(true);
	CHECK_FALSE(false);
	REQUIRE(1 == 1);
}

TEST_CASE("Model tests", "[Model]")
{
	std::string version = "1.0.0";
	auto model = std::make_shared<MetaModel>(version);

	SECTION("Model is well-defined")
	{
		REQUIRE(model != nullptr);
		REQUIRE(model->getVersion() == version);
		REQUIRE(model->getTypesDefinition() != nullptr);
		REQUIRE(model->getRoot() != nullptr);
		REQUIRE(model->getRoot()->getItems().empty());
	}

	SECTION("We can change the Model Description")
	{
		std::string newDescription = "newDescription";
		model->getRoot()->setDescription(newDescription);
		REQUIRE(model->getRoot()->getDescription() == newDescription);
	}

	SECTION("Basic types already exist")
	{
		REQUIRE(model->getTypesDefinition()->getType("bool") != nullptr);
		REQUIRE(model->getTypesDefinition()->getType("double") != nullptr);
		REQUIRE(model->getTypesDefinition()->getType("float") != nullptr);
		REQUIRE(model->getTypesDefinition()->getType("int") != nullptr);
		REQUIRE(model->getTypesDefinition()->getType("short") != nullptr);
		REQUIRE(model->getTypesDefinition()->getType("long") != nullptr);
		REQUIRE(model->getTypesDefinition()->getType("string") != nullptr);
	}
}

TEST_CASE("Model enumeration types tests", "[Model][EnumTypes]")
{
	auto model = std::make_shared<MetaModel>("1.0.0");
	REQUIRE(model != nullptr);
	auto primitive_type_count = model->getTypesDefinition()->getTypes().size();
	REQUIRE(primitive_type_count > 0);

	SECTION("Adding an enum without value is impossible")
	{
		REQUIRE(model->getTypesDefinition()->addEnumType("e", "enum", {}) == nullptr);
		REQUIRE(model->getTypesDefinition()->getTypes().size() == primitive_type_count);
		REQUIRE(model->getTypesDefinition()->getType("e") == nullptr);

		REQUIRE(model->getTypesDefinition()->addEnumType("e", "enum", {}, "Description") == nullptr);
		REQUIRE(model->getTypesDefinition()->getTypes().size() == primitive_type_count);
		REQUIRE(model->getTypesDefinition()->getType("e") == nullptr);
	}

	SECTION("Adding an enum with values is possible")
	{
		std::initializer_list<std::string> vals = {"val1", "val2", "val3"};
		REQUIRE(model->getTypesDefinition()->addEnumType("e", "enum", vals) != nullptr);
		REQUIRE(model->getTypesDefinition()->getTypes().size() == primitive_type_count + 1);
		REQUIRE(model->getTypesDefinition()->getType("e") != nullptr);
		
		auto e = std::dynamic_pointer_cast<MetaEnumType>(model->getTypesDefinition()->getType("e"));
		REQUIRE(e != nullptr);
		REQUIRE(e->getValues().size() == 3);

		auto desc = "my custom enum";
		e->setDescription(desc);
		REQUIRE(model->getTypesDefinition()->getType("e")->getDescription() == desc);

		auto values = e->getValues();
		for(auto val: vals)
		{
			REQUIRE(std::find(values.begin(), values.end(), val) != values.end());
		}
	}

	SECTION("Adding two enums with values is possible")
	{
		std::initializer_list<std::string> vals = {"val1", "val2", "val3"};

		REQUIRE(model->getTypesDefinition()->addEnumType("e", "enum", {"test"}) != nullptr);
		REQUIRE(model->getTypesDefinition()->getTypes().size() == primitive_type_count + 1);
		REQUIRE(model->getTypesDefinition()->getType("e") != nullptr);

		REQUIRE(model->getTypesDefinition()->addEnumType("f", "enum", vals) != nullptr);
		REQUIRE(model->getTypesDefinition()->getTypes().size() == primitive_type_count + 2);
		REQUIRE(model->getTypesDefinition()->getType("f") != nullptr);
		
		auto f = std::dynamic_pointer_cast<MetaEnumType>(model->getTypesDefinition()->getType("f"));
		REQUIRE(f != nullptr);
		REQUIRE(f->getValues().size() == 3);

		auto desc = "my custom enum";
		f->setDescription(desc);
		REQUIRE(model->getTypesDefinition()->getType("f")->getDescription() == desc);

		auto values = f->getValues();
		for(auto val: vals)
		{
			REQUIRE(std::find(values.begin(), values.end(), val) != values.end());
		}
	}

	SECTION("Adding an enum with duplicated values is possible")
	{
		std::initializer_list<std::string> vals = {"val1", "val1"};
		REQUIRE(model->getTypesDefinition()->addEnumType("e", "enum", vals) != nullptr);
		REQUIRE(model->getTypesDefinition()->getTypes().size() == primitive_type_count + 1);
		REQUIRE(model->getTypesDefinition()->getType("e") != nullptr);

		auto e = std::dynamic_pointer_cast<MetaEnumType>(model->getTypesDefinition()->getType("e"));
		REQUIRE(e != nullptr);
		REQUIRE(e->getValues().size() == 1);

		auto values = e->getValues();
		REQUIRE(std::find(values.begin(), values.end(), "val1") != values.end());
	}

	SECTION("Adding an enum with already existing id fails")
	{
		REQUIRE(model->getTypesDefinition()->addEnumType("e", "enum", {"val1", "val2"}) != nullptr);
		REQUIRE(model->getTypesDefinition()->getTypes().size() == primitive_type_count + 1);
		REQUIRE(model->getTypesDefinition()->getType("e") != nullptr);

		REQUIRE(model->getTypesDefinition()->addEnumType("e", "enum2", {"test"}) == nullptr);
		REQUIRE(model->getTypesDefinition()->getTypes().size() == primitive_type_count + 1);
		REQUIRE(model->getTypesDefinition()->getType("e") != nullptr);
		REQUIRE(model->getTypesDefinition()->getType("e")->getName() == "enum");
	}
}

TEST_CASE("Model component tests", "[Model][Component]")
{
	std::string version = "1.0.0";
	auto model = std::make_shared<MetaModel>(version);
	REQUIRE(model != nullptr);

	SECTION("We can add components")
	{
		auto cpt1 = model->getRoot()->addComponent("id1", "Component 1");
		REQUIRE(cpt1 != nullptr);
		REQUIRE(cpt1->getId() == "id1");
		REQUIRE(cpt1->getName() == "Component 1");
		REQUIRE(cpt1->getDescription() == "");
		REQUIRE(cpt1->getItems().empty());
		REQUIRE(model->getRoot()->getItems().size() == 1);

		auto cpt2 = model->getRoot()->addComponent("id2", "Component 2", "Description 2");
		REQUIRE(cpt2 != nullptr);
		REQUIRE(cpt2->getId() == "id2");
		REQUIRE(cpt2->getName() == "Component 2");
		REQUIRE(cpt2->getDescription() == "Description 2");
		REQUIRE(cpt2->getItems().empty());
		REQUIRE(model->getRoot()->addComponent("id1", "Component 2") == nullptr);
		REQUIRE(model->getRoot()->getItems().size() == 2);

		REQUIRE(cpt1->addParameter("id1", "Parameter 1", model->getTypesDefinition()->getType("int")) != nullptr);
		REQUIRE(cpt1->getItems().size() == 1);
		REQUIRE(cpt2->getItems().empty());
	}

	SECTION("We can add composite components")
	{
		auto cpt1 = model->getRoot()->addComponent("id1", "Component 1");
		REQUIRE(cpt1 != nullptr);
		REQUIRE(cpt1->getItems().empty());
		auto cpt2 = model->getRoot()->addComponent("id2", "Component 1");
		REQUIRE(cpt2 != nullptr);
		REQUIRE(cpt2->getItems().empty());

		auto cpt3 = cpt1->addComponent("id1", "Component 1");
		REQUIRE(cpt3 != nullptr);
		REQUIRE(cpt1->getItems().size() == 1);
		REQUIRE(cpt2->getItems().empty());
		
		REQUIRE(cpt1->addComponent("id1", "Component 2") == nullptr);
		REQUIRE(cpt2->addComponent("id1", "Component 2") != nullptr);

		REQUIRE(cpt3->addComponent("id3", "Component 3") != nullptr);

		REQUIRE(cpt1->getItems().size() == 1);
		REQUIRE(cpt2->getItems().size() == 1);
	}

	SECTION("We can create a data components from a meta components")
	{
		auto cpt1 = model->getRoot()->addComponent("id1", "Component 1");
		REQUIRE(cpt1 != nullptr);
		
		auto cpt2 = model->getRoot()->addComponent("id2", "Component 2");
		REQUIRE(cpt2 != nullptr);

		auto cpt13 = cpt1->addComponent("id3", "Component 3");
		REQUIRE(cpt13 != nullptr);

		auto cpt24 = cpt2->addComponent("id4", "Component 4");
		REQUIRE(cpt24 != nullptr);

		auto cpt135 = cpt13->addComponent("id5", "Component 5");

		auto datamodel = model->createData();
		REQUIRE(datamodel != nullptr);
		REQUIRE(datamodel->getVersion() == model->getVersion());
		REQUIRE(datamodel->getRoot() != nullptr);
		REQUIRE(!datamodel->getRoot()->getItems().empty());

		auto data1 = datamodel->getRoot()->getComponent(cpt1->getId());
		REQUIRE(data1 != nullptr);

		auto data2 = datamodel->getRoot()->getComponent(cpt2->getId());
		REQUIRE(data2 != nullptr);

		auto data13 = data1->getComponent(cpt13->getId());
		REQUIRE(data13 != nullptr);

		auto data24 = data2->getComponent(cpt24->getId());
		REQUIRE(data24 != nullptr);

		auto data135 = data13->getComponent(cpt135->getId());
		REQUIRE(data135 != nullptr);

		REQUIRE(datamodel->validate());
	}
}

TEST_CASE("Model parameter tests", "[Model][Parameter]")
{
	std::string version = "1.0.0";
	auto model = std::make_shared<MetaModel>(version);
	REQUIRE(model != nullptr);

	SECTION("We can add parameter")
	{
		SECTION("We can add a boolean parameter")
		{
			std::string id = "id1";
			bool val1 = true;
			bool val2 = false;

			auto param = model->getRoot()->addParameter(id, "Parameter 1", model->getTypesDefinition()->getType("bool"));
			REQUIRE(param != nullptr);
			REQUIRE(std::dynamic_pointer_cast<MetaValueType<bool>>(param->getType()) != nullptr);
			REQUIRE(param->getUnit() == "");
			auto unit = "u";
			param->setUnit(unit);
			REQUIRE(param->getUnit() == unit);

			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			REQUIRE(datamodel->getVersion() == model->getVersion());
			REQUIRE(datamodel->getRoot() != nullptr);
			REQUIRE(!datamodel->getRoot()->getItems().empty());

			auto dataparam = datamodel->getRoot()->getParameter(param->getId());
			REQUIRE(dataparam != nullptr);
			auto data = std::dynamic_pointer_cast<DataValue<bool>>(dataparam->getData());
			REQUIRE(data != nullptr);
			REQUIRE(dataparam->getData()->isSet() == false);

			REQUIRE(data->set(val1) == true);
			REQUIRE(data->isSet() == true);
			REQUIRE(data->get() == val1);
			REQUIRE(data->set(val2) == true);
			REQUIRE(data->isSet() == true);
			REQUIRE(data->get() == val2);
			data->reset();
			REQUIRE(data->isSet() == false);

			SECTION("We can set a boolean data from string")
			{
				bool val1 = false;
				std::string str1 = "false";
				bool val2 = true;
				std::string str2 = "true";
				std::string invalid = "42";

				data->reset();
				REQUIRE(data->isSet() == false);
				REQUIRE(data->set(val1) == true);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val1);
				REQUIRE(data->toString() == str1);

				REQUIRE(data->fromString(str2) == true);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val2);
				REQUIRE(data->toString() == str2);

				REQUIRE(data->fromString(invalid) == false);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val2);
				REQUIRE(data->toString() == str2);

				data->reset();
				REQUIRE(data->isSet() == false);
				REQUIRE(data->fromString(invalid) == false);
				REQUIRE(data->isSet() == false);

				data->reset();
				REQUIRE(data->isSet() == false);
				REQUIRE(data->fromString(str1) == true);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val1);
				REQUIRE(data->toString() == str1);
			}
		}

		SECTION("We can add a integer parameter")
		{
			std::string id = "id1";
			int val1 = 23;
			int val2 = 42;

			auto param = model->getRoot()->addParameter(id, "Parameter 1", model->getTypesDefinition()->getType("int"));
			REQUIRE(param != nullptr);
			REQUIRE(std::dynamic_pointer_cast<MetaValueType<int>>(param->getType()) != nullptr);
			REQUIRE(param->getUnit() == "");
			auto unit = "u";
			param->setUnit(unit);
			REQUIRE(param->getUnit() == unit);

			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			REQUIRE(datamodel->getVersion() == model->getVersion());
			REQUIRE(datamodel->getRoot() != nullptr);
			REQUIRE(!datamodel->getRoot()->getItems().empty());

			auto dataparam = datamodel->getRoot()->getParameter(param->getId());
			REQUIRE(dataparam != nullptr);
			auto data = std::dynamic_pointer_cast<DataValue<int>>(dataparam->getData());
			REQUIRE(data != nullptr);
			REQUIRE(dataparam->getData()->isSet() == false);

			REQUIRE(data->set(val1) == true);
			REQUIRE(data->isSet() == true);
			REQUIRE(data->get() == val1);
			REQUIRE(data->set(val2) == true);
			REQUIRE(data->isSet() == true);
			REQUIRE(data->get() == val2);
			data->reset();
			REQUIRE(data->isSet() == false);

			SECTION("We can set an integer data from string")
			{
				int val1 = 42;
				std::string str1 = "42";
				int val2 = 23;
				std::string str2 = "23";
				int val3 = 86;
				std::string str3 = "86.2";
				std::string str3b = "86";
				std::string invalid = "azerty";
				//std::string invalid = "42.23azerty"; // FIXME? fromString succeeds but would not

				data->reset();
				REQUIRE(data->isSet() == false);
				REQUIRE(data->set(val1) == true);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val1);
				REQUIRE(data->toString() == str1);

				REQUIRE(data->fromString(str2) == true);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val2);
				REQUIRE(data->toString() == str2);

				REQUIRE(data->fromString(invalid) == false);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val2);
				REQUIRE(data->toString() == str2);

				REQUIRE(data->fromString(str3) == true);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val3);
				REQUIRE(data->toString() == str3b);

				data->reset();
				REQUIRE(data->isSet() == false);
				REQUIRE(data->fromString(invalid) == false);
				REQUIRE(data->isSet() == false);

				data->reset();
				REQUIRE(data->isSet() == false);
				REQUIRE(data->fromString(str1) == true);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val1);
				REQUIRE(data->toString() == str1);
			}
		}

		SECTION("We can add a double parameter")
		{
			std::string id = "id1";
			double val1 = 0.23;
			double val2 = 0.42;

			auto param = model->getRoot()->addParameter(id, "Parameter 1", model->getTypesDefinition()->getType("double"));
			REQUIRE(param != nullptr);
			REQUIRE(std::dynamic_pointer_cast<MetaValueType<double>>(param->getType()) != nullptr);
			REQUIRE(param->getUnit() == "");
			auto unit = "u";
			param->setUnit(unit);
			REQUIRE(param->getUnit() == unit);

			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			REQUIRE(datamodel->getVersion() == model->getVersion());
			REQUIRE(datamodel->getRoot() != nullptr);
			REQUIRE(!datamodel->getRoot()->getItems().empty());

			auto dataparam = datamodel->getRoot()->getParameter(param->getId());
			REQUIRE(dataparam != nullptr);
			auto data = std::dynamic_pointer_cast<DataValue<double>>(dataparam->getData());
			REQUIRE(data != nullptr);
			REQUIRE(dataparam->getData()->isSet() == false);

			REQUIRE(data->set(val1) == true);
			REQUIRE(data->isSet() == true);
			REQUIRE(data->get() == val1);
			REQUIRE(data->set(val2) == true);
			REQUIRE(data->isSet() == true);
			REQUIRE(data->get() == val2);
			data->reset();
			REQUIRE(data->isSet() == false);

			SECTION("We can set a double data from string")
			{
				double val1 = 42.42;
				std::string str1 = "42.420000";
				double val2 = 23.23;
				std::string str2 = "23.230000";
				double val3 = 1.12e3;
				std::string str3 = "1.12e3";
				std::string str3b = "1120.000000";
				int val4 = 86;
				std::string str4 = "86";
				std::string str4b = "86.000000";
				std::string invalid = "azerty";

				data->reset();
				REQUIRE(data->isSet() == false);
				REQUIRE(data->set(val1) == true);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val1);
				REQUIRE(data->toString() == str1);

				REQUIRE(data->fromString(str2) == true);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val2);
				REQUIRE(data->toString() == str2);

				REQUIRE(data->fromString(invalid) == false);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val2);
				REQUIRE(data->toString() == str2);

				REQUIRE(data->fromString(str3) == true);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val3);
				REQUIRE(data->toString() == str3b);

				REQUIRE(data->fromString(str4) == true);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == (double)val4);
				REQUIRE(data->toString() == str4b);

				data->reset();
				REQUIRE(data->isSet() == false);
				REQUIRE(data->fromString(invalid) == false);
				REQUIRE(data->isSet() == false);

				data->reset();
				REQUIRE(data->isSet() == false);
				REQUIRE(data->fromString(str1) == true);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val1);
				REQUIRE(data->toString() == str1);
			}
		}

		SECTION("We can add a string parameter")
		{
			std::string id = "id1";
			std::string val1 = "value 1";
			std::string val2 = "value 2";

			auto param = model->getRoot()->addParameter(id, "Parameter 1", model->getTypesDefinition()->getType("string"));
			REQUIRE(param != nullptr);
			REQUIRE(std::dynamic_pointer_cast<MetaValueType<std::string>>(param->getType()) != nullptr);
			REQUIRE(param->getUnit() == "");
			auto unit = "u";
			param->setUnit(unit);
			REQUIRE(param->getUnit() == unit);

			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			REQUIRE(datamodel->getVersion() == model->getVersion());
			REQUIRE(datamodel->getRoot() != nullptr);
			REQUIRE(!datamodel->getRoot()->getItems().empty());

			auto dataparam = datamodel->getRoot()->getParameter(param->getId());
			REQUIRE(dataparam != nullptr);
			auto data = std::dynamic_pointer_cast<DataValue<std::string>>(dataparam->getData());
			REQUIRE(data != nullptr);
			REQUIRE(dataparam->getData()->isSet() == false);

			REQUIRE(data->set(val1) == true);
			REQUIRE(data->isSet() == true);
			REQUIRE(data->get() == val1);
			REQUIRE(data->set(val2) == true);
			REQUIRE(data->isSet() == true);
			REQUIRE(data->get() == val2);
			data->reset();
			REQUIRE(data->isSet() == false);

			SECTION("We can set a string data from string")
			{
				std::string val1 = "42.42azerty";
				std::string str1 = val1;
				std::string val2 = "23.23!?*";
				std::string str2 = val2;

				data->reset();
				REQUIRE(data->isSet() == false);
				REQUIRE(data->set(val1) == true);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val1);
				REQUIRE(data->toString() == str1);

				REQUIRE(data->fromString(str2) == true);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val2);
				REQUIRE(data->toString() == str2);

				data->reset();
				REQUIRE(data->isSet() == false);
				REQUIRE(data->fromString(str1) == true);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->get() == val1);
				REQUIRE(data->toString() == str1);
			}
		}

		SECTION("We can add an enumeration parameter")
		{
			std::string id = "id1";
			std::string val1 = "value 1";
			std::string val2 = "value 2";
			std::string invalid = "value 3";

			auto type = model->getTypesDefinition()->addEnumType("enum1", "Parameter enum 1", { val1, val2 });
			REQUIRE(type != nullptr);

			REQUIRE(model->getRoot()->getItems().empty());
			REQUIRE(model->getRoot()->getParameter(id) == nullptr);
			auto param = model->getRoot()->addParameter(id, "Parameter 1", model->getTypesDefinition()->getType("enum1"));
			REQUIRE(param != nullptr);
			REQUIRE(std::dynamic_pointer_cast<MetaEnumType>(param->getType()) != nullptr);
			REQUIRE(param->getUnit() == "");
			auto unit = "u";
			param->setUnit(unit);
			REQUIRE(param->getUnit() == unit);

			auto datamodel = model->createData();
			REQUIRE(datamodel != nullptr);
			REQUIRE(datamodel->getVersion() == model->getVersion());
			REQUIRE(datamodel->getRoot() != nullptr);
			REQUIRE(!datamodel->getRoot()->getItems().empty());

			auto dataparam = datamodel->getRoot()->getParameter(param->getId());
			REQUIRE(dataparam != nullptr);
			auto data = std::dynamic_pointer_cast<DataValue<std::string>>(dataparam->getData());
			REQUIRE(data != nullptr);
			REQUIRE(dataparam->getData()->isSet() == false);

			REQUIRE(data->set(val1) == true);
			REQUIRE(data->isSet() == true);
			REQUIRE(data->get() == val1);
			REQUIRE(data->set(val2) == true);
			REQUIRE(data->isSet() == true);
			REQUIRE(data->get() == val2);
			data->reset();
			REQUIRE(data->isSet() == false);
			REQUIRE(data->set(invalid) == false);
			REQUIRE(data->isSet() == false);
		}
	}

	SECTION("We can add several parameters")
	{
		auto param1 = model->getRoot()->addParameter("id1", "Parameter 1", model->getTypesDefinition()->getType("string"));
		REQUIRE(param1 != nullptr);
		REQUIRE(model->getRoot()->addParameter("id1", "Parameter 2", model->getTypesDefinition()->getType("string")) == nullptr);
		REQUIRE(model->getRoot()->addParameter("id1", "Parameter 1", model->getTypesDefinition()->getType("int")) == nullptr);
		REQUIRE(param1->getUnit() == "");
		auto unit1 = "u";
		param1->setUnit(unit1);
		REQUIRE(param1->getUnit() == unit1);

		auto param2 = model->getRoot()->addParameter("id2", "Parameter 2", model->getTypesDefinition()->getType("string"));
		REQUIRE(param2 != nullptr);
		REQUIRE(param2->getUnit() == "");
		auto unit2 = "u2";
		param2->setUnit(unit2);
		REQUIRE(param2->getUnit() == unit2);
		REQUIRE(param1->getUnit() == unit1);

		auto datamodel = model->createData();
		REQUIRE(datamodel != nullptr);
		REQUIRE(datamodel->getVersion() == model->getVersion());
		REQUIRE(datamodel->getRoot() != nullptr);
		REQUIRE(!datamodel->getRoot()->getItems().empty());

		auto dataparam1 = datamodel->getRoot()->getParameter(param1->getId());
		REQUIRE(dataparam1 != nullptr);
		auto dataparam2 = datamodel->getRoot()->getParameter(param2->getId());
		REQUIRE(dataparam2 != nullptr);
		auto data1 = std::dynamic_pointer_cast<DataValue<std::string>>(dataparam1->getData());
		REQUIRE(data1 != nullptr);
		auto data2 = std::dynamic_pointer_cast<DataValue<std::string>>(dataparam2->getData());
		REQUIRE(data2 != nullptr);

		REQUIRE(data1->isSet() == false);
		REQUIRE(data2->isSet() == false);
		REQUIRE(data1->set("value") == true);
		REQUIRE(data1->isSet() == true);
		REQUIRE(data2->isSet() == false);
	}

	SECTION("We can create a data parameters from a meta parameters")
	{
		auto param1 = model->getRoot()->addParameter("id1", "Parameter 1", model->getTypesDefinition()->getType("string"));
		REQUIRE(param1 != nullptr);
		REQUIRE(model->getRoot()->addParameter("id2", "Parameter 2", model->getTypesDefinition()->getType("string")) != nullptr);
		REQUIRE(model->getRoot()->addComponent("id1", "Component 1") == nullptr);

		auto cpt1 = model->getRoot()->addComponent("cpt1", "Component 1");
		REQUIRE(cpt1 != nullptr);
		REQUIRE(model->getRoot()->addParameter("cpt1", "Parameter 2", model->getTypesDefinition()->getType("string")) == nullptr);

		auto param2 = cpt1->addParameter("id1", "Parameter 1", model->getTypesDefinition()->getType("string"));
		REQUIRE(param2 != nullptr);
		REQUIRE(cpt1->addParameter("id2", "Parameter 2", model->getTypesDefinition()->getType("string")) != nullptr);

		auto datamodel = model->createData();
		REQUIRE(datamodel != nullptr);
		REQUIRE(datamodel->getVersion() == model->getVersion());
		REQUIRE(datamodel->getRoot() != nullptr);
		REQUIRE(!datamodel->getRoot()->getItems().empty());

		auto dataparam1 = datamodel->getRoot()->getParameter(param1->getId());
		REQUIRE(dataparam1 != nullptr);
		auto datacpt1 = datamodel->getRoot()->getComponent(cpt1->getId());
		REQUIRE(datacpt1 != nullptr);
		auto dataparam2 = datacpt1->getParameter(param2->getId());
		REQUIRE(dataparam2 != nullptr);
		auto data1 = std::dynamic_pointer_cast<DataValue<std::string>>(dataparam1->getData());
		REQUIRE(data1 != nullptr);
		auto data2 = std::dynamic_pointer_cast<DataValue<std::string>>(dataparam2->getData());
		REQUIRE(data2 != nullptr);

		REQUIRE(data1->isSet() == false);
		REQUIRE(data2->isSet() == false);
		REQUIRE(data1->set("value") == true);
		REQUIRE(data1->isSet() == true);
		REQUIRE(data2->isSet() == false);
	}

	SECTION("We can get parameters from path")
	{
		REQUIRE(model->getItemByPath("") == nullptr);
		auto root = model->getItemByPath("/");
		REQUIRE(root == model->getRoot());
		auto cpt1 = model->getRoot()->addComponent("cpt1", "Component 1");
		REQUIRE(cpt1 != nullptr);
		REQUIRE(cpt1 == model->getRoot()->getItem("cpt1"));
		REQUIRE(cpt1 == model->getItemByPath("/cpt1"));
		auto cpt2 = cpt1->addComponent("cpt2", "Component 2");
		REQUIRE(cpt2 != nullptr);
		REQUIRE(cpt2 == cpt1->getItem("cpt2"));
		REQUIRE(cpt2 == model->getItemByPath("/cpt1/cpt2"));
		auto param = cpt2->addParameter("p1", "Parameter 1", model->getTypesDefinition()->getType("string"));
		REQUIRE(param != nullptr);
		REQUIRE(param == cpt2->getItem("p1"));
		auto elt = model->getItemByPath("/cpt1/cpt2/p1");
		REQUIRE(param == elt);
		auto param2 = std::dynamic_pointer_cast<MetaParameter>(elt);
		REQUIRE(param == param2);

		std::string desc = "This is a description";
		param2->setDescription(desc);
		REQUIRE(param->getDescription() == desc);
	}
}

TEST_CASE("Model list tests", "[Model][List]")
{
	std::string version = "1.0.0";
	auto model = std::make_shared<MetaModel>(version);
	REQUIRE(model != nullptr);

	SECTION("We can add lists")
	{
		auto lst1 = model->getRoot()->addList("id1", "List 1", "Pattern 1");
		REQUIRE(lst1 != nullptr);
		REQUIRE(lst1->getPattern() != nullptr);

		auto lst2 = model->getRoot()->addList("id2", "List 1", "Pattern 1");
		REQUIRE(lst2 != nullptr);
		REQUIRE(lst2->getPattern() != nullptr);
		REQUIRE(model->getRoot()->addList("id1", "List 2", "Pattern 2") == nullptr);

		auto ptn1 = std::dynamic_pointer_cast<MetaComponent>(lst1->getPattern());
		REQUIRE(ptn1 != nullptr);
		REQUIRE(ptn1->getItems().empty());
		auto ptn2 = std::dynamic_pointer_cast<MetaComponent>(lst2->getPattern());
		REQUIRE(ptn2 != nullptr);
		REQUIRE(ptn2->getItems().empty());

		REQUIRE(ptn1->addParameter("id1", "Parameter 1", model->getTypesDefinition()->getType("int")) != nullptr);
		REQUIRE(ptn1->getItems().size() == 1);
		REQUIRE(ptn2->getItems().empty());
	}

	SECTION("We can add lists items")
	{
		auto lst = model->getRoot()->addList("id1", "List 1", "Pattern 1");
		REQUIRE(lst != nullptr);
		REQUIRE(lst->getPattern() != nullptr);

		// Build pattern
		std::string desc = "This is a description";
		auto ptn = std::dynamic_pointer_cast<MetaComponent>(lst->getPattern());
		REQUIRE(ptn != nullptr);
		ptn->setDescription(desc);
		REQUIRE(ptn->getDescription() == desc);
		REQUIRE(ptn->getItems().empty());
		REQUIRE(ptn->addParameter("p1", "Parameter 1", model->getTypesDefinition()->getType("string")) != nullptr);
		REQUIRE(ptn->addList("l1", "List 1", "Item") != nullptr);
		REQUIRE(ptn->getList("l1")->getPattern()->addParameter("p1", "Parameter 1", model->getTypesDefinition()->getType("string")) != nullptr);

		// Add items and check
		auto datamodel = model->createData();
		REQUIRE(datamodel != nullptr);
		REQUIRE(datamodel->getVersion() == model->getVersion());
		REQUIRE(datamodel->getRoot() != nullptr);
		REQUIRE(!datamodel->getRoot()->getItems().empty());
		auto datalst = datamodel->getRoot()->getList(lst->getId());
		REQUIRE(datalst != nullptr);
		REQUIRE(datalst->getItems().empty());

		auto item1 = std::dynamic_pointer_cast<DataComponent>(datalst->addItem());
		REQUIRE(item1 != nullptr);
		REQUIRE(datalst->getItems().size() == 1);
		auto item2 = std::dynamic_pointer_cast<DataComponent>(datalst->addItem());
		REQUIRE(item2 != nullptr);
		REQUIRE(datalst->getItems().size() == 2);

		// Check items parameters
		auto i1p1 = item1->getParameter("p1");
		REQUIRE(i1p1 != nullptr);
		auto i1d1 = std::dynamic_pointer_cast<DataValue<std::string>>(i1p1->getData());
		REQUIRE(i1d1 != nullptr);
		auto i2p1 = item2->getParameter("p1");
		REQUIRE(i2p1 != nullptr);
		auto i2d1 = std::dynamic_pointer_cast<DataValue<std::string>>(i2p1->getData());
		REQUIRE(i2d1 != nullptr);

		REQUIRE(i1d1->isSet() == false);
		REQUIRE(i2d1->isSet() == false);

		REQUIRE(i1d1->set("value") == true);
		REQUIRE(i1d1->isSet() == true);
		REQUIRE(i2d1->isSet() == false);

		// Check items lists
		auto i1l1 = item1->getList("l1");
		REQUIRE(i1l1 != nullptr);
		REQUIRE(i1l1->getItems().empty());
		auto i2l1 = item2->getList("l1");
		REQUIRE(i2l1 != nullptr);
		REQUIRE(i2l1->getItems().empty());
		auto i1l1i1 = i1l1->addItem();
		REQUIRE(i1l1i1 != nullptr);
		REQUIRE(i1l1->getItems().size() == 1);
		REQUIRE(i2l1->getItems().empty());
	}

	SECTION("We can create a data lists from a meta lists")
	{
		auto cpt = model->getRoot()->addComponent("cpt", "Component");
		auto l = model->getRoot()->addList("id1", "List 1", "Pattern 1");
		REQUIRE(l != nullptr);

		auto l1 = cpt->addList("id1", "List 1", "Pattern 1");
		REQUIRE(l1 != nullptr);

		// Build pattern
		auto ptn = std::dynamic_pointer_cast<MetaComponent>(l1->getPattern());
		REQUIRE(ptn != nullptr);
		REQUIRE(ptn->getItems().empty());
		REQUIRE(ptn->addParameter("p1", "Parameter 1", model->getTypesDefinition()->getType("string")) != nullptr);
		REQUIRE(ptn->addList("l1", "List 1", "Item") != nullptr);
		REQUIRE(ptn->getList("l1")->getPattern()->addParameter("p1", "Parameter 1", model->getTypesDefinition()->getType("string")) != nullptr);
		auto p = std::dynamic_pointer_cast<MetaComponent>(l->getPattern());
		REQUIRE(p != nullptr);
		REQUIRE(p->getItems().empty());
		REQUIRE(ptn->getItems().size() == 2);

		// Add and check items
		auto datamodel = model->createData();
		REQUIRE(datamodel != nullptr);
		REQUIRE(datamodel->getVersion() == model->getVersion());
		REQUIRE(datamodel->getRoot() != nullptr);
		REQUIRE(!datamodel->getRoot()->getItems().empty());
		auto datacpt = datamodel->getRoot()->getComponent(cpt->getId());
		REQUIRE(datacpt != nullptr);
		auto datal = datamodel->getRoot()->getList(l->getId());
		REQUIRE(datal != nullptr);
		auto datal1 = datacpt->getList(l1->getId());
		REQUIRE(datal1 != nullptr);
		REQUIRE(datal1->getItems().empty());

		auto item1 = std::dynamic_pointer_cast<DataComponent>(datal1->addItem());
		REQUIRE(item1 != nullptr);
		auto item2 = std::dynamic_pointer_cast<DataComponent>(datal1->addItem());
		REQUIRE(item2 != nullptr);
		REQUIRE(datal->getItems().empty());
		REQUIRE(datal1->getItems().size() == 2);

		// Check items parameters
		auto i1p1 = item1->getParameter("p1");
		REQUIRE(i1p1 != nullptr);
		auto i1d1 = std::dynamic_pointer_cast<DataValue<std::string>>(i1p1->getData());
		REQUIRE(i1d1 != nullptr);
		auto i2p1 = item2->getParameter("p1");
		REQUIRE(i2p1 != nullptr);
		auto i2d1 = std::dynamic_pointer_cast<DataValue<std::string>>(i2p1->getData());
		REQUIRE(i2d1 != nullptr);

		REQUIRE(i1d1->isSet() == false);
		REQUIRE(i2d1->isSet() == false);

		REQUIRE(i1d1->set("value") == true);
		REQUIRE(i1d1->isSet() == true);
		REQUIRE(i2d1->isSet() == false);

		// Check items lists
		auto i1l1 = item1->getList("l1");
		REQUIRE(i1l1 != nullptr);
		REQUIRE(i1l1->getItems().empty());
		auto i2l1 = item2->getList("l1");
		REQUIRE(i2l1 != nullptr);
		REQUIRE(i2l1->getItems().empty());
		auto i1l1i1 = i1l1->addItem();
		REQUIRE(i1l1i1 != nullptr);
		REQUIRE(i1l1->getItems().size() == 1);
		REQUIRE(i2l1->getItems().empty());
	}

	SECTION("We can get lists from path")
	{
		REQUIRE(model->getItemByPath("") == nullptr);
		auto root = model->getItemByPath("/");
		REQUIRE(root == model->getRoot());
		auto cpt1 = model->getRoot()->addComponent("cpt1", "Component 1");
		REQUIRE(cpt1 != nullptr);
		REQUIRE(cpt1 == model->getRoot()->getItem("cpt1"));
		REQUIRE(cpt1 == model->getItemByPath("/cpt1"));
		auto cpt2 = cpt1->addComponent("cpt2", "Component 2");
		REQUIRE(cpt2 != nullptr);
		REQUIRE(cpt2 == cpt1->getItem("cpt2"));
		REQUIRE(cpt2 == model->getItemByPath("/cpt1/cpt2"));
		auto lst = cpt2->addList("l1", "List 1", "Item");
		REQUIRE(lst != nullptr);
		REQUIRE(lst == cpt2->getList("l1"));
		REQUIRE(lst == model->getItemByPath("/cpt1/cpt2/l1"));
		auto pattern = lst->getPattern();
		REQUIRE(pattern != nullptr);
		REQUIRE(pattern == model->getItemByPath("/cpt1/cpt2/l1/*"));
		auto param = pattern->addParameter("p1", "Parameter 1", model->getTypesDefinition()->getType("string"));
		REQUIRE(param != nullptr);
		REQUIRE(param == pattern->getParameter("p1"));
		REQUIRE(param == model->getItemByPath("/cpt1/cpt2/l1/*/p1"));

		// Check data
		auto datamodel = model->createData();
		REQUIRE(datamodel != nullptr);
		REQUIRE(datamodel->getVersion() == model->getVersion());
		REQUIRE(datamodel->getRoot() != nullptr);
		REQUIRE(!datamodel->getRoot()->getItems().empty());
		REQUIRE(datamodel->getItemByPath("") == nullptr);
		REQUIRE(datamodel->getItemByPath("/") == datamodel->getRoot());
		auto datacpt1 = datamodel->getRoot()->getComponent("cpt1");
		REQUIRE(datacpt1 == datamodel->getItemByPath("/cpt1"));
		auto datacpt2 = datacpt1->getComponent("cpt2");
		REQUIRE(datacpt2 == datamodel->getItemByPath("/cpt1/cpt2"));
		auto datalst = datacpt2->getList("l1");
		REQUIRE(datalst == datamodel->getItemByPath("/cpt1/cpt2/l1"));
		REQUIRE(datamodel->getItemByPath("/cpt1/cpt2/l1/*") == nullptr);
		REQUIRE(datalst->getItems().empty() == true);

		auto dataitem1 = datalst->addItem();
		REQUIRE(dataitem1 != nullptr);
		REQUIRE(dataitem1 == datamodel->getItemByPath("/cpt1/cpt2/l1/0"));
		auto dataparam = dataitem1->getParameter("p1");
		REQUIRE(dataparam != nullptr);
		REQUIRE(dataparam == datamodel->getItemByPath("/cpt1/cpt2/l1/0/p1"));
		auto data = std::static_pointer_cast<DataValue<std::string>>(dataparam->getData());
		REQUIRE(data != nullptr);
		REQUIRE(data->isSet() == false);
		REQUIRE(data->set("value") == true);
		REQUIRE(data->isSet() == true);
		REQUIRE(data->get() == "value");

		auto dataitem2 = datalst->addItem();
		REQUIRE(dataitem2 != nullptr);
		REQUIRE(dataitem1 == datamodel->getItemByPath("/cpt1/cpt2/l1/0"));
		REQUIRE(dataitem2 == datamodel->getItemByPath("/cpt1/cpt2/l1/1"));
		auto dataparam2 = dataitem2->getParameter("p1");
		REQUIRE(dataparam2 != nullptr);
		REQUIRE(dataparam2 == datamodel->getItemByPath("/cpt1/cpt2/l1/1/p1"));
		auto data2 = std::dynamic_pointer_cast<DataValue<std::string>>(dataparam2->getData());
		REQUIRE(data2 != nullptr);
		REQUIRE(data2->isSet() == false);
	}
}

TEST_CASE("Model data tests", "[Model][Data]")
{
	std::string version = "1.0.0";
	auto model = std::make_shared<MetaModel>(version);

	SECTION("We can create a data model from a meta model")
	{
		auto datamodel = model->createData();
		REQUIRE(datamodel != nullptr);
		REQUIRE(datamodel->getVersion() == model->getVersion());
		REQUIRE(datamodel->getRoot() != nullptr);
		REQUIRE(datamodel->getRoot()->getItems().empty());
		REQUIRE(datamodel->validate());
	}

	SECTION("We can create several data models from a single meta model")
	{
		REQUIRE(model->getRoot()->addParameter("p", "Parameter", model->getTypesDefinition()->getType("int")) != nullptr);
		REQUIRE(model->getRoot()->addList("l", "List", "Pattern") != nullptr);
		REQUIRE(model->getRoot()->getList("l")->getPattern()->addParameter("p", "Parameter", model->getTypesDefinition()->getType("int")) != nullptr);

		auto datamodel = model->createData();
		REQUIRE(datamodel != nullptr);
		REQUIRE(datamodel->getVersion() == model->getVersion());
		REQUIRE(datamodel->getRoot() != nullptr);
		REQUIRE(datamodel->getRoot()->getItems().size() == 2);
		REQUIRE(datamodel->getRoot()->getParameter("p") != nullptr);
		REQUIRE(datamodel->getRoot()->getList("l") != nullptr);
		REQUIRE(datamodel->getRoot()->getList("l")->getItems().empty());
		REQUIRE(datamodel->validate() == false);
		auto item1 = std::dynamic_pointer_cast<DataComponent>(datamodel->getRoot()->getList("l")->addItem());
		REQUIRE(item1 != nullptr);
		REQUIRE(item1->getItems().size() == 1);
		REQUIRE(item1->getParameter("p") != nullptr);
		REQUIRE(datamodel->validate() == false);

		auto datamodel2 = model->createData();
		REQUIRE(datamodel2 != nullptr);
		REQUIRE(datamodel2->getVersion() == model->getVersion());
		REQUIRE(datamodel2->getRoot() != nullptr);
		REQUIRE(datamodel2->getRoot()->getItems().size() == 2);
		REQUIRE(datamodel2->getRoot()->getParameter("p") != nullptr);
		REQUIRE(datamodel2->getRoot()->getList("l") != nullptr);
		REQUIRE(datamodel2->getRoot()->getList("l")->getItems().empty());
		auto item2 = std::dynamic_pointer_cast<DataComponent>(datamodel2->getRoot()->getList("l")->addItem());
		REQUIRE(item2 != nullptr);
		REQUIRE(item2->getItems().size() == 1);
		REQUIRE(item2->getParameter("p") != nullptr);
		auto item3 = std::dynamic_pointer_cast<DataComponent>(datamodel2->getRoot()->getList("l")->addItem());
		REQUIRE(item3 != nullptr);
		REQUIRE(item3->getItems().size() == 1);
		REQUIRE(item3->getParameter("p") != nullptr);
		REQUIRE(datamodel2->validate() == false);

		REQUIRE(datamodel->getRoot()->getList("l")->getItems().size() == 1);
		REQUIRE(datamodel2->getRoot()->getList("l")->getItems().size() == 2);

		auto data = std::dynamic_pointer_cast<DataValue<int>>(datamodel->getRoot()->getParameter("p")->getData());
		REQUIRE(data != nullptr);
		data->set(42);
		auto data1 = std::dynamic_pointer_cast<DataValue<int>>(item1->getParameter("p")->getData());
		REQUIRE(data1 != nullptr);
		data1->set(23);
		REQUIRE(datamodel->validate() == true);
		REQUIRE(datamodel2->validate() == false);
		REQUIRE(datamodel->getRoot()->getList("l")->addItem() != nullptr);
		REQUIRE(datamodel->validate() == false);
	}

	SECTION("We can modify meta model after a data model creation")
	{
		REQUIRE(model->getRoot()->addParameter("p", "Parameter", model->getTypesDefinition()->getType("int")) != nullptr);
		REQUIRE(model->getRoot()->addList("l", "List", "Pattern") != nullptr);
		REQUIRE(model->getRoot()->getList("l")->getPattern()->addParameter("p", "Parameter", model->getTypesDefinition()->getType("int")) != nullptr);

		auto datamodel = model->createData();
		REQUIRE(datamodel != nullptr);
		REQUIRE(datamodel->getVersion() == model->getVersion());
		REQUIRE(datamodel->getRoot() != nullptr);
		REQUIRE(datamodel->getRoot()->getItems().size() == 2);
		REQUIRE(datamodel->getRoot()->getParameter("p") != nullptr);
		REQUIRE(datamodel->getRoot()->getList("l") != nullptr);
		REQUIRE(datamodel->getRoot()->getList("l")->getItems().empty());
		auto item1 = std::dynamic_pointer_cast<DataComponent>(datamodel->getRoot()->getList("l")->addItem());
		REQUIRE(item1 != nullptr);
		REQUIRE(item1->getItems().size() == 1);
		REQUIRE(item1->getParameter("p") != nullptr);
		REQUIRE(datamodel->validate() == false);

		auto data = std::dynamic_pointer_cast<DataValue<int>>(datamodel->getRoot()->getParameter("p")->getData());
		REQUIRE(data != nullptr);
		data->set(42);
		auto data1 = std::dynamic_pointer_cast<DataValue<int>>(item1->getParameter("p")->getData());
		REQUIRE(data1 != nullptr);
		data1->set(23);
		REQUIRE(datamodel->validate() == true);

		REQUIRE(model->getRoot()->addParameter("p2", "Parameter 2", model->getTypesDefinition()->getType("double")) != nullptr);
		REQUIRE(model->getRoot()->getItems().size() == 3);
		REQUIRE(datamodel->getRoot()->getItems().size() == 2);
		REQUIRE(datamodel->validate() == true);

		auto datamodel2 = model->createData();
		REQUIRE(datamodel2 != nullptr);
		REQUIRE(datamodel2->getVersion() == model->getVersion());
		REQUIRE(datamodel2->getRoot() != nullptr);
		REQUIRE(datamodel2->getRoot()->getItems().size() == 3);
		REQUIRE(datamodel2->getRoot()->getParameter("p") != nullptr);
		REQUIRE(datamodel2->getRoot()->getList("l") != nullptr);
		REQUIRE(datamodel2->getRoot()->getList("l")->getItems().empty());
		REQUIRE(datamodel2->validate() == false);
		REQUIRE(datamodel->validate() == true);

		auto data2 = std::dynamic_pointer_cast<DataValue<int>>(datamodel2->getRoot()->getParameter("p")->getData());
		REQUIRE(data2 != nullptr);
		data2->set(42);
		auto data21 = std::dynamic_pointer_cast<DataValue<double>>(datamodel2->getRoot()->getParameter("p2")->getData());
		REQUIRE(data21 != nullptr);
		data21->set(23);
		REQUIRE(datamodel2->validate() == true);
		REQUIRE(datamodel->validate() == true);
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
