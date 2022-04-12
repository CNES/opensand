#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import unittest
import os
import os.path
import py_opensand_conf as OpenSandConf

class ModelTests(unittest.TestCase):
    def setUp(self):
        self.version = "1.2.3"
        self.model = OpenSandConf.MetaModel(self.version)

    def test_meta_creation(self):
        self.assertIsNotNone(self.model)
        self.assertEqual(self.model.get_version(), self.version)
        self.assertIsNotNone(self.model.get_types_definition())
        self.assertIsNotNone(self.model.get_root())
        self.assertEqual(len(self.model.get_root().get_items()), 0)

    def test_description(self):
        desc = "newDescription"
        self.assertIsNotNone(self.model.get_root())
        self.model.get_root().set_description(desc)
        self.assertEqual(self.model.get_root().get_description(), desc)

    def test_basic_types(self):
        self.assertIsNotNone(self.model.get_types_definition().get_type("bool"))
        self.assertIsNotNone(self.model.get_types_definition().get_type("double"))
        self.assertIsNotNone(self.model.get_types_definition().get_type("float"))
        self.assertIsNotNone(self.model.get_types_definition().get_type("int"))
        self.assertIsNotNone(self.model.get_types_definition().get_type("short"))
        self.assertIsNotNone(self.model.get_types_definition().get_type("long"))
        self.assertIsNotNone(self.model.get_types_definition().get_type("string"))


class ModelEnumTypesTest(unittest.TestCase):
    def setUp(self):
        self.version = "1.2.3"
        self.model = OpenSandConf.MetaModel(self.version)
        self.types_count = len(self.model.get_types_definition().get_types())

    def test_add_enum_type_without_values(self):
        self.assertIsNone(self.model.get_types_definition().add_enum_type("e", "enum", []))
        self.assertEqual(len(self.model.get_types_definition().get_types()), self.types_count)
        self.assertIsNone(self.model.get_types_definition().get_type("e"))

        self.assertIsNone(self.model.get_types_definition().add_enum_type("e", "enum", [], "description"))
        self.assertEqual(len(self.model.get_types_definition().get_types()), self.types_count)
        self.assertIsNone(self.model.get_types_definition().get_type("e"))

    def test_add_enum_type(self):
        vals = ["val1", "val2", "val3"]
        self.assertIsNotNone(self.model.get_types_definition().add_enum_type("e", "enum", vals))
        self.assertEqual(len(self.model.get_types_definition().get_types()), self.types_count + 1)
        self.assertIsNotNone(self.model.get_types_definition().get_type("e"))

        e = self.model.get_types_definition().get_type("e")
        self.assertIsNotNone(e)
        self.assertEqual(len(e.get_values()), len(vals))

        desc = "my custom enum"
        e.set_description(desc)
        self.assertEqual(self.model.get_types_definition().get_type("e").get_description(), desc)

        for v in vals:
            self.assertIn(v, e.get_values())

    def test_add_several_enum_type(self):
        vals = ["val1", "val2", "val3"]

        self.assertIsNotNone(self.model.get_types_definition().add_enum_type("e", "enum", ["test"]))
        self.assertEqual(len(self.model.get_types_definition().get_types()), self.types_count + 1)
        self.assertEqual(len(self.model.get_types_definition().get_enum_types()), 1)
        self.assertIsNotNone(self.model.get_types_definition().get_type("e"))

        self.assertIsNotNone(self.model.get_types_definition().add_enum_type("f", "enum", vals))
        self.assertEqual(len(self.model.get_types_definition().get_types()), self.types_count + 2)
        self.assertEqual(len(self.model.get_types_definition().get_enum_types()), 2)
        self.assertIsNotNone(self.model.get_types_definition().get_type("f"))

        f = self.model.get_types_definition().get_type("f")
        self.assertIsNotNone(f)
        self.assertEqual(len(f.get_values()), len(vals))

        desc = "my custom enum"
        f.set_description(desc)
        self.assertEqual(self.model.get_types_definition().get_type("f").get_description(), desc)

        for v in vals:
            self.assertIn(v, f.get_values())

    def test_add_enum_type_with_duplicated_values(self):
        vals = ["val1", "val1"]
        self.assertIsNotNone(self.model.get_types_definition().add_enum_type("e", "enum", vals))
        self.assertEqual(len(self.model.get_types_definition().get_types()), self.types_count + 1)
        self.assertEqual(len(self.model.get_types_definition().get_enum_types()), 1)
        self.assertIsNotNone(self.model.get_types_definition().get_type("e"))

        e = self.model.get_types_definition().get_type("e")
        self.assertIsNotNone(e)
        self.assertNotEqual(len(e.get_values()), len(vals))
        self.assertEqual(len(e.get_values()), 1)

        self.assertIn(vals[0], e.get_values())

    def test_add_already_existing_enum_type(self):
        vals = ["val1", "val1"]
        self.assertIsNotNone(self.model.get_types_definition().add_enum_type("e", "enum", vals))
        self.assertEqual(len(self.model.get_types_definition().get_types()), self.types_count + 1)
        self.assertEqual(len(self.model.get_types_definition().get_enum_types()), 1)
        self.assertIsNotNone(self.model.get_types_definition().get_type("e"))
        self.assertEqual(self.model.get_types_definition().get_type("e").get_name(), "enum")

        self.assertIsNone(self.model.get_types_definition().add_enum_type("e", "enum2", ["test"]))
        self.assertEqual(len(self.model.get_types_definition().get_types()), self.types_count + 1)
        self.assertEqual(len(self.model.get_types_definition().get_enum_types()), 1)
        self.assertEqual(self.model.get_types_definition().get_type("e").get_name(), "enum")


class ModelComponentTest(unittest.TestCase):
    def setUp(self):
        self.version = "1.2.3"
        self.model = OpenSandConf.MetaModel(self.version)

    def test_add_components(self):
        cpt1 = self.model.get_root().add_component("id1", "Component 1")
        self.assertIsNotNone(cpt1)
        self.assertEqual(cpt1.get_id(), "id1")
        self.assertEqual(cpt1.get_name(), "Component 1")
        self.assertEqual(cpt1.get_description(), "")
        self.assertEqual(len(cpt1.get_items()), 0)
        self.assertEqual(len(self.model.get_root().get_items()), 1)

        cpt2 = self.model.get_root().add_component("id2", "Component 2", "Description 2")
        self.assertIsNotNone(cpt2)
        self.assertEqual(cpt2.get_id(), "id2")
        self.assertEqual(cpt2.get_name(), "Component 2")
        self.assertEqual(cpt2.get_description(), "Description 2")
        self.assertEqual(len(cpt2.get_items()), 0)
        self.assertEqual(len(self.model.get_root().get_items()), 2)

        self.assertIsNotNone(cpt1.add_parameter("id1", "Parameter 1", self.model.get_types_definition().get_type("int")))
        self.assertEqual(len(cpt1.get_items()), 1)
        self.assertEqual(len(cpt2.get_items()), 0)

    def test_add_composite_components(self):
        cpt1 = self.model.get_root().add_component("id1", "Component 1")
        self.assertIsNotNone(cpt1)
        self.assertEqual(len(cpt1.get_items()), 0)
        cpt2 = self.model.get_root().add_component("id2", "Component 2", "Description 2")
        self.assertIsNotNone(cpt2)
        self.assertEqual(len(cpt2.get_items()), 0)

        cpt3 = cpt1.add_component("id1", "Component 1")
        self.assertIsNotNone(cpt3)
        self.assertEqual(len(cpt1.get_items()), 1)
        self.assertEqual(len(cpt2.get_items()), 0)

        self.assertIsNone(cpt1.add_component("id1", "Component 2"))
        self.assertIsNotNone(cpt2.add_component("id1", "Component 2"))

        self.assertIsNotNone(cpt3.add_component("id3", "Component 3", "Description 3"))

        self.assertEqual(len(cpt1.get_items()), 1)
        self.assertEqual(len(cpt2.get_items()), 1)

    def test_create_data_components(self):
        cpt1 = self.model.get_root().add_component("id1", "Component 1")
        self.assertIsNotNone(cpt1)
        cpt2 = self.model.get_root().add_component("id2", "Component 2")
        self.assertIsNotNone(cpt2)
        cpt13 = cpt1.add_component("id3", "Component 3")
        self.assertIsNotNone(cpt13)
        cpt24 = cpt2.add_component("id4", "Component 4")
        self.assertIsNotNone(cpt24)
        cpt135 = cpt13.add_component("id5", "Component 5")
        self.assertIsNotNone(cpt135)

        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)
        self.assertEqual(datamodel.get_version(), self.version)
        self.assertIsNotNone(datamodel.get_root())
        self.assertFalse(len(datamodel.get_root().get_items()) == 0)

        data1 = datamodel.get_root().get_component("id1")
        self.assertIsNotNone(data1)
        data2 = datamodel.get_root().get_component("id2")
        self.assertIsNotNone(data2)
        data13 = data1.get_component("id3")
        self.assertIsNotNone(data13)
        data24 = data2.get_component("id4")
        self.assertIsNotNone(data24)
        data135 = data13.get_component("id5")
        self.assertIsNotNone(data135)

        self.assertTrue(datamodel.validate())


