#define CATCH_CONFIG_MAIN

#include "catch.hpp"

#include <memory>
#include <initializer_list>
#include <iostream>

#include <MetaModel.h>
#include <DataModel.h>

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

TEST_CASE("Truth test", "[Common]")
{
	CHECK(true);
	CHECK_FALSE(false);
	REQUIRE(1 == 1);
}

TEST_CASE("Model tests", "[Model]")
{
	string version = "1.0.0";
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
		string newDescription = "newDescription";
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
		std::initializer_list<string> vals = {"val1", "val2", "val3"};
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
		std::initializer_list<string> vals = {"val1", "val2", "val3"};

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
		std::initializer_list<string> vals = {"val1", "val1"};
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
	string version = "1.0.0";
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
	string version = "1.0.0";
	auto model = std::make_shared<MetaModel>(version);
	REQUIRE(model != nullptr);

	SECTION("We can add parameter")
	{
		SECTION("We can add a boolean parameter")
		{
			string id = "id1";
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
				string str1 = "false";
				bool val2 = true;
				string str2 = "true";
				string invalid = "42";

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
			string id = "id1";
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
				string str1 = "42";
				int val2 = 23;
				string str2 = "23";
				int val3 = 86;
				string str3 = "86.2";
				string str3b = "86";
				string invalid = "azerty";
				//string invalid = "42.23azerty"; // FIXME? fromString succeeds but would not

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
			string id = "id1";
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
				string str1 = "42.42";
				double val2 = 23.23;
				string str2 = "23.23";
				double val3 = 1.12e3;
				string str3 = "1.12e3";
				string str3b = "1120";
				int val4 = 86;
				string str4 = "86";
				string invalid = "azerty";

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
				REQUIRE(data->toString() == str4);

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
			string id = "id1";
			string val1 = "value 1";
			string val2 = "value 2";

			auto param = model->getRoot()->addParameter(id, "Parameter 1", model->getTypesDefinition()->getType("string"));
			REQUIRE(param != nullptr);
			REQUIRE(std::dynamic_pointer_cast<MetaValueType<string>>(param->getType()) != nullptr);
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
			auto data = std::dynamic_pointer_cast<DataValue<string>>(dataparam->getData());
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
				string val1 = "42.42azerty";
				string str1 = val1;
				string val2 = "23.23!?*";
				string str2 = val2;

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
			string id = "id1";
			string val1 = "value 1";
			string val2 = "value 2";
			string invalid = "value 3";

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
			auto data = std::dynamic_pointer_cast<DataValue<string>>(dataparam->getData());
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
		auto data1 = std::dynamic_pointer_cast<DataValue<string>>(dataparam1->getData());
		REQUIRE(data1 != nullptr);
		auto data2 = std::dynamic_pointer_cast<DataValue<string>>(dataparam2->getData());
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
		auto data1 = std::dynamic_pointer_cast<DataValue<string>>(dataparam1->getData());
		REQUIRE(data1 != nullptr);
		auto data2 = std::dynamic_pointer_cast<DataValue<string>>(dataparam2->getData());
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

		string desc = "This is a description";
		param2->setDescription(desc);
		REQUIRE(param->getDescription() == desc);
	}
}

TEST_CASE("Model list tests", "[Model][List]")
{
	string version = "1.0.0";
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
		string desc = "This is a description";
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
		auto i1d1 = std::dynamic_pointer_cast<DataValue<string>>(i1p1->getData());
		REQUIRE(i1d1 != nullptr);
		auto i2p1 = item2->getParameter("p1");
		REQUIRE(i2p1 != nullptr);
		auto i2d1 = std::dynamic_pointer_cast<DataValue<string>>(i2p1->getData());
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
		auto i1d1 = std::dynamic_pointer_cast<DataValue<string>>(i1p1->getData());
		REQUIRE(i1d1 != nullptr);
		auto i2p1 = item2->getParameter("p1");
		REQUIRE(i2p1 != nullptr);
		auto i2d1 = std::dynamic_pointer_cast<DataValue<string>>(i2p1->getData());
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
		auto data = std::static_pointer_cast<DataValue<string>>(dataparam->getData());
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
		auto data2 = std::dynamic_pointer_cast<DataValue<string>>(dataparam2->getData());
		REQUIRE(data2 != nullptr);
		REQUIRE(data2->isSet() == false);
	}
}

