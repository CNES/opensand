/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
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
 * @file PyConfiguration.cpp
 * @brief Define a Python interface to the OpenSAND configuration library
 */

#include <memory>
#include <string>
#include <vector>

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <Configuration.h>

using namespace OpenSANDConf;

using std::shared_ptr;
using std::string;
using std::vector;

struct iterable_converter
{
	template <typename Container>
	iterable_converter& from_python()
	{
		boost::python::converter::registry::push_back(&iterable_converter::convertible, &iterable_converter::construct<Container>, boost::python::type_id<Container>());

		// Support chaining.
		return *this;
	}

	static void* convertible(PyObject* object)
	{
		return PyObject_GetIter(object) ? object : NULL;
	}

	template <typename Container>
	static void construct(PyObject* object, boost::python::converter::rvalue_from_python_stage1_data* data)
	{
		boost::python::handle<> handle(boost::python::borrowed(object));

		typedef boost::python::converter::rvalue_from_python_storage<Container> storage_type;
		void* storage = reinterpret_cast<storage_type*>(data)->storage.bytes;

		typedef boost::python::stl_input_iterator<typename Container::value_type> iterator;

		new (storage) Container(iterator(boost::python::object(handle)), iterator());
		data->convertible = storage;
	}
};

BOOST_PYTHON_MODULE(py_opensand_conf)
{
	namespace python = boost::python;

	iterable_converter()
		.from_python<std::vector<bool>>()
		.from_python<std::vector<double>>()
		.from_python<std::vector<float>>()
		.from_python<std::vector<int>>()
		.from_python<std::vector<short>>()
		.from_python<std::vector<long>>()
		.from_python<std::vector<string>>()
		;

	//---------------------------------------------------------------------
	// Base
	//---------------------------------------------------------------------
	python::class_<BaseElement, shared_ptr<BaseElement>, boost::noncopyable>("BaseElement", python::no_init)
		.def("get_id", &BaseElement::getId, python::return_value_policy<python::return_by_value>())
	;

	python::class_<vector<string>>("StringVector")
		.def(python::vector_indexing_suite<vector<string>>())
	;

	python::class_<BaseEnum, shared_ptr<BaseEnum>, boost::noncopyable>("BaseEnum", python::no_init)
		.def("get_values", &BaseEnum::getValues, python::return_value_policy<python::return_by_value>())
	;

	//---------------------------------------------------------------------
	// Meta
	//---------------------------------------------------------------------
	python::class_<NamedElement, shared_ptr<NamedElement>, python::bases<BaseElement>, boost::noncopyable>("NamedElement", python::no_init)
		.def("get_name", &NamedElement::getName, python::return_value_policy<python::return_by_value>())
		.def("get_description", &NamedElement::getDescription, python::return_value_policy<python::return_by_value>())
		.def("set_description", &NamedElement::setDescription)
	;

	python::class_<MetaType, shared_ptr<MetaType>, python::bases<NamedElement>, boost::noncopyable>("MetaType", python::no_init)
	;

	python::class_<MetaValueType<bool>, shared_ptr<MetaValueType<bool>>, python::bases<MetaType>, boost::noncopyable>("MetaBoolType", python::no_init)
	;

	python::class_<MetaValueType<double>, shared_ptr<MetaValueType<double>>, python::bases<MetaType>, boost::noncopyable>("MetaDoubleType", python::no_init)
	;

	python::class_<MetaValueType<float>, shared_ptr<MetaValueType<float>>, python::bases<MetaType>, boost::noncopyable>("MetaFloatType", python::no_init)
	;

	python::class_<MetaValueType<int>, shared_ptr<MetaValueType<int>>, python::bases<MetaType>, boost::noncopyable>("MetaIntType", python::no_init)
	;

	python::class_<MetaValueType<short>, shared_ptr<MetaValueType<short>>, python::bases<MetaType>, boost::noncopyable>("MetaShortType", python::no_init)
	;

	python::class_<MetaValueType<long>, shared_ptr<MetaValueType<long>>, python::bases<MetaType>, boost::noncopyable>("MetaLongType", python::no_init)
	;

	python::class_<MetaValueType<string>, shared_ptr<MetaValueType<string>>, python::bases<MetaType>, boost::noncopyable>("MetaStringType", python::no_init)
	;

	python::class_<MetaEnumType, shared_ptr<MetaEnumType>, python::bases<MetaValueType<string>, BaseEnum>, boost::noncopyable>("MetaEnumType", python::no_init)
	;

	python::class_<vector<shared_ptr<MetaType>>>("MetaTypeVector")
		.def(python::vector_indexing_suite<vector<shared_ptr<MetaType>>>())
	;

	python::class_<vector<shared_ptr<MetaEnumType>>>("MetaEnumTypeVector")
		.def(python::vector_indexing_suite<vector<shared_ptr<MetaEnumType>>>())
	;

	shared_ptr<MetaEnumType> (MetaTypesList::*addMetaEnumTypeWithoutDesc)(const string &, const string &, const vector<string> &) = &MetaTypesList::addEnumType;
	shared_ptr<MetaEnumType> (MetaTypesList::*addMetaEnumTypeWithDesc)(const string &, const string &, const vector<string> &, const string &) = &MetaTypesList::addEnumType;

	python::class_<MetaTypesList, shared_ptr<MetaTypesList>, boost::noncopyable>("MetaTypesList", python::no_init)
		.def("get_types", &MetaTypesList::getTypes)
		.def("get_type", &MetaTypesList::getType)
		.def("get_enum_types", &MetaTypesList::getEnumTypes)
		.def("add_enum_type", addMetaEnumTypeWithoutDesc)
		.def("add_enum_type", addMetaEnumTypeWithDesc)
	;

	python::class_<MetaElement, shared_ptr<MetaElement>, python::bases<NamedElement>, boost::noncopyable>("MetaElement", python::no_init)
		.def("get_path", &MetaElement::getPath)
		.def("is_advanced", &MetaElement::isAdvanced)
		.def("set_advanced", &MetaElement::setAdvanced)
		.def("is_read_only", &MetaElement::isReadOnly)
		.def("set_read_only", &MetaElement::setReadOnly)
		.def("get_reference_target", &MetaElement::getReferenceTarget)
		.def("get_reference_data", &MetaElement::getReferenceData)
	;

	python::class_<vector<shared_ptr<MetaElement>>>("MetaElementVector")
		.def(python::vector_indexing_suite<vector<shared_ptr<MetaElement>>>())
	;

	python::class_<MetaParameter, shared_ptr<MetaParameter>, python::bases<MetaElement>, boost::noncopyable>("MetaParameter", python::no_init)
		.def("get_type", &MetaParameter::getType)
		.def("get_unit", &MetaParameter::getUnit, python::return_value_policy<python::return_by_value>())
		.def("set_unit", &MetaParameter::setUnit)
	;

	python::class_<MetaList, shared_ptr<MetaList>, python::bases<MetaElement>, boost::noncopyable>("MetaList", python::no_init)
		.def("get_pattern", &MetaList::getPattern)
	;

	shared_ptr<MetaParameter> (MetaComponent::*addMetaParameterWithoutDesc)(const string &, const string &, shared_ptr<MetaType>) = &MetaComponent::addParameter;
	shared_ptr<MetaParameter> (MetaComponent::*addMetaParameterWithDesc)(const string &, const string &, shared_ptr<MetaType>, const string &) = &MetaComponent::addParameter;

	shared_ptr<MetaList> (MetaComponent::*addMetaListWithoutDesc)(const string &, const string &, const string &) = &MetaComponent::addList;
	shared_ptr<MetaList> (MetaComponent::*addMetaListWithoutPatternDesc)(const string &, const string &, const string &, const string &) = &MetaComponent::addList;
	shared_ptr<MetaList> (MetaComponent::*addMetaListWithDesc)(const string &, const string &, const string &, const string &, const string &) = &MetaComponent::addList;

	shared_ptr<MetaComponent> (MetaComponent::*addMetaComponentWithoutDesc)(const string &, const string &) = &MetaComponent::addComponent;
	shared_ptr<MetaComponent> (MetaComponent::*addMetaComponentWithDesc)(const string &, const string &, const string &) = &MetaComponent::addComponent;

	python::class_<MetaComponent, shared_ptr<MetaComponent>, python::bases<MetaElement>, boost::noncopyable>("MetaComponent", python::no_init)
		.def("get_items", &MetaComponent::getItems, python::return_value_policy<python::return_by_value>())
		.def("get_item", &MetaComponent::getItem)
		.def("get_parameter", &MetaComponent::getParameter)
		.def("add_parameter", addMetaParameterWithoutDesc)
		.def("add_parameter", addMetaParameterWithDesc)
		.def("get_list", &MetaComponent::getList)
		.def("add_list", addMetaListWithoutDesc)
		.def("add_list", addMetaListWithoutPatternDesc)
		.def("add_list", addMetaListWithDesc)
		.def("get_component", &MetaComponent::getComponent)
		.def("add_component", addMetaComponentWithoutDesc)
		.def("add_component", addMetaComponentWithDesc)
	;

	python::class_<MetaModel, shared_ptr<MetaModel>>("MetaModel", python::init<string>())
		.def("get_version", &MetaModel::getVersion, python::return_value_policy<python::return_by_value>())
		.def("get_types_definition", &MetaModel::getTypesDefinition)
		.def("get_root", &MetaModel::getRoot)
		.def("get_item_by_path", &MetaModel::getItemByPath)
		.def("create_data", &MetaModel::createData)
		.def("set_reference", &MetaModel::setReference)
		.def("clone", &DataModel::clone)
	;

	//---------------------------------------------------------------------
	// Data
	//---------------------------------------------------------------------
	python::class_<Data, shared_ptr<Data>, boost::noncopyable>("Data", python::no_init)
		.def("is_set", &Data::isSet)
		.def("reset", &Data::reset)
		.def("__str__", python::pure_virtual(&Data::toString))
		.def("from_string", python::pure_virtual(&Data::fromString))
	;

	python::class_<DataValue<bool>, shared_ptr<DataValue<bool>>, python::bases<Data>, boost::noncopyable>("DataBool", python::no_init)
		.def("get", &DataValue<bool>::get)
		.def("set", &DataValue<bool>::set)
		.def("__str__", &DataValue<bool>::toString)
		.def("from_string", &DataValue<bool>::fromString)
	;

	python::class_<DataValue<double>, shared_ptr<DataValue<double>>, python::bases<Data>, boost::noncopyable>("DataDouble", python::no_init)
		.def("get", &DataValue<double>::get)
		.def("set", &DataValue<double>::set)
		.def("__str__", &DataValue<double>::toString)
		.def("from_string", &DataValue<double>::fromString)
	;

	python::class_<DataValue<float>, shared_ptr<DataValue<float>>, python::bases<Data>, boost::noncopyable>("DataFloat", python::no_init)
		.def("get", &DataValue<float>::get)
		.def("set", &DataValue<float>::set)
		.def("__str__", &DataValue<float>::toString)
		.def("from_string", &DataValue<float>::fromString)
	;

	python::class_<DataValue<int8_t>, shared_ptr<DataValue<int8_t>>, python::bases<Data>, boost::noncopyable>("DataByte", python::no_init)
		.def("get", &DataValue<int8_t>::get)
		.def("set", &DataValue<int8_t>::set)
		.def("__str__", &DataValue<int8_t>::toString)
		.def("from_string", &DataValue<int8_t>::fromString)
	;

	python::class_<DataValue<int16_t>, shared_ptr<DataValue<int16_t>>, python::bases<Data>, boost::noncopyable>("DataShort", python::no_init)
		.def("get", &DataValue<int16_t>::get)
		.def("set", &DataValue<int16_t>::set)
		.def("__str__", &DataValue<int16_t>::toString)
		.def("from_string", &DataValue<int16_t>::fromString)
	;

	python::class_<DataValue<int32_t>, shared_ptr<DataValue<int32_t>>, python::bases<Data>, boost::noncopyable>("DataInt", python::no_init)
		.def("get", &DataValue<int32_t>::get)
		.def("set", &DataValue<int32_t>::set)
		.def("__str__", &DataValue<int32_t>::toString)
		.def("from_string", &DataValue<int32_t>::fromString)
	;

	python::class_<DataValue<int64_t>, shared_ptr<DataValue<int64_t>>, python::bases<Data>, boost::noncopyable>("DataLong", python::no_init)
		.def("get", &DataValue<int64_t>::get)
		.def("set", &DataValue<int64_t>::set)
		.def("__str__", &DataValue<int64_t>::toString)
		.def("from_string", &DataValue<int64_t>::fromString)
	;

	python::class_<DataValue<uint8_t>, shared_ptr<DataValue<uint8_t>>, python::bases<Data>, boost::noncopyable>("DataUnsignedByte", python::no_init)
		.def("get", &DataValue<uint8_t>::get)
		.def("set", &DataValue<uint8_t>::set)
		.def("__str__", &DataValue<uint8_t>::toString)
		.def("from_string", &DataValue<uint8_t>::fromString)
	;

	python::class_<DataValue<uint16_t>, shared_ptr<DataValue<uint16_t>>, python::bases<Data>, boost::noncopyable>("DataUnsignedShort", python::no_init)
		.def("get", &DataValue<uint16_t>::get)
		.def("set", &DataValue<uint16_t>::set)
		.def("__str__", &DataValue<uint16_t>::toString)
		.def("from_string", &DataValue<uint16_t>::fromString)
	;

	python::class_<DataValue<uint32_t>, shared_ptr<DataValue<uint32_t>>, python::bases<Data>, boost::noncopyable>("DataUnsignedInt", python::no_init)
		.def("get", &DataValue<uint32_t>::get)
		.def("set", &DataValue<uint32_t>::set)
		.def("__str__", &DataValue<uint32_t>::toString)
		.def("from_string", &DataValue<uint32_t>::fromString)
	;

	python::class_<DataValue<uint64_t>, shared_ptr<DataValue<uint64_t>>, python::bases<Data>, boost::noncopyable>("DataUnsignedLong", python::no_init)
		.def("get", &DataValue<uint64_t>::get)
		.def("set", &DataValue<uint64_t>::set)
		.def("__str__", &DataValue<uint64_t>::toString)
		.def("from_string", &DataValue<uint64_t>::fromString)
	;

	python::class_<DataValue<string>, shared_ptr<DataValue<string>>, python::bases<Data>, boost::noncopyable>("DataString", python::no_init)
		.def("get", &DataValue<string>::get)
		.def("set", &DataValue<string>::set)
		.def("__str__", &DataValue<string>::toString)
		.def("from_string", &DataValue<string>::fromString)
	;

	python::class_<DataElement, shared_ptr<DataElement>, python::bases<BaseElement>, boost::noncopyable>("DataElement", python::no_init)
		.def("get_path", &DataElement::getPath)
		.def("check_reference", &DataElement::checkReference)
	;

	python::class_<vector<shared_ptr<DataElement>>>("DataElementVector")
		.def(python::vector_indexing_suite<vector<shared_ptr<DataElement>>>())
	;

	python::class_<DataParameter, shared_ptr<DataParameter>, python::bases<DataElement>, boost::noncopyable>("DataParameter", python::no_init)
		.def("get_data", &DataParameter::getData)
	;

	python::class_<DataList, shared_ptr<DataList>, python::bases<DataElement>, boost::noncopyable>("DataList", python::no_init)
		.def("get_items", &DataList::getItems, python::return_value_policy<python::return_by_value>())
		.def("get_item", &DataList::getItem)
		.def("add_item", &DataList::addItem)
		.def("clear_items", &DataList::clearItems)
	;

	python::class_<DataComponent, shared_ptr<DataComponent>, python::bases<DataElement>, boost::noncopyable>("DataComponent", python::no_init)
		.def("get_items", &DataComponent::getItems, python::return_value_policy<python::return_by_value>())
		.def("get_item", &DataComponent::getItem)
		.def("get_parameter", &DataComponent::getParameter)
		.def("get_list", &DataComponent::getList)
		.def("get_component", &DataComponent::getComponent)
	;

	python::class_<DataModel, shared_ptr<DataModel>>("DataModel", python::no_init)
		.def("get_version", &DataModel::getVersion, python::return_value_policy<python::return_by_value>())
		.def("get_root", &DataModel::getRoot)
		.def("get_item_by_path", &DataModel::getItemByPath)
		.def("validate", &DataModel::validate)
		.def("clone", &DataModel::clone)
	;

	python::def("toXSD", toXSD);
	python::def("fromXSD", fromXSD);

	python::def("toXML", toXML);
	python::def("fromXML", fromXML);
}