class ModelParameterTest(unittest.TestCase):
    def setUp(self):
        self.version = "1.2.3"
        self.model = OpenSandConf.MetaModel(self.version)

    def test_add_bool_parameter(self):
        id = "id1"
        val1 = True
        val2 = False

        # Check the meta parameter
        param = self.model.get_root().add_parameter(id, "Parameter 1", self.model.get_types_definition().get_type("bool"))
        self.assertIsNotNone(param)
        self.assertIsInstance(param.get_type(), OpenSandConf.MetaBoolType)
        self.assertEqual(param.get_unit(), "")

        # Check the data parameter
        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)
        self.assertEqual(datamodel.get_version(), self.model.get_version())
        self.assertIsNotNone(datamodel.get_root())
        self.assertEqual(len(datamodel.get_root().get_items()), 1)
        dataparam = datamodel.get_root().get_parameter(id)
        self.assertIsNotNone(dataparam)
        data = dataparam.get_data()
        self.assertIsNotNone(data)
        self.assertIsInstance(data, OpenSandConf.DataBool)
        self.assertFalse(data.is_set())

        # Check the data value
        self.assertTrue(data.set(val1))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val1)
        self.assertTrue(data.set(val2))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val2)
        data.reset()
        self.assertFalse(data.is_set())

        # Check the data string
        data.reset()
        self.assertFalse(data.is_set())
        val1 = False
        str1 = "false"
        val2 = True
        str2 = "true"
        invalid = "42"
        self.assertTrue(data.set(val1))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val1)
        self.assertEqual(str(data), str1)

        self.assertTrue(data.from_string(str2))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val2)
        self.assertEqual(str(data), str2)

        self.assertFalse(data.from_string(invalid))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val2)
        self.assertEqual(str(data), str2)

        data.reset()
        self.assertFalse(data.is_set())
        self.assertFalse(data.from_string(invalid))
        self.assertFalse(data.is_set())

        data.reset()
        self.assertFalse(data.is_set())
        self.assertTrue(data.from_string(str1))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val1)
        self.assertEqual(str(data), str1)

    def test_add_int_parameter(self):
        id = "id1"
        val1 = 23
        val2 = 42

        # Check the meta parameter
        param = self.model.get_root().add_parameter(id, "Parameter 1", self.model.get_types_definition().get_type("int"))
        self.assertIsNotNone(param)
        self.assertIsInstance(param.get_type(), OpenSandConf.MetaIntType)
        self.assertEqual(param.get_unit(), "")

        # Check the data parameter
        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)
        self.assertEqual(datamodel.get_version(), self.model.get_version())
        self.assertIsNotNone(datamodel.get_root())
        self.assertEqual(len(datamodel.get_root().get_items()), 1)
        dataparam = datamodel.get_root().get_parameter(id)
        self.assertIsNotNone(dataparam)
        data = dataparam.get_data()
        self.assertIsNotNone(data)
        self.assertIsInstance(data, OpenSandConf.DataInt)
        self.assertFalse(data.is_set())

        # Check the data value
        self.assertTrue(data.set(val1))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val1)
        self.assertTrue(data.set(val2))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val2)
        data.reset()
        self.assertFalse(data.is_set())

        # Check the data string
        data.reset()
        self.assertFalse(data.is_set())
        val1 = 42
        str1 = "42"
        val2 = 23
        str2 = "23"
        val3 = 86
        str3 = "86.2"
        str3b = "86"
        invalid = "azerty"
        #invalid = "42.23azerty" # FIXME? fromString succeeds but would not
        self.assertTrue(data.set(val1))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val1)
        self.assertEqual(str(data), str1)

        self.assertTrue(data.from_string(str2))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val2)
        self.assertEqual(str(data), str2)

        self.assertFalse(data.from_string(invalid))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val2)
        self.assertEqual(str(data), str2)

        self.assertTrue(data.from_string(str3))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val3)
        self.assertEqual(str(data), str3b)

        data.reset()
        self.assertFalse(data.is_set())
        self.assertFalse(data.from_string(invalid))
        self.assertFalse(data.is_set())

        data.reset()
        self.assertFalse(data.is_set())
        self.assertTrue(data.from_string(str1))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val1)
        self.assertEqual(str(data), str1)

    def test_add_double_parameter(self):
        id = "id1"
        val1 = 0.23
        val2 = 0.42

        # Check the meta parameter
        param = self.model.get_root().add_parameter(id, "Parameter 1", self.model.get_types_definition().get_type("double"))
        self.assertIsNotNone(param)
        self.assertIsInstance(param.get_type(), OpenSandConf.MetaDoubleType)
        self.assertEqual(param.get_unit(), "")

        # Check the data parameter
        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)
        self.assertEqual(datamodel.get_version(), self.model.get_version())
        self.assertIsNotNone(datamodel.get_root())
        self.assertEqual(len(datamodel.get_root().get_items()), 1)
        dataparam = datamodel.get_root().get_parameter(id)
        self.assertIsNotNone(dataparam)
        data = dataparam.get_data()
        self.assertIsNotNone(data)
        self.assertIsInstance(data, OpenSandConf.DataDouble)
        self.assertFalse(data.is_set())

        # Check the data value
        self.assertTrue(data.set(val1))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val1)
        self.assertTrue(data.set(val2))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val2)
        data.reset()
        self.assertFalse(data.is_set())

        # Check the data string
        data.reset()
        self.assertFalse(data.is_set())
        val1 = 42.42
        str1 = "42.420000"
        val2 = 23.23
        str2 = "23.230000"
        val3 = 1.12e3
        str3 = "1.12e3"
        str3b = "1120.000000"
        val4 = 86
        str4 = "86"
        str4b = "86.000000"
        invalid = "azerty"
        self.assertTrue(data.set(val1))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val1)
        self.assertEqual(str(data), str1)

        self.assertTrue(data.from_string(str2))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val2)
        self.assertEqual(str(data), str2)

        self.assertFalse(data.from_string(invalid))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val2)
        self.assertEqual(str(data), str2)

        self.assertTrue(data.from_string(str3))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val3)
        self.assertEqual(str(data), str3b)

        self.assertTrue(data.from_string(str4))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val4)
        self.assertEqual(str(data), str4b)

        data.reset()
        self.assertFalse(data.is_set())
        self.assertFalse(data.from_string(invalid))
        self.assertFalse(data.is_set())

        data.reset()
        self.assertFalse(data.is_set())
        self.assertTrue(data.from_string(str1))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val1)
        self.assertEqual(str(data), str1)

    def test_add_string_parameter(self):
        id = "id1"
        val1 = "value 1"
        val2 = "value 2"

        # Check the meta parameter
        param = self.model.get_root().add_parameter(id, "Parameter 1", self.model.get_types_definition().get_type("string"))
        self.assertIsNotNone(param)
        self.assertIsInstance(param.get_type(), OpenSandConf.MetaStringType)
        self.assertEqual(param.get_unit(), "")

        # Check the data parameter
        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)
        self.assertEqual(datamodel.get_version(), self.model.get_version())
        self.assertIsNotNone(datamodel.get_root())
        self.assertEqual(len(datamodel.get_root().get_items()), 1)
        dataparam = datamodel.get_root().get_parameter(id)
        self.assertIsNotNone(dataparam)
        data = dataparam.get_data()
        self.assertIsNotNone(data)
        self.assertIsInstance(data, OpenSandConf.DataString)
        self.assertFalse(data.is_set())

        # Check the data value
        self.assertTrue(data.set(val1))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val1)
        self.assertTrue(data.set(val2))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val2)
        data.reset()
        self.assertFalse(data.is_set())

        # Check the data string
        data.reset()
        self.assertFalse(data.is_set())
        val1 = "42.42azerty"
        str1 = val1
        val2 = "23.23!?*"
        str2 = val2
        self.assertTrue(data.set(val1))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val1)
        self.assertEqual(str(data), str1)

        self.assertTrue(data.from_string(str2))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val2)
        self.assertEqual(str(data), str2)

        data.reset()
        self.assertFalse(data.is_set())
        self.assertTrue(data.from_string(str1))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val1)
        self.assertEqual(str(data), str1)

    def test_add_enum_parameter(self):
        id = "id1"
        val1 = "value 1"
        val2 = "value 2"
        invalid = "value3"

        # Check the meta parameter
        enum_type = self.model.get_types_definition().add_enum_type("enum1", "Parameter enum 1", [ val1, val2 ])
        self.assertIsNotNone(enum_type)
        param = self.model.get_root().add_parameter(id, "Parameter 1", self.model.get_types_definition().get_type("enum1"))
        self.assertIsNotNone(param)
        self.assertIsInstance(param.get_type(), OpenSandConf.MetaEnumType)
        self.assertEqual(param.get_unit(), "")

        # Check the data parameter
        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)
        self.assertEqual(datamodel.get_version(), self.model.get_version())
        self.assertIsNotNone(datamodel.get_root())
        self.assertEqual(len(datamodel.get_root().get_items()), 1)
        dataparam = datamodel.get_root().get_parameter(id)
        self.assertIsNotNone(dataparam)
        data = dataparam.get_data()
        self.assertIsNotNone(data)
        self.assertIsInstance(data, OpenSandConf.DataString)
        self.assertFalse(data.is_set())

        # Check the data value
        self.assertTrue(data.set(val1))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val1)
        self.assertTrue(data.set(val2))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), val2)
        data.reset()
        self.assertFalse(data.is_set())
        self.assertFalse(data.set(invalid))
        self.assertFalse(data.is_set())

    def test_add_several_parameters(self):
        # Meta model
        param1 = self.model.get_root().add_parameter("id1", "Parameter 1", self.model.get_types_definition().get_type("string"))
        self.assertIsNotNone(param1)
        self.assertIsNone(self.model.get_root().add_parameter("id1", "Parameter 2", self.model.get_types_definition().get_type("string")))
        self.assertIsNone(self.model.get_root().add_parameter("id1", "Parameter 1", self.model.get_types_definition().get_type("int")))
        self.assertEqual(param1.get_unit(), "")
        unit1 = "u"
        param1.set_unit(unit1)
        self.assertEqual(param1.get_unit(), unit1)

        param2 = self.model.get_root().add_parameter("id2", "Parameter 2", self.model.get_types_definition().get_type("string"))
        self.assertIsNotNone(param2)
        self.assertEqual(param2.get_unit(), "")
        unit2 = "u2"
        param2.set_unit(unit2)
        self.assertEqual(param2.get_unit(), unit2)
        self.assertEqual(param1.get_unit(), unit1)

        # Data model
        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)
        self.assertEqual(datamodel.get_version(), self.model.get_version())
        self.assertIsNotNone(datamodel.get_root())
        self.assertEqual(len(datamodel.get_root().get_items()), 2)
        dataparam1 = datamodel.get_root().get_parameter(param1.get_id())
        self.assertIsNotNone(dataparam1)
        data1 = dataparam1.get_data()
        self.assertIsNotNone(data1)
        self.assertIsInstance(data1, OpenSandConf.DataString)
        self.assertFalse(data1.is_set())
        dataparam2 = datamodel.get_root().get_parameter(param2.get_id())
        self.assertIsNotNone(dataparam2)
        data2 = dataparam2.get_data()
        self.assertIsNotNone(data2)
        self.assertIsInstance(data2, OpenSandConf.DataString)
        self.assertFalse(data2.is_set())

        self.assertTrue(data1.set("value"))
        self.assertTrue(data1.is_set())
        self.assertFalse(data2.is_set())

    def test_create_data_parameters(self):
        # Meta model
        param1 = self.model.get_root().add_parameter("id1", "Parameter 1", self.model.get_types_definition().get_type("string"))
        self.assertIsNotNone(param1)
        self.assertTrue(self.model.get_root().add_parameter("id2", "Parameter 2", self.model.get_types_definition().get_type("string")))
        self.assertIsNone(self.model.get_root().add_component("id1", "Component 1"))
        cpt1 = self.model.get_root().add_component("cpt1", "Component 1")
        self.assertIsNotNone(cpt1)
        param2 = cpt1.add_parameter("id1", "Parameter 1", self.model.get_types_definition().get_type("string"))
        self.assertIsNotNone(param2)
        self.assertIsNotNone(cpt1.add_parameter("id2", "Parameter 2", self.model.get_types_definition().get_type("string")))

        # Data model
        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)
        self.assertEqual(datamodel.get_version(), self.model.get_version())
        self.assertIsNotNone(datamodel.get_root())
        self.assertEqual(len(datamodel.get_root().get_items()), 3)
        dataparam1 = datamodel.get_root().get_parameter(param1.get_id())
        self.assertIsNotNone(dataparam1)
        data1 = dataparam1.get_data()
        self.assertIsNotNone(data1)
        self.assertIsInstance(data1, OpenSandConf.DataString)
        self.assertFalse(data1.is_set())
        dataparam2 = datamodel.get_root().get_component(cpt1.get_id()).get_parameter(param2.get_id())
        self.assertIsNotNone(dataparam2)
        data2 = dataparam2.get_data()
        self.assertIsNotNone(data2)
        self.assertIsInstance(data2, OpenSandConf.DataString)
        self.assertFalse(data2.is_set())

        self.assertTrue(data1.set("value"))
        self.assertTrue(data1.is_set())
        self.assertFalse(data2.is_set())

    def test_get_parameters_from_path(self):
        self.assertIsNone(self.model.get_item_by_path(""))
        root = self.model.get_item_by_path("/")
        self.assertEqual(root.get_path(), self.model.get_root().get_path())
        cpt1 = self.model.get_root().add_component("cpt1", "Component 1")
        self.assertIsNotNone(cpt1)
        self.assertEqual(cpt1.get_path(), self.model.get_root().get_item("cpt1").get_path())
        self.assertEqual(cpt1.get_path(), self.model.get_item_by_path("/cpt1").get_path())
        cpt2 = cpt1.add_component("cpt2", "Component 2")
        self.assertIsNotNone(cpt2)
        self.assertEqual(cpt2.get_path(), cpt1.get_item("cpt2").get_path())
        self.assertEqual(cpt2.get_path(), self.model.get_item_by_path("/cpt1/cpt2").get_path())
        param = cpt2.add_parameter("p1", "Parameter 1", self.model.get_types_definition().get_type("string"))
        self.assertIsNotNone(param)
        self.assertEqual(param.get_path(), cpt2.get_item("p1").get_path())
        param2 = self.model.get_item_by_path("/cpt1/cpt2/p1")
        self.assertEqual(param.get_path(), param2.get_path())
        desc = "This is a description"
        param2.set_description(desc)
        self.assertEqual(param.get_description(), desc)

        self.assertNotEqual(root.get_path(), cpt1.get_path())
        self.assertNotEqual(cpt1.get_path(), cpt2.get_path())
        self.assertNotEqual(cpt2.get_path(), param.get_path())