TEST_CASE("Model data tests", "[Model][Data]")
{
	string version = "1.0.0";
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

TEST_CASE("Model reference tests", "[Model][Reference]")
{
	string version = "1.0.0";
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
			auto expected = std::dynamic_pointer_cast<DataValue<string>>(cpt->getReferenceData());
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
			auto data = std::dynamic_pointer_cast<DataValue<string>>(datatarget->getData());
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
			auto data2 = std::dynamic_pointer_cast<DataValue<string>>(datatarget2->getData());
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
			auto expected = std::dynamic_pointer_cast<DataValue<string>>(cpt->getReferenceData());
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
			auto data = std::dynamic_pointer_cast<DataValue<string>>(datatarget->getData());
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
			auto data2 = std::dynamic_pointer_cast<DataValue<string>>(datatarget2->getData());
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
			string elementpath2 = "";
			auto res2 = false;

			// Configure reference
			REQUIRE(target != nullptr);
			REQUIRE(element != nullptr);
			REQUIRE(model->setReference(element, target));
			REQUIRE(element->getReferenceTarget() == target);
			REQUIRE(element->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<string>>(element->getReferenceData());
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
			auto data = std::dynamic_pointer_cast<DataValue<string>>(datatarget->getData());
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
			string elementpath2 = "";
			auto res2 = false;

			// Configure reference
			REQUIRE(target != nullptr);
			REQUIRE(element != nullptr);
			REQUIRE(model->setReference(element, target));
			REQUIRE(element->getReferenceTarget() == target);
			REQUIRE(element->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<string>>(element->getReferenceData());
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
			auto data = std::dynamic_pointer_cast<DataValue<string>>(datatarget->getData());
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
			string elementpath2 = "/c/c2/l3/0/s4";
			auto res2 = true;

			// Configure reference
			REQUIRE(target != nullptr);
			REQUIRE(element != nullptr);
			REQUIRE(model->setReference(element, target));
			REQUIRE(element->getReferenceTarget() == target);
			REQUIRE(element->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<string>>(element->getReferenceData());
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
			auto data = std::dynamic_pointer_cast<DataValue<string>>(datatarget->getData());
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
			string elementpath2 = "/c/c2/l3/0/s4";
			auto res2 = false;

			// Configure reference
			REQUIRE(target != nullptr);
			REQUIRE(element != nullptr);
			REQUIRE(model->setReference(element, target));
			REQUIRE(element->getReferenceTarget() == target);
			REQUIRE(element->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<string>>(element->getReferenceData());
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
			auto data = std::dynamic_pointer_cast<DataValue<string>>(datatarget->getData());
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
			string elementpath2 = "/c/c2/l3/1/l4/1/s5";
			auto res2 = false;

			// Configure reference
			REQUIRE(target != nullptr);
			REQUIRE(element != nullptr);
			REQUIRE(model->setReference(element, target));
			REQUIRE(element->getReferenceTarget() == target);
			REQUIRE(element->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<string>>(element->getReferenceData());
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
			auto data = std::dynamic_pointer_cast<DataValue<string>>(datatarget->getData());
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
			string elementpath2 = "/c/c2/l3/1/l4/1/s5";
			auto res2 = true;

			// Configure reference
			REQUIRE(target != nullptr);
			REQUIRE(element != nullptr);
			REQUIRE(model->setReference(element, target));
			REQUIRE(element->getReferenceTarget() == target);
			REQUIRE(element->getReferenceData() != nullptr);
			auto expected = std::dynamic_pointer_cast<DataValue<string>>(element->getReferenceData());
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
			auto data = std::dynamic_pointer_cast<DataValue<string>>(datatarget->getData());
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
			vector<string> value_paths = {
				"/e",
				"/s",
				"/c/e2",
				"/c/s2",
				"/c/l2/0/e3",
				"/c/l2/0/s3",
				"/c/l2/1/e3",
				"/c/l2/1/s3",
			};
			vector<string> direct_referenced_paths = {
			};
			vector<string> indirect_referenced_paths = {
			};
			shared_ptr<MetaParameter> target = nullptr;
			shared_ptr<MetaElement> element = nullptr;
			string targetpath = "";

			// Add reference
			if(target != nullptr)
			{
				REQUIRE(element != nullptr);
				REQUIRE(model->setReference(element, target));
				REQUIRE(element->getReferenceTarget() == target);
				REQUIRE(element->getReferenceData() != nullptr);
				auto expected = std::dynamic_pointer_cast<DataValue<string>>(element->getReferenceData());
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
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
				REQUIRE(data->set("val2"));
			}
			for(auto path: direct_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == false);
			}
			for(auto path: indirect_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
			}
			REQUIRE(datamodel->validate() == true);
			if(!targetpath.empty())
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(targetpath));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->set("val1"));
				REQUIRE(datamodel->validate() == false);
			}
		}

		SECTION("Check validity of a datamodel with a lower element with reference")
		{
			vector<string> value_paths = {
				"/e",
				"/s",
				"/c/e2",
				"/c/l2/0/e3",
				"/c/l2/0/s3",
				"/c/l2/1/e3",
				"/c/l2/1/s3",
			};
			vector<string> direct_referenced_paths = {
				"/c/s2",
			};
			vector<string> indirect_referenced_paths = {
			};
			shared_ptr<MetaParameter> target = std::dynamic_pointer_cast<MetaParameter>(model->getItemByPath("/e"));
			shared_ptr<MetaElement> element = model->getItemByPath("/c/s2");
			string targetpath = target->getPath();

			// Add reference
			if(target != nullptr)
			{
				REQUIRE(element != nullptr);
				REQUIRE(model->setReference(element, target));
				REQUIRE(element->getReferenceTarget() == target);
				REQUIRE(element->getReferenceData() != nullptr);
				auto expected = std::dynamic_pointer_cast<DataValue<string>>(element->getReferenceData());
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
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
				REQUIRE(data->set("val2"));
			}
			for(auto path: direct_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == false);
			}
			for(auto path: indirect_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
			}
			REQUIRE(datamodel->validate() == true);
			if(!targetpath.empty())
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(targetpath));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->set("val1"));
				REQUIRE(datamodel->validate() == false);
			}
		}

		SECTION("Check validity of a datamodel with a upper element with reference")
		{
			vector<string> value_paths = {
				"/e",
				"/s",
				"/c/e2",
				"/c/s2",
			};
			vector<string> direct_referenced_paths = {
			};
			vector<string> indirect_referenced_paths = {
				"/c/l2/0/e3",
				"/c/l2/0/s3",
				"/c/l2/1/e3",
				"/c/l2/1/s3",
			};
			shared_ptr<MetaParameter> target = std::dynamic_pointer_cast<MetaParameter>(model->getItemByPath("/e"));
			shared_ptr<MetaElement> element = model->getItemByPath("/c");
			string targetpath = target->getPath();

			// Add reference
			if(target != nullptr)
			{
				REQUIRE(element != nullptr);
				REQUIRE(model->setReference(element, target));
				REQUIRE(element->getReferenceTarget() == target);
				REQUIRE(element->getReferenceData() != nullptr);
				auto expected = std::dynamic_pointer_cast<DataValue<string>>(element->getReferenceData());
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
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
				REQUIRE(data->set("val2"));
			}
			for(auto path: direct_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == false);
			}
			for(auto path: indirect_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
			}
			REQUIRE(datamodel->validate() == true);
			if(!targetpath.empty())
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(targetpath));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->set("val1"));
				REQUIRE(datamodel->validate() == false);
			}
		}

		SECTION("Check validity of a datamodel with an element in a list item with reference")
		{
			vector<string> value_paths = {
				"/e",
				"/s",
				"/c/e2",
				"/c/s2",
				"/c/l2/0/e3",
				"/c/l2/1/e3",
			};
			vector<string> direct_referenced_paths = {
				"/c/l2/0/s3",
				"/c/l2/1/s3",
			};
			vector<string> indirect_referenced_paths = {
			};
			shared_ptr<MetaParameter> target = std::dynamic_pointer_cast<MetaParameter>(model->getItemByPath("/e"));
			shared_ptr<MetaElement> element = model->getItemByPath("/c/l2/*/s3");
			string targetpath = target->getPath();

			// Add reference
			if(target != nullptr)
			{
				REQUIRE(element != nullptr);
				REQUIRE(model->setReference(element, target));
				REQUIRE(element->getReferenceTarget() == target);
				REQUIRE(element->getReferenceData() != nullptr);
				auto expected = std::dynamic_pointer_cast<DataValue<string>>(element->getReferenceData());
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
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
				REQUIRE(data->set("val2"));
			}
			for(auto path: direct_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == false);
			}
			for(auto path: indirect_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
			}
			REQUIRE(datamodel->validate() == true);
			if(!targetpath.empty())
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(targetpath));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->set("val1"));
				REQUIRE(datamodel->validate() == false);
			}
		}

		SECTION("Check validity of a datamodel with an element in a list item with reference in list pattern")
		{
			vector<string> value_paths = {
				"/e",
				"/s",
				"/c/e2",
				"/c/s2",
				"/c/l2/0/e3",
				"/c/l2/1/e3",
			};
			vector<string> direct_referenced_paths = {
				"/c/l2/0/s3",
				"/c/l2/1/s3",
			};
			vector<string> indirect_referenced_paths = {
			};
			shared_ptr<MetaParameter> target = std::dynamic_pointer_cast<MetaParameter>(model->getItemByPath("/c/l2/*/e3"));
			shared_ptr<MetaElement> element = model->getItemByPath("/c/l2/*/s3");
			string targetpath = "/c/l2/1/e3";

			// Add reference
			if(target != nullptr)
			{
				REQUIRE(element != nullptr);
				REQUIRE(model->setReference(element, target));
				REQUIRE(element->getReferenceTarget() == target);
				REQUIRE(element->getReferenceData() != nullptr);
				auto expected = std::dynamic_pointer_cast<DataValue<string>>(element->getReferenceData());
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
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
				REQUIRE(data->set("val2"));
			}
			for(auto path: direct_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == false);
			}
			for(auto path: indirect_referenced_paths)
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(path));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == false);
				REQUIRE(param->checkReference() == true);
			}
			REQUIRE(datamodel->validate() == true);
			if(!targetpath.empty())
			{
				auto param = std::dynamic_pointer_cast<DataParameter>(datamodel->getItemByPath(targetpath));
				REQUIRE(param != nullptr);
				auto data = std::dynamic_pointer_cast<DataValue<string>>(param->getData());
				REQUIRE(data != nullptr);
				REQUIRE(data->isSet() == true);
				REQUIRE(data->set("val1"));
				REQUIRE(datamodel->validate() == false);
			}
		}
	}
	
}