class ModelListTest(unittest.TestCase):
    def setUp(self):
        self.version = "1.2.3"
        self.model = OpenSandConf.MetaModel(self.version)

    def test_add_lists(self):
        lst1 = self.model.get_root().add_list("id1", "List 1", "Pattern 1")
        self.assertIsNotNone(lst1)
        self.assertIsNotNone(lst1.get_pattern())
        ptn1 = lst1.get_pattern()
        self.assertIsNotNone(ptn1)
        self.assertEqual(len(ptn1.get_items()), 0)

        lst2 = self.model.get_root().add_list("id2", "List 1", "Pattern 1")
        self.assertIsNotNone(lst2)
        self.assertIsNotNone(lst2.get_pattern())
        self.assertIsNone(self.model.get_root().add_list("id1", "List 2", "Pattern 2"))
        ptn2 = lst2.get_pattern()
        self.assertIsNotNone(ptn2)
        self.assertEqual(len(ptn2.get_items()), 0)

        self.assertIsNotNone(ptn1.add_parameter("id1", "Parameter 1", self.model.get_types_definition().get_type("int")))
        self.assertEqual(len(ptn1.get_items()), 1)
        self.assertEqual(len(ptn2.get_items()), 0)

    def test_add_lists_items(self):
        # Build list
        lst = self.model.get_root().add_list("id1", "List 1", "Pattern 1")
        self.assertIsNotNone(lst)
        self.assertIsNotNone(lst.get_pattern())

        # Build pattern
        desc = "This is a description"
        ptn = lst.get_pattern()
        self.assertIsNotNone(ptn)
        self.assertEqual(ptn.get_description(), "")
        ptn.set_description(desc)
        self.assertEqual(ptn.get_description(), desc)
        self.assertEqual(len(ptn.get_items()), 0)
        self.assertIsNotNone(ptn.add_parameter("p1", "Parameter 1", self.model.get_types_definition().get_type("string")))
        self.assertIsNotNone(ptn.add_list("l1", "List 1", "Item"))
        self.assertIsNotNone(ptn.get_list("l1").get_pattern().add_parameter("p1", "Parameter 1", self.model.get_types_definition().get_type("string")))

        # Add and check items
        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)
        self.assertEqual(datamodel.get_version(), self.model.get_version())
        self.assertIsNotNone(datamodel.get_root())
        self.assertEqual(len(datamodel.get_root().get_items()), 1)
        datalst = datamodel.get_root().get_list(lst.get_id())
        self.assertIsNotNone(datalst)
        self.assertEqual(len(datalst.get_items()), 0)
        item1 = datalst.add_item()
        self.assertIsNotNone(item1)
        self.assertEqual(len(datalst.get_items()), 1)
        item2 = datalst.add_item()
        self.assertIsNotNone(item2)
        self.assertEqual(len(datalst.get_items()), 2)

        # Check items parameters
        i1p1 = item1.get_parameter("p1")
        self.assertIsNotNone(i1p1)
        i1d1 = i1p1.get_data()
        self.assertIsNotNone(i1d1)
        i2p1 = item2.get_parameter("p1")
        self.assertIsNotNone(i2p1)
        i2d1 = i2p1.get_data()
        self.assertIsNotNone(i2d1)
        self.assertFalse(i1d1.is_set())
        self.assertFalse(i2d1.is_set())
        i1d1.set("value")
        self.assertTrue(i1d1.is_set())
        self.assertFalse(i2d1.is_set())

        # Check items lists
        i1l1 = item1.get_list("l1")
        self.assertIsNotNone(i1l1)
        self.assertEqual(len(i1l1.get_items()), 0)
        i2l1 = item2.get_list("l1")
        self.assertIsNotNone(i2l1)
        self.assertEqual(len(i2l1.get_items()), 0)
        i1l1i1 = i1l1.add_item()
        self.assertEqual(len(i1l1.get_items()), 1)
        self.assertEqual(len(i2l1.get_items()), 0)

    def test_create_data_lists(self):
        cpt = self.model.get_root().add_component("cpt", "Component")
        l = self.model.get_root().add_list("id1", "List 1", "Pattern 1")
        self.assertIsNotNone(l)
        l1 = cpt.add_list("id1", "List 1", "Pattern 1")
        self.assertIsNotNone(l1)

        # Build pattern
        ptn = l1.get_pattern()
        self.assertIsNotNone(ptn)
        self.assertEqual(len(ptn.get_items()), 0)
        self.assertIsNotNone(ptn.add_parameter("p1", "Parameter 1", self.model.get_types_definition().get_type("string")))
        self.assertIsNotNone(ptn.add_list("l1", "List 1", "Item"))
        self.assertIsNotNone(ptn.get_list("l1").get_pattern().add_parameter("p1", "Parameter 1", self.model.get_types_definition().get_type("string")))
        p = l.get_pattern()
        self.assertIsNotNone(p)
        self.assertEqual(len(p.get_items()), 0)
        self.assertEqual(len(ptn.get_items()), 2)

        # Add and check items
        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)
        self.assertEqual(datamodel.get_version(), self.model.get_version())
        self.assertIsNotNone(datamodel.get_root())
        self.assertEqual(len(datamodel.get_root().get_items()), 2)
        datacpt = datamodel.get_root().get_component(cpt.get_id())
        self.assertIsNotNone(datacpt)
        datal = datamodel.get_root().get_list(l.get_id())
        self.assertIsNotNone(datal)
        datal1 = datacpt.get_list(l1.get_id())
        self.assertIsNotNone(datal1)
        self.assertEqual(len(datal1.get_items()), 0)

        item1 = datal1.add_item()
        self.assertIsNotNone(item1)
        item2 = datal1.add_item()
        self.assertIsNotNone(item2)
        self.assertEqual(len(datal.get_items()), 0)
        self.assertEqual(len(datal1.get_items()), 2)

        # Check items parameters
        i1p1 = item1.get_parameter("p1")
        self.assertIsNotNone(i1p1)
        i1d1 = i1p1.get_data()
        self.assertIsNotNone(i1d1)
        i2p1 = item2.get_parameter("p1")
        self.assertIsNotNone(i2p1)
        i2d1 = i2p1.get_data()
        self.assertIsNotNone(i2d1)
        self.assertFalse(i1d1.is_set())
        self.assertFalse(i2d1.is_set())
        self.assertTrue(i1d1.set("value"))
        self.assertTrue(i1d1.is_set())
        self.assertFalse(i2d1.is_set())

        # Check items lists
        i1l1 = item1.get_list("l1")
        self.assertIsNotNone(i1l1)
        self.assertEqual(len(i1l1.get_items()), 0)
        i2l1 = item2.get_list("l1")
        self.assertIsNotNone(i2l1)
        self.assertEqual(len(i2l1.get_items()), 0)
        i1l1i1 = i1l1.add_item()
        self.assertIsNotNone(i1l1i1)
        self.assertEqual(len(i1l1.get_items()), 1)
        self.assertEqual(len(i2l1.get_items()), 0)

    def test_get_lists_from_path(self):
        self.assertIsNone(self.model.get_item_by_path(""))
        root = self.model.get_item_by_path("/")
        self.assertEqual(root.get_path(), self.model.get_root().get_path())
        cpt1 = self.model.get_root().add_component("cpt1", "Component 1")
        self.assertIsNotNone(cpt1)
        self.assertEqual(cpt1.get_path(), self.model.get_root().get_item("cpt1").get_path())
        self.assertEqual(cpt1.get_path(), self.model.get_item_by_path("/cpt1").get_path())
        cpt2 = cpt1.add_component("cpt2", "Component 2")
        self.assertIsNotNone(cpt2)
        self.assertEqual(cpt2.get_path(), cpt1.get_item("cpt2").get_path())
        self.assertEqual(cpt2.get_path(), self.model.get_item_by_path("/cpt1/cpt2").get_path())
        lst = cpt2.add_list("l1", "List 1", "Item of list 1")
        self.assertIsNotNone(lst)
        self.assertEqual(lst.get_path(), cpt2.get_item("l1").get_path())
        self.assertEqual(lst.get_path(), self.model.get_item_by_path("/cpt1/cpt2/l1").get_path())
        pattern = lst.get_pattern()
        self.assertIsNotNone(pattern)
        self.assertEqual(pattern.get_path(), self.model.get_item_by_path("/cpt1/cpt2/l1/*").get_path())
        param = pattern.add_parameter("p1", "Parameter 1", self.model.get_types_definition().get_type("string"))
        self.assertIsNotNone(param)
        self.assertEqual(param.get_path(), pattern.get_item("p1").get_path())
        self.assertEqual(param.get_path(), self.model.get_item_by_path("/cpt1/cpt2/l1/*/p1").get_path())

        self.assertNotEqual(root.get_path(), cpt1.get_path())
        self.assertNotEqual(cpt1.get_path(), cpt2.get_path())
        self.assertNotEqual(cpt2.get_path(), lst.get_path())
        self.assertNotEqual(cpt2.get_path(), pattern.get_path())
        self.assertNotEqual(lst.get_path(), pattern.get_path())
        self.assertNotEqual(pattern.get_path(), param.get_path())

        # Check data
        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)
        self.assertEqual(datamodel.get_version(), self.model.get_version())
        self.assertIsNotNone(datamodel.get_root())
        self.assertEqual(len(datamodel.get_root().get_items()), 1)
        self.assertIsNone(datamodel.get_item_by_path(""))
        self.assertEqual(datamodel.get_root().get_path(), datamodel.get_item_by_path("/").get_path())
        datacpt1 = datamodel.get_root().get_component(cpt1.get_id())
        self.assertEqual(datacpt1.get_path(), datamodel.get_item_by_path("/cpt1").get_path())
        datacpt2 = datacpt1.get_component(cpt2.get_id())
        self.assertEqual(datacpt2.get_path(), datamodel.get_item_by_path("/cpt1/cpt2").get_path())
        datalst = datacpt2.get_list(lst.get_id())
        self.assertEqual(datalst.get_path(), datamodel.get_item_by_path("/cpt1/cpt2/l1").get_path())
        self.assertIsNone(datamodel.get_item_by_path("/cpt1/cpt2/l1/*"))
        self.assertEqual(len(datalst.get_items()), 0)

        dataitem1 = datalst.add_item()
        self.assertIsNotNone(dataitem1)
        self.assertEqual(dataitem1.get_path(), datamodel.get_item_by_path("/cpt1/cpt2/l1/0").get_path())
        dataparam = dataitem1.get_parameter("p1")
        self.assertIsNotNone(dataparam)
        self.assertEqual(dataparam.get_path(), datamodel.get_item_by_path("/cpt1/cpt2/l1/0/p1").get_path())
        data = dataparam.get_data()
        self.assertIsNotNone(data)
        self.assertFalse(data.is_set())
        self.assertTrue(data.set("value"))
        self.assertTrue(data.is_set())
        self.assertEqual(data.get(), "value")

        dataitem2 = datalst.add_item()
        self.assertIsNotNone(dataitem2)
        self.assertEqual(dataitem1.get_path(), datamodel.get_item_by_path("/cpt1/cpt2/l1/0").get_path())
        self.assertEqual(dataitem2.get_path(), datamodel.get_item_by_path("/cpt1/cpt2/l1/1").get_path())
        dataparam2 = dataitem2.get_parameter("p1")
        self.assertIsNotNone(dataparam2)
        self.assertEqual(dataparam2.get_path(), datamodel.get_item_by_path("/cpt1/cpt2/l1/1/p1").get_path())
        data2 = dataparam2.get_data()
        self.assertIsNotNone(data2)
        self.assertFalse(data2.is_set())

class ModelDataTests(unittest.TestCase):
    def setUp(self):
        self.version = "1.2.3"
        self.model = OpenSandConf.MetaModel(self.version)

    def test_data_creation(self):
        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)
        self.assertEqual(datamodel.get_version(), self.version)
        self.assertIsNotNone(datamodel.get_root())
        self.assertEqual(len(datamodel.get_root().get_items()), 0)
        self.assertTrue(datamodel.validate())

    def test_multiple_data_creation(self):
        # Create a meta model
        self.assertIsNotNone(self.model.get_root().add_parameter(
            "p", "Parameter", self.model.get_types_definition().get_type("int")))
        self.assertIsNotNone(self.model.get_root().add_list(
            "l", "List", "Pattern", "", ""))
        self.assertIsNotNone(self.model.get_root().get_list("l").get_pattern().add_parameter(
            "p", "Parameter", self.model.get_types_definition().get_type("int")))

        # Create a data model
        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)
        self.assertEqual(datamodel.get_version(), self.version)
        self.assertIsNotNone(datamodel.get_root())
        self.assertEqual(len(datamodel.get_root().get_items()), 2)
        self.assertIsNotNone(datamodel.get_root().get_parameter("p"))
        self.assertIsNotNone(datamodel.get_root().get_list("l"))
        self.assertEqual(len(datamodel.get_root().get_list("l").get_items()), 0)
        self.assertFalse(datamodel.validate())
        item1 = datamodel.get_root().get_list("l").add_item()
        self.assertIsNotNone(item1)
        self.assertEqual(len(item1.get_items()), 1)
        self.assertIsNotNone(item1.get_parameter("p"))
        self.assertFalse(datamodel.validate())

        # Create a second data model
        datamodel2 = self.model.create_data()
        self.assertIsNotNone(datamodel2)
        self.assertEqual(datamodel2.get_version(), self.version)
        self.assertIsNotNone(datamodel2.get_root())
        self.assertEqual(len(datamodel2.get_root().get_items()), 2)
        self.assertIsNotNone(datamodel2.get_root().get_parameter("p"))
        self.assertIsNotNone(datamodel2.get_root().get_list("l"))
        self.assertEqual(len(datamodel2.get_root().get_list("l").get_items()), 0)
        self.assertFalse(datamodel2.validate())
        item2 = datamodel2.get_root().get_list("l").add_item()
        self.assertIsNotNone(item2)
        self.assertEqual(len(item2.get_items()), 1)
        self.assertIsNotNone(item2.get_parameter("p"))
        item3 = datamodel2.get_root().get_list("l").add_item()
        self.assertIsNotNone(item3)
        self.assertEqual(len(item3.get_items()), 1)
        self.assertIsNotNone(item3.get_parameter("p"))
        self.assertFalse(datamodel2.validate())

        # Compare data models
        self.assertEqual(len(datamodel.get_root().get_list("l").get_items()), 1)
        self.assertEqual(len(datamodel2.get_root().get_list("l").get_items()), 2)

        # Fill data of the first data model
        data = datamodel.get_root().get_parameter("p").get_data()
        self.assertIsNotNone(data)
        data.set(42)
        data1 = item1.get_parameter("p").get_data()
        self.assertIsNotNone(data1)
        data1.set(23)
        self.assertTrue(datamodel.validate())
        self.assertFalse(datamodel2.validate())
        self.assertIsNotNone(datamodel.get_root().get_list("l").add_item())
        self.assertFalse(datamodel.validate())

    def test_meta_modification_after_data_creation(self):
        # Create a meta model
        self.assertIsNotNone(self.model.get_root().add_parameter(
            "p", "Parameter", self.model.get_types_definition().get_type("int")))
        self.assertIsNotNone(self.model.get_root().add_list(
            "l", "List", "Pattern", "", ""))
        self.assertIsNotNone(self.model.get_root().get_list("l").get_pattern().add_parameter(
            "p", "Parameter", self.model.get_types_definition().get_type("int")))

        # Create a data model
        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)
        self.assertEqual(datamodel.get_version(), self.version)
        self.assertIsNotNone(datamodel.get_root())
        self.assertEqual(len(datamodel.get_root().get_items()), 2)
        self.assertIsNotNone(datamodel.get_root().get_parameter("p"))
        self.assertIsNotNone(datamodel.get_root().get_list("l"))
        self.assertEqual(len(datamodel.get_root().get_list("l").get_items()), 0)
        item1 = datamodel.get_root().get_list("l").add_item()
        self.assertIsNotNone(item1)
        self.assertEqual(len(item1.get_items()), 1)
        self.assertIsNotNone(item1.get_parameter("p"))
        self.assertFalse(datamodel.validate())

        # Fill data of the data model
        data = datamodel.get_root().get_parameter("p").get_data()
        self.assertIsNotNone(data)
        data.set(42)
        data1 = item1.get_parameter("p").get_data()
        self.assertIsNotNone(data1)
        data1.set(23)
        self.assertTrue(datamodel.validate())

        # Modify the meta model
        self.assertIsNotNone(self.model.get_root().add_parameter("p2", "Parameter 2", self.model.get_types_definition().get_type("double")))
        self.assertEqual(len(self.model.get_root().get_items()), 3)
        self.assertEqual(len(datamodel.get_root().get_items()), 2)
        self.assertTrue(datamodel.validate())

        # Create a data model from the modified meta model
        datamodel2 = self.model.create_data()
        self.assertIsNotNone(datamodel2)
        self.assertEqual(datamodel2.get_version(), self.version)
        self.assertIsNotNone(datamodel2.get_root())
        self.assertEqual(len(datamodel2.get_root().get_items()), 3)
        self.assertIsNotNone(datamodel2.get_root().get_parameter("p"))
        self.assertIsNotNone(datamodel2.get_root().get_list("l"))
        self.assertEqual(len(datamodel2.get_root().get_list("l").get_items()), 0)
        self.assertFalse(datamodel2.validate())
        self.assertTrue(datamodel.validate())

        # Fill data of the second data model
        data2 = datamodel2.get_root().get_parameter("p").get_data()
        self.assertIsNotNone(data2)
        data2.set(42)
        data21 = datamodel2.get_root().get_parameter("p2").get_data()
        self.assertIsNotNone(data21)
        data21.set(23)
        self.assertTrue(datamodel.validate())
        self.assertTrue(datamodel2.validate())