//TEST_CASE( "Simple Model configuration test", "[Environment][Model][Configuration]" ) {
//    auto env = new Environment();
//    auto model = new Model("1.0", "simpleModel", "Model", "Describes my Model.");
//    model->setEnvironment(env);
//
//    env->getTypesDefinition()->addEnumType("e", "enum", "My custom enum.");
//
//    model->addList("l", "list", "My custom list.");
//    model->addComponent("c", "component", "My custom component.");
//    model->addParameter("e", "pe", "parameter e", "My custom parameter e.");
//    model->addParameter(FLOAT, "pf", "parameter f", "My custom parameter f.")->setValue<Float>(5.0f);
//
//    CHECK( Utils::toXSD(model, "simpleModel.xsd") );
//
//    Environment * envLoaded = nullptr;
//    Model * mmLoaded = nullptr;
//
//    CHECK( Utils::fromXSD(&envLoaded, &mmLoaded, "simpleModel.xsd") );
//
//    SECTION( "Environment and Model are loaded" ) {
//        CHECK( envLoaded != nullptr );
//        CHECK( mmLoaded != nullptr );
//    }
//
//    SECTION( "Check that the pointers are different" ) {
//        CHECK( envLoaded != env );
//        CHECK( mmLoaded != model );
//    }
//
//    SECTION( "Check that the content are the same" ) {
//        CHECK( env->isSame(envLoaded) );
//        CHECK( model->isSame(mmLoaded) );
//    }
//
//    SECTION( "Changing one environment makes them different" ) {
//        env->getEnumType("e")->setName("ENUM");
//
//        CHECK_FALSE( env->isSame(envLoaded) );
//    }
//
//    SECTION( "Changing one model makes them different (environment stay same though)" ) {
//        model->getRootComponent()->getParameter("pf")->setName("parameter changed");
//
//        CHECK( env->isSame(envLoaded) );
//        CHECK_FALSE( model->isSame(mmLoaded) );
//    }
//
//    CHECK( Utils::toXML(model, "simpleModel.xml") );
//    CHECK( Utils::fromXML(&envLoaded, &mmLoaded, "simpleModel.xml", "simpleModel.xsd") );
//
//    delete mmLoaded;
//    delete envLoaded;
//
//    delete model;
//    delete env;
//}
//
//TEST_CASE( "Complex Model configuration test", "[Environment][Model][Configuration]" ) {
//    auto env = new Environment();
//    auto model = new Model("5.1.2", "complexModel", "Model", "Describes my Model.");
//    model->setEnvironment(env);
//
//    auto e = env->getTypesDefinition()->addEnumType("e", "Enum", "...");
//    e->addValue("A");
//    e->addValue("B");
//    e->addValue("C");
//
//    auto p1 = model->addParameter(FLOAT, "p1", "Parameter 1", "...");
//    auto p2 = model->addParameter(INT, "p2", "Parameter 2", "...");
//    p2->setDefaultValue<Int>(0);
//    auto p3 = model->addParameter("e", "p3", "Parameter 3", "...");
//    p3->setDefaultValue<String>("B");
//    auto p4 = model->addParameter(DOUBLE, "p4", "Parameter 4", "...");
//    auto p5 = model->addParameter(STRING, "p5", "Parameter 5", "...");
//
//    auto c1 = model->addComponent("c1", "Component 1", "...");
//    auto c2 = model->addComponent("c2", "Component 2", "...");
//    auto c3 = model->addComponent("c3", "Component 3", "...");
//
//    auto l1 = model->addList("l1", "List 1", "...", "Pattern 1");
//    l1->getPattern()->addParameter(INT, "p1", "Parameter 1", "...");
//    auto l1p2 = l1->getPattern()->addParameter("e", "p2", "Parameter 2", "...");
//    l1p2->setVisibility(ADVANCED);
//    l1->getPattern()->addParameter(FLOAT, "p3", "Parameter 3", "...");
//    auto l2 = model->addList("l2", "List 2", "...", "Pattern 2");
//    l2->getPattern()->addParameter(FLOAT, "p1", "Parameter 1", "...");
//    l2->getPattern()->addParameter(LONGDOUBLE, "p2", "Parameter 2", "...");
//    l2->getPattern()->addParameter("e", "p3", "Parameter 3", "...");
//    l2->getPattern()->addParameter(DOUBLE, "p4", "Parameter 4", "...");
//    l2->getPattern()->addParameter("e", "p5", "Parameter 5", "...");
//
//    auto c1c1 = c1->addComponent("c1c1", "C1 Component 1", "...");
//    c1c1->addParameter(INT, "p1", "Parameter 1", "...");
//    auto c1c1p2 = c1c1->addParameter("e", "p2", "Parameter 2", "...");
//    auto c1c1p3 = c1c1->addParameter(INT, "p3", "Parameter 3", "...");
//    c1c1p3->setReference(c1c1p2, "D");
//    auto c1c2 = c1->addComponent("c1c2", "C1 Component 2", "...");
//    c1c2->addParameter(LONGDOUBLE, "p1", "Parameter 1", "...");
//    c1c2->addParameter(FLOAT, "p2", "Parameter 2", "...");
//    auto c1c3 = c1->addComponent("c1c3", "C1 Component 3", "...");
//    c1c3->addParameter(DOUBLE, "p1", "Parameter 1", "...");
//    c1c3->addParameter(INT, "p2", "Parameter 2", "...");
//
//    auto c2c1 = c2->addComponent("c2c1", "C2 Component 1", "...");
//    c2c1->addParameter(INT, "p1", "Parameter 1", "...");
//    c2c1->addParameter(STRING, "p2", "Parameter 2", "...");
//    auto c2c2 = c2->addComponent("c2c2", "C2 Component 2", "...");
//    c2c2->addParameter(STRING, "p1", "Parameter 1", "...");
//    c2c2->addParameter(LONGDOUBLE, "p2", "Parameter 2", "...");
//    c2c2->addParameter(INT, "p3", "Parameter 3", "...");
//
//    auto c2c1c1 = c2c1->addComponent("c2c1c1", "C2C1 Component 1", "...");
//    auto c2c1c1c1 = c2c1c1->addComponent("c2c1c1c1", "C2C1C1 Component 1", "...");
//    auto c2c1c1c1c1 = c2c1c1c1->addComponent("c2c1c1c1c1", "C2C1C1C1 Component 1", "...");
//    c2c1c1c1c1->addParameter(FLOAT, "p1", "Parameter 1", "...");
//    auto c2c1c1c1c1p2 = c2c1c1c1c1->addParameter(STRING, "p2", "Parameter 2", "...");
//
//    CHECK( Utils::toXSD(model, "complexModel.xsd") );
//
//    Environment * envLoaded(nullptr);
//    Model * mmLoaded(nullptr);
//
//    CHECK( Utils::fromXSD(&envLoaded, &mmLoaded, "complexModel.xsd") );
//
//    SECTION( "Check that the content are the same" ) {
//        CHECK( env->isSame(envLoaded) );
//        CHECK( model->isSame(mmLoaded) );
//    }
//
//    SECTION( "Check parameter visibility" ) {
//        REQUIRE( l1p2->getVisibility() == ADVANCED );
//        REQUIRE( c2->getVisibility() == NORMAL );
//    }
//
//    SECTION( "Check parameter visibility after model loading" ) {
//        REQUIRE( mmLoaded->getRootComponent()->getList("l1")->getPattern()->getParameter("p2")->getVisibility() == ADVANCED );
//        REQUIRE( mmLoaded->getRootComponent()->getComponent("c2")->getVisibility() == NORMAL );
//    }
//
//    SECTION( "Check that P2 & P3 default values are loaded" ) {
//        REQUIRE( mmLoaded->getRootComponent()->getParameter("p2")->getDefaultValue<Int>() == 0 );
//        REQUIRE( mmLoaded->getRootComponent()->getParameter("p3")->getDefaultValue<String>() == "B" );
//    }
//
//    SECTION( "Check parameters path is correct" ) {
//        REQUIRE( l1p2->getPath() == "/L:l1/C:*/P:p2" );
//        REQUIRE( c2c1c1c1c1p2->getPath() == "/C:c2/C:c2c1/C:c2c1c1/C:c2c1c1c1/C:c2c1c1c1c1/P:p2" );
//    }
//
//    SECTION( "Check parameters path is correct after model loading" ) {
//        REQUIRE( mmLoaded->getRootComponent()->getList("l1")->getPattern()->getParameter("p2")->getPath() == "/L:l1/C:*/P:p2" );
//        REQUIRE( mmLoaded->getRootComponent()->getComponent("c2")
//                                             ->getComponent("c2c1")
//                                             ->getComponent("c2c1c1")
//                                             ->getComponent("c2c1c1c1")
//                                             ->getComponent("c2c1c1c1c1")
//                                             ->getParameter("p2")->getPath() == "/C:c2/C:c2c1/C:c2c1c1/C:c2c1c1c1/C:c2c1c1c1c1/P:p2" );
//    }
//
//    SECTION( "Check parameter 'C1C1P3' reference is correct" ) {
//        REQUIRE( c1c1p3->getReference().first == "/C:c1/C:c1c1/P:p2" );
//        REQUIRE( c1c1p3->getReference().second == "D" );
//    }
//
//    SECTION( "Check parameter 'C1C1P3' reference is correct after model loading" ) {
//        REQUIRE( mmLoaded->getRootComponent()->getComponent("c1")
//                                             ->getComponent("c1c1")
//                                             ->getParameter("p3")
//                                             ->getReference().first == "/C:c1/C:c1c1/P:p2" );
//        REQUIRE( mmLoaded->getRootComponent()->getComponent("c1")
//                                             ->getComponent("c1c1")
//                                             ->getParameter("p3")
//                                             ->getReference().second == "D" );
//    }
//
//    CHECK( Utils::toXML(model, "complexModel.xml") );
//    CHECK( Utils::fromXML(&envLoaded, &mmLoaded, "complexModel.xml", "complexModel.xsd") );
//
//    delete mmLoaded;
//    delete envLoaded;
//
//    delete model;
//    delete env;
//}

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