class ModelReferenceTestsVariousTypes(unittest.TestCase):
    def setUp(self):
        self.version = "1.2.3"
        self.model = OpenSandConf.MetaModel(self.version)
        self.root = self.model.get_root()
        self.types = self.model.get_types_definition()

        self.assertIsNotNone(self.types.add_enum_type("enum1", "Enum 1", [ "val1", "val2"]))

        self.assertIsNotNone(self.root.add_parameter("b", "Boolean parameter", self.types.get_type("bool")))
        self.assertIsNotNone(self.root.add_parameter("d", "Double parameter", self.types.get_type("double")))
        self.assertIsNotNone(self.root.add_parameter("i", "Integer parameter", self.types.get_type("int")))
        self.assertIsNotNone(self.root.add_parameter("s", "String parameter", self.types.get_type("string")))
        self.assertIsNotNone(self.root.add_parameter("e", "Enum parameter", self.types.get_type("enum1")))
        self.cpt = self.root.add_component("c", "Component with reference")
        self.assertIsNotNone(self.cpt)
        self.assertIsNotNone(self.model.create_data())
        self.assertIsNone(self.cpt.get_reference_target())
        self.assertIsNone(self.cpt.get_reference_data())

    def _test(self, target, datacls, value, invalid, value2, invalid2):
        # Configure reference
        self.assertIsNotNone(target)
        self.assertTrue(self.model.set_reference(self.cpt, target))
        self.assertIsNotNone(self.cpt.get_reference_target())
        self.assertIsNotNone(self.cpt.get_reference_data())
        expected = self.cpt.get_reference_data()
        self.assertIsNotNone(expected)
        self.assertIsInstance(expected, datacls)

        # Check data creation failed if expected data is not set
        self.assertFalse(expected.is_set())
        self.assertIsNone(self.model.create_data())

        # Check with a value
        self.assertTrue(expected.set(value))
        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)
        self.assertIsNotNone(datamodel.get_item_by_path(target.get_path()))
        datatarget = datamodel.get_item_by_path(target.get_path())
        self.assertIsNotNone(datatarget)
        self.assertIsInstance(datatarget, OpenSandConf.DataParameter)
        datacpt = datamodel.get_item_by_path(self.cpt.get_path())
        self.assertIsNotNone(datacpt)
        self.assertIsInstance(datacpt, OpenSandConf.DataComponent)
        data = datatarget.get_data()
        self.assertIsNotNone(data)
        self.assertIsInstance(data, datacls)
        self.assertTrue(datatarget.check_reference())
        self.assertFalse(datacpt.check_reference())
        self.assertTrue(data.set(invalid))
        self.assertFalse(datacpt.check_reference())
        self.assertTrue(data.set(value))
        self.assertTrue(datacpt.check_reference())

        # Check with a second value
        self.assertTrue(expected.set(value2))
        datamodel2 = self.model.create_data()
        self.assertIsNotNone(datamodel2)
        self.assertIsNotNone(datamodel2.get_item_by_path(target.get_path()))
        datatarget2 = datamodel2.get_item_by_path(target.get_path())
        self.assertIsNotNone(datatarget2)
        self.assertIsInstance(datatarget2, OpenSandConf.DataParameter)
        datacpt2 = datamodel2.get_item_by_path(self.cpt.get_path())
        self.assertIsNotNone(datacpt2)
        self.assertIsInstance(datacpt2, OpenSandConf.DataComponent)
        data2 = datatarget2.get_data()
        self.assertIsNotNone(data2)
        self.assertIsInstance(data2, datacls)
        self.assertTrue(datatarget2.check_reference())
        self.assertFalse(datacpt2.check_reference())
        self.assertTrue(data2.set(invalid2))
        self.assertFalse(datacpt2.check_reference())
        self.assertTrue(data2.set(value2))
        self.assertTrue(datacpt2.check_reference())

        self.assertTrue(data.set(invalid))
        self.assertFalse(datacpt.check_reference())
        self.assertTrue(data.set(value))
        self.assertTrue(datacpt.check_reference())

    def test_reference_to_bool(self):
        self._test(self.root.get_parameter("b"), OpenSandConf.DataBool,
                True,  False,
                False, True)

    def test_reference_to_int(self):
        self._test(self.root.get_parameter("i"), OpenSandConf.DataInt,
                42, 23,
                23, 42)

    def test_reference_to_double(self):
        self._test(self.root.get_parameter("d"), OpenSandConf.DataDouble,
                0.42, 0.23,
                0.23, 0.42)

    def test_reference_to_string(self):
        self._test(self.root.get_parameter("s"), OpenSandConf.DataString,
                "value", "invalid",
                "value2", "value")

    def test_reference_to_enum(self):
        self._test(self.root.get_parameter("e"), OpenSandConf.DataString,
                "val1", "val2",
                "val2", "val1")


class ModelReferenceTestsAdvanced(unittest.TestCase):
    def setUp(self):
        self.version = "1.2.3"
        self.model = OpenSandConf.MetaModel(self.version)
        self.root = self.model.get_root()
        self.types = self.model.get_types_definition()

        # Add enum type
        self.assertIsNotNone(self.types.add_enum_type("enum1", "Enum 1", [ "val1", "val2"]))
        enum_type = self.types.get_type("enum1")
        string_type = self.types.get_type("string")

        # Create elements at level 1
        self.assertIsNotNone(self.root.add_parameter("e", "Enum parameter (level 1)", enum_type))
        self.assertIsNotNone(self.root.add_parameter("s", "String parameter (level 1)", string_type))
        self.cpt = self.root.add_component("c", "Component (level 1)")
        self.assertIsNotNone(self.cpt)

        # Create elements at level 2
        self.assertIsNotNone(self.cpt.add_parameter("e2", "Enum parameter (level 2)", enum_type))
        self.assertIsNotNone(self.cpt.add_parameter("s2", "String parameter (level 2)", string_type))
        self.cpt2 = self.cpt.add_component("c2", "Component (level 2)")
        self.assertIsNotNone(self.cpt2)

        # Create elements at level 3
        self.assertIsNotNone(self.cpt2.add_parameter("e3", "Enum parameter (level 3)", enum_type))
        self.assertIsNotNone(self.cpt2.add_parameter("s3", "String parameter (level 3)", string_type))
        self.lst3 = self.cpt2.add_list("l3", "List (level 3)", "Item")
        self.assertIsNotNone(self.lst3)
        self.ptn3 = self.lst3.get_pattern()
        self.assertIsNotNone(self.ptn3)
        self.lst3b = self.cpt2.add_list("l3b", "List 2 (level 3)", "Item")
        self.assertIsNotNone(self.lst3b)
        self.ptn3b = self.lst3b.get_pattern()
        self.assertIsNotNone(self.ptn3b)

        # Create elements at level 4
        self.assertIsNotNone(self.ptn3.add_parameter("e4", "Enum parameter (level 4)", enum_type))
        self.assertIsNotNone(self.ptn3.add_parameter("s4", "String parameter (level 4)", string_type))
        self.assertIsNotNone(self.ptn3b.add_parameter("e4", "Enum parameter (level 4)", enum_type))
        self.assertIsNotNone(self.ptn3b.add_parameter("s4", "String parameter (level 4)", string_type))
        self.lst4 = self.ptn3.add_list("l4", "List (level 4)", "Item")
        self.assertIsNotNone(self.lst4)
        self.ptn4 = self.lst4.get_pattern()
        self.assertIsNotNone(self.ptn4)

        # Create elements at level 5
        self.assertIsNotNone(self.ptn4.add_parameter("e5", "Enum parameter (level 5)", enum_type))
        self.assertIsNotNone(self.ptn4.add_parameter("s5", "String parameter (level 5)", string_type))
        self.cpt5 = self.ptn4.add_component("c5", "Component (level 5)")

        # Create elements at level 6
        self.assertIsNotNone(self.cpt5.add_parameter("e6", "Enum parameter (level 6)", enum_type))
        self.assertIsNotNone(self.cpt5.add_parameter("s6", "String parameter (level 6)", string_type))

        # Sanity checks
        self.assertIsNotNone(self.model.get_item_by_path("/c/c2/l3"))
        self.assertIsNotNone(self.model.get_item_by_path("/c/c2/l3/*"))
        self.assertIsNotNone(self.model.get_item_by_path("/c/c2/l3/*/l4"))
        self.assertIsNotNone(self.model.create_data())

    def _test(self, target, element, targetpath, elementpath, elementpath2, res2):
        # Configure reference
        self.assertIsNotNone(target)
        self.assertIsNotNone(element)
        self.assertTrue(self.model.set_reference(element, target))
        self.assertIsNotNone(element.get_reference_target())
        self.assertIsNotNone(element.get_reference_data())
        expected = element.get_reference_data()
        self.assertIsNotNone(expected)
        self.assertIsInstance(expected, OpenSandConf.DataString)

        # Check data creation failed if expected data is not set
        self.assertFalse(expected.is_set())
        self.assertIsNone(self.model.create_data())
        self.assertFalse(expected.set("invalid"))

        # Check with a value
        self.assertTrue(expected.set("val1"))
        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)

        datalst3 = datamodel.get_item_by_path("/c/c2/l3")
        self.assertIsNotNone(datalst3)
        self.assertIsInstance(datalst3, OpenSandConf.DataList)
        self.assertIsNotNone(datalst3.add_item())
        self.assertIsNotNone(datalst3.add_item())

        datalst3b = datamodel.get_item_by_path("/c/c2/l3b")
        self.assertIsNotNone(datalst3b)
        self.assertIsInstance(datalst3b, OpenSandConf.DataList)
        self.assertIsNotNone(datalst3b.add_item())

        datalst4_0 = datamodel.get_item_by_path("/c/c2/l3/0/l4")
        self.assertIsNotNone(datalst4_0)
        self.assertIsInstance(datalst4_0, OpenSandConf.DataList)
        self.assertIsNotNone(datalst4_0.add_item())
        self.assertIsNotNone(datalst4_0.add_item())

        datalst4_1 = datamodel.get_item_by_path("/c/c2/l3/1/l4")
        self.assertIsNotNone(datalst4_1)
        self.assertIsInstance(datalst4_1, OpenSandConf.DataList)
        self.assertIsNotNone(datalst4_1.add_item())
        self.assertIsNotNone(datalst4_1.add_item())

        datatarget = datamodel.get_item_by_path(targetpath)
        self.assertIsNotNone(datatarget)
        self.assertIsInstance(datatarget, OpenSandConf.DataParameter)
        dataelement = datamodel.get_item_by_path(elementpath)
        self.assertIsNotNone(dataelement)
        data = datatarget.get_data()
        self.assertIsNotNone(data)
        self.assertIsInstance(data, OpenSandConf.DataString)

        self.assertTrue(datatarget.check_reference())
        self.assertFalse(dataelement.check_reference())
        self.assertTrue(data.set("val2"))
        self.assertFalse(dataelement.check_reference())
        self.assertTrue(data.set("val1"))
        self.assertTrue(dataelement.check_reference())

        if elementpath2 != "":
            dataelement2 = datamodel.get_item_by_path(elementpath2)
            self.assertIsNotNone(dataelement2)
            self.assertEqual(dataelement2.check_reference(), res2)

    def _test2(self, target, element):
        # Configure reference
        self.assertIsNotNone(target)
        self.assertIsNotNone(element)
        self.assertFalse(self.model.set_reference(element, target))

    def test_upper_target(self):
        target = self.root.get_parameter("e")
        element = self.cpt2.get_parameter("s3")
        self._test(target, element,
                target.get_path(), element.get_path(),
                "", False)

    def test_upper_element(self):
        target = self.cpt2.get_parameter("e3")
        element = self.root.get_parameter("s")
        self._test(target, element,
                target.get_path(), element.get_path(),
                "", False)

    def test_element_in_list_pattern(self):
        target = self.root.get_parameter("e")
        element = self.ptn3.get_parameter("s4")
        self._test(target, element,
                target.get_path(), "/c/c2/l3/1/s4",
                "/c/c2/l3/0/s4", True)

    def test_target_in_list_pattern(self):
        target = self.ptn3.get_parameter("e4")
        element = self.root.get_parameter("s")
        self._test2(target, element)

    def test_element_and_target_in_list_pattern(self):
        target = self.ptn3.get_parameter("e4")
        element = self.ptn3.get_parameter("s4")
        self._test(target, element,
                "/c/c2/l3/1/e4", "/c/c2/l3/1/s4",
                "/c/c2/l3/0/s4", False)

    def test_element_and_target_in_list_pattern_in_list_item(self):
        target = self.ptn4.get_parameter("e5")
        element = self.ptn4.get_parameter("s5")
        self._test(target, element,
                "/c/c2/l3/1/l4/0/e5", "/c/c2/l3/1/l4/0/s5",
                "/c/c2/l3/1/l4/1/s5", False)

    def test_element_and_target_in_different_list_patterns(self):
        target = self.ptn3.get_parameter("e4")
        element = self.ptn3b.get_parameter("s4")
        self._test2(target, element)

    def test_target_in_list_pattern_and_element_in_pattern_list_of_target_list_item(self):
        target = self.ptn3.get_parameter("e4")
        element = self.ptn4.get_parameter("s5")
        self._test(target, element,
                "/c/c2/l3/1/e4", "/c/c2/l3/1/l4/0/s5",
                "/c/c2/l3/1/l4/1/s5", True)

    def test_element_in_list_pattern_and_target_in_pattern_list_of_element_list_item(self):
        target = self.ptn4.get_parameter("e5")
        element = self.ptn3.get_parameter("s4")
        self._test2(target, element)


class ModelValidityTests(unittest.TestCase):
    def setUp(self):
        self.version = "1.2.3"
        self.model = OpenSandConf.MetaModel(self.version)
        self.root = self.model.get_root()
        self.types = self.model.get_types_definition()

        # Add enum type
        self.assertIsNotNone(self.types.add_enum_type("enum1", "Enum 1", [ "val1", "val2"]))
        enum_type = self.types.get_type("enum1")
        string_type = self.types.get_type("string")

        # Create elements at level 1
        self.assertIsNotNone(self.root.add_parameter("e", "Enum parameter (level 1)", enum_type))
        self.assertIsNotNone(self.root.add_parameter("s", "String parameter (level 1)", string_type))
        self.cpt = self.root.add_component("c", "Component (level 1)")
        self.assertIsNotNone(self.cpt)

        # Create elements at level 2
        self.assertIsNotNone(self.cpt.add_parameter("e2", "Enum parameter (level 2)", enum_type))
        self.assertIsNotNone(self.cpt.add_parameter("s2", "String parameter (level 2)", string_type))
        self.lst2 = self.cpt.add_list("l2", "List (level 2)", "Item")
        self.assertIsNotNone(self.lst2)
        self.ptn2 = self.lst2.get_pattern()
        self.assertIsNotNone(self.ptn2)

        # Create elements at level 3
        self.assertIsNotNone(self.ptn2.add_parameter("e3", "Enum parameter (level 3)", enum_type))
        self.assertIsNotNone(self.ptn2.add_parameter("s3", "String parameter (level 3)", string_type))

    def _test(self, value_paths, direct_referenced_paths, indirect_referenced_paths, target, element, targetpath):
        # Add reference
        if target is not None:
            self.assertIsNotNone(element)
            self.assertTrue(self.model.set_reference(element, target))
            self.assertIsNotNone(element.get_reference_target())
            self.assertIsNotNone(element.get_reference_data())
            expected = element.get_reference_data()
            self.assertIsNotNone(expected)
            self.assertIsInstance(expected, OpenSandConf.DataString)
            self.assertFalse(expected.is_set())
            self.assertIsNone(self.model.create_data())
            self.assertTrue(expected.set("val1"))

        # Create datamodel
        datamodel = self.model.create_data()
        self.assertIsNotNone(datamodel)

        datalst2 = datamodel.get_item_by_path("/c/l2")
        self.assertIsNotNone(datalst2)
        self.assertIsInstance(datalst2, OpenSandConf.DataList)
        self.assertIsNotNone(datalst2.add_item())
        self.assertIsNotNone(datalst2.add_item())
        self.assertFalse(datamodel.validate())

        # Fill datamodel
        for path in value_paths:
            param = datamodel.get_item_by_path(path)
            self.assertIsNotNone(param)
            self.assertIsInstance(param, OpenSandConf.DataParameter)
            data = param.get_data()
            self.assertIsNotNone(data)
            self.assertIsInstance(data, OpenSandConf.DataString)

            self.assertFalse(data.is_set())
            self.assertTrue(param.check_reference())
            self.assertTrue(data.set("val2"))

        for path in direct_referenced_paths:
            param = datamodel.get_item_by_path(path)
            self.assertIsNotNone(param)
            self.assertIsInstance(param, OpenSandConf.DataParameter)
            data = param.get_data()
            self.assertIsNotNone(data)
            self.assertIsInstance(data, OpenSandConf.DataString)

            self.assertFalse(data.is_set())
            self.assertFalse(param.check_reference())

        for path in indirect_referenced_paths:
            param = datamodel.get_item_by_path(path)
            self.assertIsNotNone(param)
            self.assertIsInstance(param, OpenSandConf.DataParameter)
            data = param.get_data()
            self.assertIsNotNone(data)
            self.assertIsInstance(data, OpenSandConf.DataString)

            self.assertFalse(data.is_set())
            self.assertTrue(param.check_reference())

        self.assertTrue(datamodel.validate())

        if targetpath != "":
            param = datamodel.get_item_by_path(targetpath)
            self.assertIsNotNone(param)
            self.assertIsInstance(param, OpenSandConf.DataParameter)
            data = param.get_data()
            self.assertIsNotNone(data)
            self.assertIsInstance(data, OpenSandConf.DataString)

            self.assertTrue(data.is_set())
            self.assertTrue(data.set("val1"))
            self.assertFalse(datamodel.validate())

    def test_no_reference(self):
        value_paths = [
                "/e",
                "/s",
                "/c/e2",
                "/c/s2",
                "/c/l2/0/e3",
                "/c/l2/0/s3",
                "/c/l2/1/e3",
                "/c/l2/1/s3",
        ]
        direct_referenced_paths = [
        ]
        indirect_referenced_paths = [
        ]
        target = None
        element = None
        targetpath = ""
        self._test(value_paths, direct_referenced_paths, indirect_referenced_paths, target, element, targetpath)

    def test_lower_element_with_reference(self):
        value_paths = [
                "/e",
                "/s",
                "/c/e2",
                "/c/l2/0/e3",
                "/c/l2/0/s3",
                "/c/l2/1/e3",
                "/c/l2/1/s3",
        ]
        direct_referenced_paths = [
                "/c/s2",
        ]
        indirect_referenced_paths = [
        ]
        target = self.model.get_item_by_path("/e")
        element = self.model.get_item_by_path("/c/s2")
        targetpath = target.get_path()
        self._test(value_paths, direct_referenced_paths, indirect_referenced_paths, target, element, targetpath)

    def test_upper_element_with_reference(self):
        value_paths = [
                "/e",
                "/s",
                "/c/e2",
                "/c/s2",
        ]
        direct_referenced_paths = [
        ]
        indirect_referenced_paths = [
                "/c/l2/0/e3",
                "/c/l2/0/s3",
                "/c/l2/1/e3",
                "/c/l2/1/s3",
        ]
        target = self.model.get_item_by_path("/e")
        element = self.model.get_item_by_path("/c")
        targetpath = target.get_path()
        self._test(value_paths, direct_referenced_paths, indirect_referenced_paths, target, element, targetpath)

    def test_element_in_list_item_with_reference(self):
        value_paths = [
                "/e",
                "/s",
                "/c/e2",
                "/c/s2",
                "/c/l2/0/e3",
                "/c/l2/1/e3",
        ]
        direct_referenced_paths = [
                "/c/l2/0/s3",
                "/c/l2/1/s3",
        ]
        indirect_referenced_paths = [
        ]
        target = self.model.get_item_by_path("/e")
        element = self.model.get_item_by_path("/c/l2/*/s3")
        targetpath = target.get_path()
        self._test(value_paths, direct_referenced_paths, indirect_referenced_paths, target, element, targetpath)

    def test_element_in_list_item_with_reference_in_list_pattern(self):
        value_paths = [
                "/e",
                "/s",
                "/c/e2",
                "/c/s2",
                "/c/l2/0/e3",
                "/c/l2/1/e3",
        ]
        direct_referenced_paths = [
                "/c/l2/0/s3",
                "/c/l2/1/s3",
        ]
        indirect_referenced_paths = [
        ]
        target = self.model.get_item_by_path("/c/l2/*/e3")
        element = self.model.get_item_by_path("/c/l2/*/s3")
        targetpath = "/c/l2/1/e3"
        self._test(value_paths, direct_referenced_paths, indirect_referenced_paths, target, element, targetpath)


class ModelIOTests(unittest.TestCase):
    def setUp(self):
        self.version = "1.2.3"
        self.model = OpenSandConf.MetaModel(self.version)
        self.root = self.model.get_root()
        self.types = self.model.get_types_definition()

        # Add enum type
        self.assertIsNotNone(self.types.add_enum_type("enum1", "Enum 1", [ "val1", "val2"]))
        enum_type = self.types.get_type("enum1")
        string_type = self.types.get_type("string")

        # Create elements at level 1
        self.assertIsNotNone(self.root.add_parameter("e", "Enum parameter (level 1)", enum_type))
        self.assertIsNotNone(self.root.add_parameter("s", "String parameter (level 1)", string_type))
        self.cpt = self.root.add_component("c", "Component (level 1)")
        self.assertIsNotNone(self.cpt)

        # Create elements at level 2
        self.assertIsNotNone(self.cpt.add_parameter("e2", "Enum parameter (level 2)", enum_type))
        self.assertIsNotNone(self.cpt.add_parameter("s2", "String parameter (level 2)", string_type))
        self.cpt2 = self.cpt.add_component("c2", "Component (level 2)")
        self.assertIsNotNone(self.cpt2)

        # Create elements at level 3
        self.assertIsNotNone(self.cpt2.add_parameter("e3", "Enum parameter (level 3)", enum_type))
        self.assertIsNotNone(self.cpt2.add_parameter("s3", "String parameter (level 3)", string_type))
        self.lst3 = self.cpt2.add_list("l3", "List (level 3)", "Item")
        self.assertIsNotNone(self.lst3)
        self.ptn3 = self.lst3.get_pattern()
        self.assertIsNotNone(self.ptn3)
        self.lst3b = self.cpt2.add_list("l3b", "List 2 (level 3)", "Item")
        self.assertIsNotNone(self.lst3b)
        self.ptn3b = self.lst3b.get_pattern()
        self.assertIsNotNone(self.ptn3b)

        # Create elements at level 4
        self.assertIsNotNone(self.ptn3.add_parameter("e4", "Enum parameter (level 4)", enum_type))
        self.assertIsNotNone(self.ptn3.add_parameter("s4", "String parameter (level 4)", string_type))
        self.assertIsNotNone(self.ptn3b.add_parameter("e4", "Enum parameter (level 4)", enum_type))
        self.assertIsNotNone(self.ptn3b.add_parameter("s4", "String parameter (level 4)", string_type))
        self.lst4 = self.ptn3.add_list("l4", "List (level 4)", "Item")
        self.assertIsNotNone(self.lst4)
        self.ptn4 = self.lst4.get_pattern()
        self.assertIsNotNone(self.ptn4)

        # Create elements at level 5
        self.assertIsNotNone(self.ptn4.add_parameter("e5", "Enum parameter (level 5)", enum_type))
        self.assertIsNotNone(self.ptn4.add_parameter("s5", "String parameter (level 5)", string_type))
        self.cpt5 = self.ptn4.add_component("c5", "Component (level 5)")

        # Create elements at level 6
        self.assertIsNotNone(self.cpt5.add_parameter("e6", "Enum parameter (level 6)", enum_type))
        self.assertIsNotNone(self.cpt5.add_parameter("s6", "String parameter (level 6)", string_type))

        self.datamodel = self.model.create_data()
        self.assertIsNotNone(self.datamodel)
        self.dataroot = self.datamodel.get_root()
        self.assertIsNotNone(self.dataroot)

        # Build data model
        p = self.dataroot.get_parameter('e')
        self.assertIsNotNone(p)
        self.assertTrue(p.get_data().from_string('val1'))
        p = self.dataroot.get_parameter('s')
        self.assertIsNotNone(p)
        self.assertTrue(p.get_data().from_string('val2'))

        self.datacpt = self.dataroot.get_component('c')
        self.assertIsNotNone(self.datacpt)
        p = self.datacpt.get_parameter('e2')
        self.assertIsNotNone(p)
        self.assertTrue(p.get_data().from_string('val1'))
        p = self.datacpt.get_parameter('s2')
        self.assertIsNotNone(p)
        self.assertTrue(p.get_data().from_string('val2'))

        self.datacpt2 = self.datacpt.get_component('c2')
        self.assertIsNotNone(self.datacpt2)
        p = self.datacpt2.get_parameter('e3')
        self.assertIsNotNone(p)
        self.assertTrue(p.get_data().from_string('val1'))
        p = self.datacpt2.get_parameter('s3')
        self.assertIsNotNone(p)
        self.assertTrue(p.get_data().from_string('val2'))

        self.datalst3 = self.datacpt2.get_list('l3')
        self.assertIsNotNone(self.datalst3)
        for i in range(0, 3):
            c3 = self.datalst3.add_item()
            self.assertIsNotNone(c3)
            p = c3.get_parameter('e4')
            self.assertIsNotNone(p)
            self.assertTrue(p.get_data().from_string('val1'))
            p = c3.get_parameter('s4')
            self.assertIsNotNone(p)
            self.assertTrue(p.get_data().from_string('val2'))

            datalst4 = c3.get_list('l4')
            self.assertIsNotNone(datalst4)
            for j in range(0, 2):
                c4 = datalst4.add_item()
                self.assertIsNotNone(c4)
                p = c4.get_parameter('e5')
                self.assertIsNotNone(p)
                self.assertTrue(p.get_data().from_string('val1'))
                p = c4.get_parameter('s5')
                self.assertIsNotNone(p)
                self.assertTrue(p.get_data().from_string('val2'))

                c5 = c4.get_component('c5')
                self.assertIsNotNone(c5)
                p = c5.get_parameter('e6')
                self.assertIsNotNone(p)
                self.assertTrue(p.get_data().from_string('val1'))
                p = c5.get_parameter('s6')
                self.assertIsNotNone(p)
                self.assertTrue(p.get_data().from_string('val2'))
        self.assertIsNotNone(self.datalst3.add_item())

        self.datalst3b = self.datacpt2.get_list('l3b')
        self.assertIsNotNone(self.datalst3b)
        for i in range(0, 5):
            self.assertIsNotNone(self.datalst3b.add_item())

    def _read_file_content(self, filepath):
        content = ''
        with open(filepath, 'r') as f:
            content = f.read()
        return content

    def test_read_write_meta(self):
        path = "my_py_model.xsd"
        path2 = "my_py_model2.xsd"
        if os.path.isfile(path):
            os.remove(path)
        if os.path.isfile(path2):
            os.remove(path2)

        # Test writing model to XSD
        self.assertTrue(OpenSandConf.toXSD(self.model, path))

        # Test reading model from XSD
        model2 = OpenSandConf.fromXSD(path)
        self.assertIsNotNone(model2)

        # Compare XSD files
        content = self._read_file_content(path)
        self.assertTrue(OpenSandConf.toXSD(model2, path2))
        content2 = self._read_file_content(path2)
        self.assertEqual(content, content2)

    def test_read_write_meta_with_ref(self):
        path = "my_py_model_ref.xsd"
        path2 = "my_py_model_ref2.xsd"
        if os.path.isfile(path):
            os.remove(path)
        if os.path.isfile(path2):
            os.remove(path2)

        # Add reference 1
        target = self.root.get_parameter('e')
        self.assertIsNotNone(target)
        element = self.cpt2.get_parameter('s3')
        self.assertIsNotNone(element)
        self.assertTrue(self.model.set_reference(element, target))
        self.assertIsNotNone(element.get_reference_target())
        self.assertEqual(element.get_reference_target().get_path(), target.get_path())
        expected = element.get_reference_data()
        self.assertIsNotNone(expected)
        self.assertTrue(expected.set('val1'))

        # Add reference 2
        target2 = self.model.get_item_by_path('/c/e2')
        self.assertIsNotNone(target2)
        element2 = self.model.get_item_by_path('/c/c2/l3/*/s4')
        self.assertIsNotNone(element2)
        self.assertTrue(self.model.set_reference(element2, target2))
        self.assertIsNotNone(element2.get_reference_target())
        self.assertEqual(element2.get_reference_target().get_path(), target2.get_path())
        expected2 = element2.get_reference_data()
        self.assertIsNotNone(expected2)
        self.assertTrue(expected2.set('val1'))

        # Test writing model to XSD
        self.assertTrue(OpenSandConf.toXSD(self.model, path))

        # Test reading model from XSD
        model2 = OpenSandConf.fromXSD(path)
        self.assertIsNotNone(model2)

        # Compare XSD files
        content = self._read_file_content(path)
        self.assertTrue(OpenSandConf.toXSD(model2, path2))
        content2 = self._read_file_content(path2)
        self.assertEqual(content, content2)

    def test_read_write_data(self):
        path = "my_py_datamodel.xml"
        path2 = "my_py_datamodel2.xml"
        if os.path.isfile(path):
            os.remove(path)
        if os.path.isfile(path2):
            os.remove(path2)

        # Test writing datamodel to XML
        self.assertTrue(OpenSandConf.toXML(self.datamodel, path))

        # Test reading datamodel from XML
        datamodel2 = OpenSandConf.fromXML(self.model, path)
        self.assertIsNotNone(datamodel2)

        # Compare XSD files
        content = self._read_file_content(path)
        self.assertTrue(OpenSandConf.toXML(datamodel2, path2))
        content2 = self._read_file_content(path2)
        self.assertEqual(content, content2)
 

if __name__ == "__main__":
    unittest.main()
