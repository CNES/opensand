import os
import shlex
import shutil
import tarfile
import tempfile
import traceback
from io import BytesIO
from pathlib import Path
from types import SimpleNamespace

from fabric import Connection
from paramiko.client import MissingHostKeyPolicy
from flask import Flask, request, jsonify, send_file
from werkzeug.exceptions import HTTPException

import py_opensand_conf


app = Flask(__name__)


try:
    from flask_cors import CORS
except ImportError:
    pass
else:
    CORS(app)


MODELS_FOLDER = Path(__file__).parent.resolve()
WWW_FOLDER = MODELS_FOLDER.parent / 'www'


DVB_S2 = [
        (1, 'QPSK', '1/4', 0.490, -2.35),
        (2, 'QPSK', '1/3', 0.656, -1.24),
        (3, 'QPSK', '2/5', 0.789, -0.30),
        (4, 'QPSK', '1/2', 0.988, 1.00),
        (5, 'QPSK', '3/5', 1.188, 2.23),
        (6, 'QPSK', '2/3', 1.322, 3.10),
        (7, 'QPSK', '3/4', 1.487, 4.03),
        (8, 'QPSK', '4/5', 1.587, 4.68),
        (9, 'QPSK', '5/6', 1.655, 5.18),
        (10, 'QPSK', '8/9', 1.767, 6.20),
        (11, 'QPSK', '9/10', 1.789, 6.42),
        (12, '8PSK', '3/5', 1.780, 5.50),
        (13, '8PSK', '2/3', 1.981, 6.62),
        (14, '8PSK', '3/4', 2.228, 7.91),
        (15, '8PSK', '5/6', 2.479, 9.35),
        (16, '8PSK', '8/9', 2.646, 10.69),
        (17, '8PSK', '9/10', 2.679, 10.98),
        (18, '16APSK', '2/3', 2.637, 8.97),
        (19, '16APSK', '3/4', 2.967, 10.21),
        (20, '16APSK', '4/5', 3.166, 11.03),
        (21, '16APSK', '5/6', 3.300, 11.61),
        (22, '16APSK', '8/9', 3.523, 12.89),
        (23, '16APSK', '9/10', 3.567, 13.13),
        (24, '32APSK', '3/4', 3.703, 12.73),
        (25, '32APSK', '4/5', 3.952, 13.64),
        (26, '32APSK', '5/6', 4.120, 14.28),
        (27, '32APSK', '8/9', 4.398, 15.69),
        (28, '32APSK', '9/10', 4.453, 16.05),
]


DVB_RCS2 = [
        (3, 'QPSK', '1/3', 0.56, 0.22, '536 sym'),
        (4, 'QPSK', '1/2', 0.87, 2.34, '536 sym'),
        (5, 'QPSK', '2/3', 1.26, 4.29, '536 sym'),
        (6, 'QPSK', '3/4', 1.42, 5.36, '536 sym'),
        (7, 'QPSK', '5/6', 1.60, 6.68, '536 sym'),
        (8, '8PSK', '2/3', 1.70, 8.08, '536 sym'),
        (9, '8PSK', '3/4', 1.93, 9.31, '536 sym'),
        (10, '8PSK', '5/6', 2.13, 10.82, '536 sym'),
        (11, '16QAM', '3/4', 2.59, 11.17, '536 sym'),
        (12, '16QAM', '5/6', 2.87, 12.56, '536 sym'),
        (13, 'QPSK', '1/3', 0.61, -0.51, '1616 sym'),
        (14, 'QPSK', '1/2', 0.93, 1.71, '1616 sym'),
        (15, 'QPSK', '2/3', 1.30, 3.69, '1616 sym'),
        (16, 'QPSK', '3/4', 1.47, 4.73, '1616 sym'),
        (17, 'QPSK', '5/6', 1.64, 5.94, '1616 sym'),
        (18, '8PSK', '2/3', 1.75, 7.49, '1616 sym'),
        (19, '8PSK', '3/4', 1.98, 8.77, '1616 sym'),
        (20, '8PSK', '5/6', 2.19, 10.23, '1616 sym'),
        (21, '16QAM', '3/4', 2.66, 10.72, '1616 sym'),
        (22, '16QAM', '5/6', 2.96, 12.04, '1616 sym'),
]


FIFOS = [
        (0, 'NM', 1000, 'DAMA_RBDC', 'ACM'),
        (1, 'EF', 3000, 'DAMA_RBDC', 'ACM'),
        (2, 'SIG', 1000, 'DAMA_RBDC', 'ACM'),
        (3, 'AF', 2000, 'DAMA_RBDC', 'ACM'),
        (4, 'BE', 6000, 'DAMA_RBDC', 'ACM'),
]


QOS_CLASSES = [
        (7, 'NC', 'NM'),
        (6, 'IC', 'SIG'),
        (5, 'VO', 'EF'),
        (4, 'VI', 'AF'),
        (3, 'CA', 'AF'),
        (2, 'EE', 'AF'),
        (1, 'BK', 'BE'),
        (0, 'BE', 'BE'),
]


def success(message='OK', **kwargs):
    result = {'status': message, **kwargs}
    return jsonify(result)


def error(message, return_code=500, **kwargs):
    result = {'error': message, **kwargs}
    return jsonify(result), return_code


@app.errorhandler(Exception)
def handle_exception(exc):
    if isinstance(exc, HTTPException):
        return exc
    print(traceback.format_exc())
    return error(traceback.format_exc())


def get_file_content(filename, xml=False):
    expected_suffix = '.xml' if xml else '.xsd'
    if not filename.endswith(expected_suffix):
        filename += expected_suffix

    base_folder = WWW_FOLDER if xml else MODELS_FOLDER
    filepath = base_folder / filename
    try:
        with filepath.open() as f:
            content = f.read()
    except OSError:
        folder = filepath.relative_to(base_folder).parent
        return error('cannot find or read {} in {}'.format(filepath.name, folder), 404)

    return success(content=content)


def write_file_content(filename, content, expected_suffix='.xml'):
    if not filename.endswith(expected_suffix):
        filename += expected_suffix

    filepath = WWW_FOLDER.joinpath(filename)
    filepath.parent.mkdir(parents=True, exist_ok=True)
    with filepath.open('w') as f:
        f.write(content)

    return success()


def normalize_xsd_folder(folder_name):
    folder = Path(folder_name)
    if not folder.suffixes or folder.suffixes[0] != '.xsd':
        folder = folder.with_name(folder.name + '.xsd')

    if not folder.suffixes or folder.suffixes[-1] != '.d':
        folder = folder.with_name(folder.name + '.d')

    return folder.name


def read_xml_file(filepath, xsd_name):
    if not xsd_name.endswith('.xsd'):
        xsd_name += '.xsd'

    xsd = py_opensand_conf.fromXSD(MODELS_FOLDER.joinpath(xsd_name).as_posix())
    if xsd is not None:
        return py_opensand_conf.fromXML(xsd, filepath.as_posix())


def _get_component(component, name):
    if component is None:
        return None

    return component.get_component(name)


def _create_list_item(component, name):
    if component is None:
        return None

    lst = component.get_list(name)
    if lst is None:
        return None

    return lst.add_item()


def _set_parameter(component, name, value):
    if component is None:
        return False

    parameter = component.get_parameter(name)
    if parameter is None:
        return False

    try:
        return parameter.get_data().set(value)
    except TypeError:
        return False


def _get_parameter(component, name, default=None):
    if component is None:
        return None

    parameter = component.get_parameter(name)
    if parameter is None:
        return None

    data = parameter.get_data()
    if not data.is_set():
        return default
    return data.get()


def create_default_infrastructure(meta_model):
    infra = meta_model.create_data()
    infrastructure = infra.get_root()

    logs = _get_component(infrastructure, 'logs')
    for log in ('init', 'lan_adaptation', 'encap', 'dvb', 'physical_layer', 'sat_carrier'):
        _set_parameter(_get_component(logs, log), 'level', 'warning')

    entity = _get_component(infrastructure, 'entity')
    _set_parameter(entity, 'entity_type', 'Satellite')

    satellite = _get_component(entity, 'entity_sat')
    _set_parameter(satellite, 'entity_id', 2)
    _set_parameter(satellite, 'emu_address', '192.168.0.63')
    _set_parameter(satellite, 'default_entity', -1)

    satellite_regen = _get_component(entity, 'entity_sat_regen')
    _set_parameter(satellite_regen, 'entity_id', 2)
    _set_parameter(satellite_regen, 'emu_address', '192.168.0.63')
    _set_parameter(satellite_regen, 'regen_level', "IP")
    _set_parameter(satellite_regen, 'mesh', True)
    _set_parameter(satellite_regen, 'default_entity', -1)

    terminal = _get_component(entity, 'entity_st')
    _set_parameter(terminal, 'entity_id', 1)
    _set_parameter(terminal, 'emu_address', '192.168.0.10')
    _set_parameter(terminal, 'tap_iface', 'opensand_tap')
    _set_parameter(terminal, 'mac_address', 'FF:FF:FF:00:00:10')

    gateway = _get_component(entity, 'entity_gw')
    _set_parameter(gateway, 'entity_id', 0)
    _set_parameter(gateway, 'emu_address', '192.168.0.1')
    _set_parameter(gateway, 'tap_iface', 'opensand_tap')
    _set_parameter(gateway, 'mac_address', 'FF:FF:FF:00:00:01')

    gateway_net_acc = _get_component(entity, 'entity_gw_net_acc')
    _set_parameter(gateway_net_acc, 'entity_id', 0)
    _set_parameter(gateway_net_acc, 'tap_iface', 'opensand_tap')
    _set_parameter(gateway_net_acc, 'mac_address', 'FF:FF:FF:00:00:01')
    _set_parameter(gateway_net_acc, 'interconnect_address', '192.168.1.1')
    _set_parameter(gateway_net_acc, 'interconnect_remote', '192.168.1.2')

    gateway_phy = _get_component(entity, 'entity_gw_phy')
    _set_parameter(gateway_phy, 'entity_id', 0)
    _set_parameter(gateway_phy, 'emu_address', '192.168.0.1')
    _set_parameter(gateway_phy, 'interconnect_address', '192.168.1.2')
    _set_parameter(gateway_phy, 'interconnect_remote', '192.168.1.1')

    return infra


def create_default_topology(meta_model):
    topo = meta_model.create_data()
    topology = topo.get_root()

    spot = _create_list_item(_get_component(topology, 'frequency_plan'), 'spots')
    _set_parameter(_get_component(spot, 'assignments'), 'gateway_id', 0)
    _set_parameter(_get_component(spot, 'assignments'), 'satellite_id', 2)
    _set_parameter(_get_component(spot, 'roll_off'), 'forward', 0.35)
    _set_parameter(_get_component(spot, 'roll_off'), 'return', 0.2)
    forward_carrier = _create_list_item(spot, 'forward_band')
    _set_parameter(forward_carrier, 'symbol_rate', 40e6)
    _set_parameter(forward_carrier, 'type', 'ACM')
    _set_parameter(forward_carrier, 'wave_form', '1-28')
    _set_parameter(forward_carrier, 'group', 'Standard')
    return_carrier = _create_list_item(spot, 'return_band')
    _set_parameter(return_carrier, 'symbol_rate', 40e6)
    _set_parameter(return_carrier, 'type', 'DAMA')
    _set_parameter(return_carrier, 'wave_form', '3-12')
    _set_parameter(return_carrier, 'group', 'Standard')

    default_assignment = _get_component(_get_component(topology, 'st_assignment'), 'defaults')
    _set_parameter(default_assignment, 'default_gateway', 0)
    _set_parameter(default_assignment, 'default_group', 'Standard')

    wave_forms = _get_component(topology, 'wave_forms')
    for i, modulation, coding, efficiency, es_n0 in DVB_S2:
        wave_form = _create_list_item(wave_forms, 'dvb_s2')
        _set_parameter(wave_form, 'id', i)
        _set_parameter(wave_form, 'modulation', modulation)
        _set_parameter(wave_form, 'coding', coding)
        _set_parameter(wave_form, 'efficiency', efficiency)
        _set_parameter(wave_form, 'threshold', es_n0)
    for i, modulation, coding, efficiency, es_n0, burst in DVB_RCS2:
        wave_form = _create_list_item(wave_forms, 'dvb_rcs2')
        _set_parameter(wave_form, 'id', i)
        _set_parameter(wave_form, 'modulation', modulation)
        _set_parameter(wave_form, 'coding', coding)
        _set_parameter(wave_form, 'efficiency', efficiency)
        _set_parameter(wave_form, 'threshold', es_n0)
        _set_parameter(wave_form, 'burst_length', burst)

    advanced = _get_component(topology, 'advanced_settings')
    links = _get_component(advanced, 'links')
    _set_parameter(links, 'forward_duration', 10.0)
    _set_parameter(links, 'forward_margin', 0.0)
    _set_parameter(links, 'return_duration', 26.5)
    _set_parameter(links, 'return_margin', 0.0)
    schedulers = _get_component(advanced, 'schedulers')
    _set_parameter(schedulers, 'burst_length', '536 sym')
    _set_parameter(schedulers, 'crdsa_frame', 3)
    _set_parameter(schedulers, 'crdsa_delay', 250)
    _set_parameter(schedulers, 'pep_allocation', 1000)
    timers = _get_component(advanced, 'timers')
    _set_parameter(timers, 'statistics', 53)
    _set_parameter(timers, 'synchro', 1000)
    _set_parameter(timers, 'acm_refresh', 1000)
    delay = _get_component(advanced, 'delay')
    _set_parameter(delay, 'fifo_size', 10000)
    _set_parameter(delay, 'delay_timer', 1)

    return topo

def create_default_profile(meta_model, entity_type):
    mod = meta_model.create_data()
    model = mod.get_root()

    encapsulation = _get_component(model, 'encap')
    _set_parameter(_get_component(encapsulation, 'gse'), 'packing_threshold', 3)
    _set_parameter(_get_component(encapsulation, 'rle'), 'alpdu_protection', 'Sequence Number')

    access = _get_component(model, 'access')
    _set_parameter(_get_component(access, 'random_access'), 'saloha_algo', 'CRDSA')
    _set_parameter(_get_component(access, 'settings'), 'category', 'Standard')
    _set_parameter(_get_component(access, 'settings'), 'dama_enabled', True)
    dama = _get_component(access, 'dama')
    _set_parameter(dama, 'cra', 100)
    _set_parameter(dama, 'algorithm', 'Legacy')
    _set_parameter(dama, 'duration', 23)

    phy_layer = _get_component(model, 'physical_layer')
    delay = _get_component(phy_layer, 'delay')
    _set_parameter(delay, 'delay_type', 'ConstantDelay')
    _set_parameter(delay, 'delay_value', 125)
    minimal_condition = _get_component(phy_layer, 'minimal_condition')
    _set_parameter(minimal_condition, 'minimal_condition_type', 'ACM-Loop')
    error_insertion = _get_component(phy_layer, 'error_insertion')
    _set_parameter(error_insertion, 'error_insertion_type', 'Gate')
    uplink_attenuation = _get_component(phy_layer, 'uplink_attenuation')
    _set_parameter(uplink_attenuation, 'clear_sky', 20.0)
    _set_parameter(uplink_attenuation, 'attenuation_type', 'Ideal')
    _set_parameter(uplink_attenuation, 'ideal_attenuation_value', 0.0)
    downlink_attenuation = _get_component(phy_layer, 'downlink_attenuation')
    _set_parameter(downlink_attenuation, 'clear_sky', 20.0)
    _set_parameter(downlink_attenuation, 'attenuation_type', 'Ideal')
    _set_parameter(downlink_attenuation, 'ideal_attenuation_value', 0.0)

    network = _get_component(model, 'network')
    _set_parameter(network, 'simulation', 'None')
    _set_parameter(network, 'fca', 0)
    _set_parameter(network, 'dama_algorithm', 'Legacy')
    if entity_type in ['st', 'sat_regen']:
        for priority, name, capacity, access_st, access_gw in FIFOS:
            fifo = _create_list_item(network, 'st_fifos')
            _set_parameter(fifo, 'priority', priority)
            _set_parameter(fifo, 'name', name)
            _set_parameter(fifo, 'capacity', capacity)
            _set_parameter(fifo, 'access_type', access_st)
    if entity_type in ['gw', 'gw_net_acc', 'sat_regen']:
        for priority, name, capacity, access_st, access_gw in FIFOS:
            fifo = _create_list_item(network, 'gw_fifos')
            _set_parameter(fifo, 'priority', priority)
            _set_parameter(fifo, 'name', name)
            _set_parameter(fifo, 'capacity', capacity)
            _set_parameter(fifo, 'access_type', access_gw)
    
    for pcp, name, fifo in QOS_CLASSES:
        qos = _create_list_item(network, 'qos_classes')
        _set_parameter(qos, 'pcp', pcp)
        _set_parameter(qos, 'name', name)
        _set_parameter(qos, 'fifo', fifo)
    settings = _get_component(network, 'qos_settings')
    _set_parameter(settings, 'lan_frame_type', 'Ethernet')
    _set_parameter(settings, 'sat_frame_type', 'Ethernet')
    _set_parameter(settings, 'default_pcp', 0)
    
    control_plane = _get_component(model, 'control_plane')
    _set_parameter(control_plane, 'disable_control_plane',
                   entity_type == 'sat_regen')
                   
    scpc = _get_component(_get_component(model, 'access2'), 'scpc')
    _set_parameter(scpc, "carrier_duration", 5)
    scpc = _get_component(_get_component(model, 'access'), 'scpc')
    _set_parameter(scpc, "carrier_duration", 5)
    
    return mod


def create_scpc_topology(meta_model):
    topo = create_default_topology(meta_model)
    
    root = topo.get_root()
    freq_plan = _get_component(root, 'frequency_plan')
    spots = freq_plan.get_list("spots")
    spot = spots.get_item("0")
    
    return_carrier = _create_list_item(spot, 'return_band')
    _set_parameter(return_carrier, 'symbol_rate', 40e6)
    _set_parameter(return_carrier, 'type', 'SCPC')
    _set_parameter(return_carrier, 'wave_form', '3-12')
    _set_parameter(return_carrier, 'group', 'Standard')
    
    return topo


def create_sat_ctrl_plane_profile(meta_model, entity_type):
    mod = create_default_profile(meta_model, entity_type)
    
    root = mod.get_root()
    control_plane = _get_component(root, 'control_plane')
    _set_parameter(control_plane, 'disable_control_plane',
                   entity_type != 'sat_regen')
    
    return mod


def create_default_templates(project):
    XSDs = (
            filepath
            for filepath in MODELS_FOLDER.iterdir()
            if filepath.suffix == '.xsd' and filepath.is_file()
    )

    template_folder = WWW_FOLDER / project / 'templates'
    for xsd in XSDs:
        meta_model = py_opensand_conf.fromXSD(xsd.as_posix())
        if meta_model is not None and xsd.stem != 'project':
            template_path = template_folder / (xsd.name + '.d') / 'Default.xml'
            template_path.parent.mkdir(parents=True, exist_ok=True)
            kind = meta_model.get_root().get_description()
            if kind == 'infrastructure':
                template = create_default_infrastructure(meta_model)
                py_opensand_conf.toXML(template, str(template_path))
            elif kind == 'topology':
                template = create_default_topology(meta_model)
                py_opensand_conf.toXML(template, str(template_path))
                scpc_template_path = template_folder / (xsd.name + '.d') / 'SCPC.xml'
                scpc_template = create_scpc_topology(meta_model)
                py_opensand_conf.toXML(scpc_template, str(scpc_template_path))
            elif kind == 'profile':
                entity_name = xsd.stem.removeprefix("profile_")
                template = create_default_profile(meta_model, entity_name)
                py_opensand_conf.toXML(template, str(template_path))
                if entity_name != "gw_phy":
                    sat_ctrl_plane_template_path = template_folder / (xsd.name + '.d') / 'Control plane handled by satellite.xml'
                    regen_template = create_sat_ctrl_plane_profile(meta_model, entity_name)
                    py_opensand_conf.toXML(regen_template, str(sat_ctrl_plane_template_path))


def create_platform_infrastructure(project):
    project_layout = read_xml_file(WWW_FOLDER / project / 'project.xml', 'project.xsd')
    if project_layout is None:
        return

    root = project_layout.get_root().get_component('configuration')
    if root is None:
        return

    entities = root.get_list('entities')
    if entities is None:
        return

    infrastructure = {
            'satellite': {},
            'gateways': {},
            'terminals': {},
    }
    for entity_id, _ in enumerate(entities.get_items()):
        # Can't directly use the items iterated over because of bad cast;
        # so retrieve them one by one instead to get the proper type.
        entity = entities.get_item(str(entity_id))
        name = _get_parameter(entity, 'entity_name')
        infra = _get_parameter(entity, 'infrastructure')
        if not name or not infra:
            continue

        filepath = WWW_FOLDER / project / 'entities' / name / 'infrastructure.xml'
        xml = read_xml_file(filepath, infra)
        if xml is None:
            continue

        entity = xml.get_root().get_component('entity')
        entity_type = _get_parameter(entity, 'entity_type')
        if entity_type == "Satellite":
            entity_sat = entity.get_component('entity_sat')
            entity_id = _get_parameter(entity_sat, 'entity_id')
            if entity_id is not None:
                satellite = {'entity_id': entity_id}
                satellite['emu_address'] = _get_parameter(entity_sat, 'emu_address', '')
                infrastructure['satellite'][entity_id] = satellite
        elif entity_type == "Satellite Regen":
            entity_sat_regen = entity.get_component('entity_sat_regen')
            entity_id = _get_parameter(entity_sat_regen, 'entity_id')
            if entity_id is not None:
                satellite_regen = {'entity_id': entity_id}
                satellite_regen['emu_address'] = _get_parameter(entity_sat_regen, 'emu_address', '')
                satellite_regen['default_entity'] = _get_parameter(entity_sat_regen, 'default_entity', -1)
                satellite_regen['isl_port'] = _get_parameter(entity_sat_regen, 'isl_port')
                infrastructure['satellite'][entity_id] = satellite_regen
        elif entity_type == "Gateway":
            entity_gw = entity.get_component('entity_gw')
            entity_id = _get_parameter(entity_gw, 'entity_id')
            if entity_id is not None:
                gateway = {'entity_id': entity_id}
                gateway['emu_address'] = _get_parameter(entity_gw, 'emu_address')
                gateway['mac_address'] = _get_parameter(entity_gw, 'mac_address')
                gateway['ctrl_multicast_address'] = _get_parameter(entity_gw, 'ctrl_multicast_address')
                gateway['data_multicast_address'] = _get_parameter(entity_gw, 'data_multicast_address')
                gateway['ctrl_out_st_port'] = _get_parameter(entity_gw, 'ctrl_out_st_port')
                gateway['ctrl_in_st_port'] = _get_parameter(entity_gw, 'ctrl_in_st_port')
                gateway['ctrl_out_gw_port'] = _get_parameter(entity_gw, 'ctrl_out_gw_port')
                gateway['ctrl_in_gw_port'] = _get_parameter(entity_gw, 'ctrl_in_gw_port')
                gateway['logon_out_port'] = _get_parameter(entity_gw, 'logon_out_port')
                gateway['logon_in_port'] = _get_parameter(entity_gw, 'logon_in_port')
                gateway['data_out_st_port'] = _get_parameter(entity_gw, 'data_out_st_port')
                gateway['data_in_st_port'] = _get_parameter(entity_gw, 'data_in_st_port')
                gateway['data_out_gw_port'] = _get_parameter(entity_gw, 'data_out_gw_port')
                gateway['data_in_gw_port'] = _get_parameter(entity_gw, 'data_in_gw_port')
                gateway['udp_stack'] = _get_parameter(entity_gw, 'udp_stack')
                gateway['udp_rmem'] = _get_parameter(entity_gw, 'udp_rmem')
                gateway['udp_wmem'] = _get_parameter(entity_gw, 'udp_wmem')
                infrastructure['gateways'][entity_id] = gateway
        elif entity_type == "Gateway Net Access":
            entity_gw_net_acc = entity.get_component('entity_gw_net_acc')
            entity_id = _get_parameter(entity_gw_net_acc, 'entity_id')
            if entity_id is not None:
                gateway = infrastructure['gateways'].get(entity_id, {'entity_id': entity_id})
                gateway['mac_address'] = _get_parameter(entity_gw_net_acc, 'mac_address')
                infrastructure['gateways'][entity_id] = gateway
        elif entity_type == "Gateway Phy":
            entity_gw_phy = entity.get_component('entity_gw_phy')
            entity_id = _get_parameter(entity_gw_phy, 'entity_id')
            if entity_id is not None:
                gateway = infrastructure['gateways'].get(entity_id, {'entity_id': entity_id})
                gateway['emu_address'] = _get_parameter(entity_gw_phy, 'emu_address')
                gateway['ctrl_multicast_address'] = _get_parameter(entity_gw_phy, 'ctrl_multicast_address')
                gateway['data_multicast_address'] = _get_parameter(entity_gw_phy, 'data_multicast_address')
                gateway['ctrl_out_st_port'] = _get_parameter(entity_gw_phy, 'ctrl_out_st_port')
                gateway['ctrl_in_st_port'] = _get_parameter(entity_gw_phy, 'ctrl_in_st_port')
                gateway['ctrl_out_gw_port'] = _get_parameter(entity_gw_phy, 'ctrl_out_gw_port')
                gateway['ctrl_in_gw_port'] = _get_parameter(entity_gw_phy, 'ctrl_in_gw_port')
                gateway['logon_out_port'] = _get_parameter(entity_gw_phy, 'logon_out_port')
                gateway['logon_in_port'] = _get_parameter(entity_gw_phy, 'logon_in_port')
                gateway['data_out_st_port'] = _get_parameter(entity_gw_phy, 'data_out_st_port')
                gateway['data_in_st_port'] = _get_parameter(entity_gw_phy, 'data_in_st_port')
                gateway['data_out_gw_port'] = _get_parameter(entity_gw_phy, 'data_out_gw_port')
                gateway['data_in_gw_port'] = _get_parameter(entity_gw_phy, 'data_in_gw_port')
                gateway['udp_stack'] = _get_parameter(entity_gw_phy, 'udp_stack')
                gateway['udp_rmem'] = _get_parameter(entity_gw_phy, 'udp_rmem')
                gateway['udp_wmem'] = _get_parameter(entity_gw_phy, 'udp_wmem')
                infrastructure['gateways'][entity_id] = gateway
        elif entity_type == "Terminal":
            entity_st = entity.get_component('entity_st')
            entity_id = _get_parameter(entity_st, 'entity_id')
            if entity_id is not None:
                terminal = {'entity_id': entity_id}
                terminal['emu_address'] = _get_parameter(entity_st, 'emu_address')
                terminal['mac_address'] = _get_parameter(entity_st, 'mac_address')
                infrastructure['terminals'][entity_id] = terminal

    for entity_id, _ in enumerate(entities.get_items()):
        # Can't directly use the items iterated over because of bad cast;
        # so retrieve them one by one instead to get the proper type.
        entity = entities.get_item(str(entity_id))
        name = _get_parameter(entity, 'entity_name')
        infra = _get_parameter(entity, 'infrastructure')
        if not name or not infra:
            continue

        filepath = WWW_FOLDER / project / 'entities' / name / 'infrastructure.xml'
        xml = read_xml_file(filepath, infra)
        if xml is None:
            continue

        infra = xml.get_root().get_component('infrastructure')
        if infra is None:
            continue

        satellites = infra.get_list('satellites')
        if satellites is not None:
            satellites.clear_items()

        for satellite in infrastructure['satellite'].values():
            sat = _create_list_item(infra, 'satellites')
            _set_parameter(sat, 'entity_id', satellite.get('entity_id'))
            _set_parameter(sat, 'emu_address', satellite.get('emu_address'))
            _set_parameter(sat, 'isl_port', satellite.get('isl_port'))
            _set_parameter(sat, 'default_entity', satellite.get('default_entity'))

        _set_parameter(infra, 'default_gw', 0) # TODO

        gateways = infra.get_list('gateways')
        if gateways is not None:
            gateways.clear_items()

        for gateway in infrastructure['gateways'].values():
            gw = _create_list_item(infra, 'gateways')
            _set_parameter(gw, 'entity_id', gateway.get('entity_id'))
            _set_parameter(gw, 'emu_address', gateway.get('emu_address'))
            _set_parameter(gw, 'mac_address', gateway.get('mac_address'))
            _set_parameter(gw, 'ctrl_multicast_address', gateway.get('ctrl_multicast_address'))
            _set_parameter(gw, 'data_multicast_address', gateway.get('data_multicast_address'))
            _set_parameter(gw, 'ctrl_out_st_port', gateway.get('ctrl_out_st_port'))
            _set_parameter(gw, 'ctrl_in_st_port', gateway.get('ctrl_in_st_port'))
            _set_parameter(gw, 'ctrl_out_gw_port', gateway.get('ctrl_out_gw_port'))
            _set_parameter(gw, 'ctrl_in_gw_port', gateway.get('ctrl_in_gw_port'))
            _set_parameter(gw, 'logon_out_port', gateway.get('logon_out_port'))
            _set_parameter(gw, 'logon_in_port', gateway.get('logon_in_port'))
            _set_parameter(gw, 'data_out_st_port', gateway.get('data_out_st_port'))
            _set_parameter(gw, 'data_in_st_port', gateway.get('data_in_st_port'))
            _set_parameter(gw, 'data_out_gw_port', gateway.get('data_out_gw_port'))
            _set_parameter(gw, 'data_in_gw_port', gateway.get('data_in_gw_port'))
            _set_parameter(gw, 'udp_stack', gateway.get('udp_stack'))
            _set_parameter(gw, 'udp_rmem', gateway.get('udp_rmem'))
            _set_parameter(gw, 'udp_wmem', gateway.get('udp_wmem'))

        terminals = infra.get_list('terminals')
        if terminals is not None:
            terminals.clear_items()

        for terminal in infrastructure['terminals'].values():
            st = _create_list_item(infra, 'terminals')
            _set_parameter(st, 'entity_id', terminal.get('entity_id'))
            _set_parameter(st, 'emu_address', terminal.get('emu_address'))
            _set_parameter(st, 'mac_address', terminal.get('mac_address'))

        py_opensand_conf.toXML(xml, filepath.as_posix())


def extract_emulation_address(path):
    xml = read_xml_file(path, 'infrastructure.xsd')
    if xml is None:
        return

    entity = _get_component(xml.get_root(), 'entity')
    entity_type = _get_parameter(entity, 'entity_type')

    component_name = {
            'Satellite': 'entity_sat',
            'Satellite Regen': 'entity_sat_regen',
            'Terminal': 'entity_st',
            'Gateway': 'entity_gw',
            'Gateway Net Access': 'entity_gw_net_acc',
            'Gateway Phy': 'entity_gw_phy',
    }.get(entity_type, 'entity_unknown')
    entity_component = _get_component(entity, component_name)
    return _get_parameter(entity_component, 'emu_address') or None


def extract_entities_names(project):
    platform = _get_component(project.get_root(), 'platform')
    if platform is None:
        return

    entities = platform.get_list('machines')
    if entities is None:
        return

    for entity_id, _ in enumerate(entities.get_items()):
        # Can't directly use the items iterated over because of bad cast;
        # so retrieve them one by one instead to get the proper type.
        entity = entities.get_item(str(entity_id))
        name = _get_parameter(entity, 'entity_name')
        if name:
            yield name


def pidof_opensand(ssh_client):
    return set(map(int, ssh_client.run('pidof opensand', hide=True, warn=True).stdout.split()))


@app.route('/api/project/<string:name>/template/<string:xsd>/<string:filename>', methods=['GET'])
def get_project_template(name, xsd, filename):
    xsd = normalize_xsd_folder(xsd)
    return get_file_content(name + '/templates/' + xsd + '/' + filename, xml=True)


@app.route('/api/project/<string:name>/template/<string:xsd>/<string:filename>', methods=['PUT'])
def write_project_template(name, xsd, filename):
    xsd = normalize_xsd_folder(xsd)
    content = request.json['xml_data']
    return write_file_content(name + '/templates/' + xsd + '/' + filename, content)


@app.route('/api/project/<string:name>/template/<string:xsd>/<string:filename>', methods=['DELETE'])
def remove_project_template(name, xsd, filename):
    filepath = WWW_FOLDER / name / 'templates' / normalize_xsd_folder(xsd) / filename
    if not filepath.suffix == '.xml':
        filepath = filepath.with_name(filepath.name + '.xml')

    if filepath.exists():
        os.remove(filepath.as_posix())

    return success()


@app.route('/api/project/<string:name>/templates', methods=['GET'])
def list_templates(name):
    templates_folder = WWW_FOLDER / name / 'templates'
    if not templates_folder.exists():
        return jsonify({})

    templates = {
            entry.stem: [f.name for f in entry.iterdir() if f.suffix == '.xml' and f.is_file()]
            for entry in templates_folder.iterdir()
            if entry.suffixes[-2:] == ['.xsd', '.d'] and entry.is_dir()
    }
    return jsonify(templates)


@app.route('/api/project/<string:name>/profile/<string:entity>', methods=['GET'])
def get_project_profile(name, entity):
    return get_file_content(name + '/entities/' + entity + '/profile', xml=True)


@app.route('/api/project/<string:name>/profile/<string:entity>', methods=['PUT'])
def write_project_profile(name, entity):
    content = request.json['xml_data']
    return write_file_content(name + '/entities/' + entity + '/profile.xml', content)


@app.route('/api/project/<string:name>/profile/<string:entity>', methods=['DELETE'])
def remove_project_profile(name, entity):
    filepath = WWW_FOLDER / name / 'entities' / entity / 'profile.xml'
    if filepath.exists():
        os.remove(filepath.as_posix())

    return success()


@app.route('/api/project/<string:name>/infrastructure/<string:entity>', methods=['GET'])
def get_project_infrastructure(name, entity):
    return get_file_content(name + '/entities/' + entity + '/infrastructure', xml=True)


@app.route('/api/project/<string:name>/infrastructure/<string:entity>', methods=['PUT'])
def write_project_infrastructure(name, entity):
    content = request.json['xml_data']
    response = write_file_content(name + '/entities/' + entity + '/infrastructure.xml', content)
    create_platform_infrastructure(name)
    return response


@app.route('/api/project/<string:name>/infrastructure/<string:entity>', methods=['DELETE'])
def remove_project_infrastructure(name, entity):
    filepath = WWW_FOLDER / name / 'entities' / entity / 'infrastructure.xml'
    if filepath.exists():
        os.remove(filepath.as_posix())

    return success()


@app.route('/api/project/<string:name>/topology', methods=['GET'])
def get_project_topology(name):
    return get_file_content(name + '/topology', xml=True)


@app.route('/api/project/<string:name>/topology', methods=['PUT'])
def write_project_topology(name):
    content = request.json['xml_data']
    return write_file_content(name + '/topology.xml', content)


@app.route('/api/project/<string:name>/topology', methods=['DELETE'])
def remove_project_topology(name):
    filepath = WWW_FOLDER / name / 'topology.xml'
    if filepath.exists():
        os.remove(filepath.as_posix())

    return success()


@app.route('/api/project/<string:name>/entity/<string:entity>', methods=['POST'])
def download_entity(name, entity):
    files = [
            WWW_FOLDER / name / 'topology.xml',
            WWW_FOLDER / name / 'entities' / entity / 'infrastructure.xml',
            WWW_FOLDER / name / 'entities' / entity / 'profile.xml',
    ]

    in_memory = BytesIO()
    with tarfile.open(fileobj=in_memory, mode='w:gz') as tar:
        for filepath in files:
            if filepath.exists() and filepath.is_file():
                tar.add(filepath.as_posix(), filepath.name)

    in_memory.seek(0)
    dl_name = '{}.tar.gz'.format(entity)
    return send_file(in_memory, attachment_filename=dl_name, as_attachment=True)


@app.route('/api/project/<string:name>/entity/<string:entity>', methods=['PUT'])
def upload_entity(name, entity):
    files = [
            WWW_FOLDER / name / 'topology.xml',
            WWW_FOLDER / name / 'entities' / entity / 'infrastructure.xml',
            WWW_FOLDER / name / 'entities' / entity / 'profile.xml',
    ]
    files = [f for f in files if f.exists()]

    destination = Path(request.json.get('destination_folder') or '.')
    run = request.json.get('run_method') or ''
    method = request.json.get('copy_method')
    ssh_config = request.json.get('ssh', {})

    if method == 'NFS' and run in ('', 'LAUNCH'):
        destination = destination.expanduser().resolve()
        destination.mkdir(parents=True, exist_ok=True)

        for file in files:
            with file.open('rb') as source, destination.joinpath(file.name).open('wb') as dest:
                dest.write(source.read())
            destination.joinpath(file.name).chmod(0o0666)

    passwords = {
            'password': None,
            'passphrase': None,
    }
    if ssh_config.get('is_passphrase', False):
        passwords['passphrase'] = ssh_config.get('password') or None
    else:
        passwords['password'] = ssh_config.get('password') or None
    host = ssh_config.get('address') or 'localhost'

    with Connection(host, connect_kwargs=passwords) as client:
        client.client.set_missing_host_key_policy(MissingHostKeyPolicy())

        if run in ('', 'LAUNCH'):
            if method == 'SCP':
                client.run(shlex.join(['mkdir', '-p', destination.as_posix()]), hide=True)
                for file in files:
                    client.put(file.as_posix(), destination.as_posix())
            elif method == 'SFTP':
                client.run(shlex.join(['mkdir', '-p', destination.as_posix()]), hide=True)
                with client.sftp() as sftp:
                    sftp.chdir(destination.as_posix())
                    for file in files:
                        sftp.put(file.as_posix(), file.name)

        pid_file = WWW_FOLDER / name / 'entities' / entity / 'process.pid'
        try:
            with pid_file.open() as f:
                launched_pid = int(f.read())
        except OSError:
            launched_pid = None

        if run == 'LAUNCH':
            pids = pidof_opensand(client)

            if launched_pid not in pids:
                result = client.run('opensand ' + ' '.join(
                        f'-{f.name[0]} "{destination.joinpath(f.name)}"'
                        for f in files
                ) + ' </dev/null >/dev/null 2>&1 & echo $!', hide=True)

                try:
                    launched_pid = int(result.stdout)
                except ValueError:
                    return error(f'OpenSAND process could not be launched on entity {entity}', 422)

            pid_file.parent.mkdir(parents=True, exist_ok=True)
            with pid_file.open('w') as f:
                print(launched_pid, file=f)

            return success(running=True)
        elif run == 'STATUS':
            running = launched_pid in pidof_opensand(client) if launched_pid is not None else False

            if not running:
                try:
                    os.remove(pid_file)
                except OSError:
                    pass

            return success(running=running)
        elif run == 'PING':
            result = client.run(shlex.join(['ping', '-c', '6', request.json['ping_address']]), hide=True, warn=True)
            ping = result.stdout
            if result.exited:
                if ping and not ping.endswith('\n'):
                    ping += '\n'
                ping += result.stderr
            return success(ping=ping)
        elif run == 'STOP':
            if launched_pid is not None:
                client.run(f'kill {launched_pid}', hide=True, warn=True)
            try:
                os.remove(pid_file)
            except OSError:
                pass

    return success()


@app.route('/api/project/<string:name>/ping', methods=['GET'])
def get_project_ping_destinations(name):
    entities = WWW_FOLDER / name / 'entities'
    addresses = (
        extract_emulation_address(infrastructure)
        for infrastructure in entities.glob('*/infrastructure.xml')
    )
    return success(addresses=list(filter(None, addresses)))


@app.route('/api/project/<string:name>', methods=['GET'])
def get_project_content(name):
    return get_file_content(name + '/project', xml=True)


@app.route('/api/project/<string:name>', methods=['PUT'])
def update_project_content(name):
    content = request.json['xml_data']

    folder = WWW_FOLDER / name
    if not folder.exists():
        create_default_templates(name)
        backup = False
    else:
        with folder.joinpath('project.xml').open() as f:
            backup = f.read()

    result = write_file_content(name + '/project.xml', content)

    project_layout = read_xml_file(folder / 'project.xml', 'project.xsd')
    if project_layout is None:
        if not backup:
            shutil.rmtree(folder.as_posix())
        else:
            write_file_content(name + '/project.xml', backup)
        return error('Invalid project layout, update canceled', 422)

    entities_folder = folder / 'entities'
    if entities_folder.is_dir():
        entities = set(extract_entities_names(project_layout))
        for entity in entities_folder.iterdir():
            if entity.name not in entities:
                shutil.rmtree(entity.as_posix())

    return result


@app.route('/api/project/<string:name>', methods=['POST'])
def validate_project(name):
    try:
        new_project_name = request.json['name']
    except (KeyError, TypeError):
        if request.files and 'project' in request.files:
            # Do upload
            destination = WWW_FOLDER / name
            if destination.exists():
                return error('Project {} already exists'.format(name), 409)

            destination.mkdir()
            entities = destination / 'entities'
            project_archive = request.files['project']
            with tarfile.open(fileobj=project_archive.stream, mode='r:gz') as tar:
                while True:
                    info = tar.next()
                    if info is None:
                        break
                    if not info.isfile():
                        continue
                    if info.name == 'project.xml':
                        tar.extract(info, path=destination.as_posix())
                    else:
                        extracted = entities.joinpath(info.name).resolve()
                        if extracted.parent.parent == entities:
                            # Nothing fishy in the filepath, we can extract safely
                            entities.mkdir(exist_ok=True)
                            tar.extract(info, path=entities.as_posix())

            project_xml = destination / 'project.xml'
            if not project_xml.exists() or not project_xml.is_file():
                shutil.rmtree(destination.as_posix())
                return error('Provided archive does not contain a "project.xml" file', 422)

            project_layout = read_xml_file(project_xml, 'project.xsd')
            if project_layout is None:
                shutil.rmtree(destination.as_posix())
                return error('Invalid "project.xml" file in the archive', 422)

            _set_parameter(_get_component(project_layout.get_root(), 'platform'), 'project', name)
            py_opensand_conf.toXML(project_layout, project_xml.as_posix())

            if entities.exists():
                project_topology = destination.joinpath('topology.xml').as_posix()
                for entity_folder in entities.iterdir():
                    topology = entity_folder / 'topology.xml'
                    if topology.exists() and topology.is_file():
                        os.rename(topology.as_posix(), project_topology)

            create_default_templates(name)

            return success()
        else:
            # Do download
            in_memory = BytesIO()
            with tarfile.open(fileobj=in_memory, mode='w:gz') as tar:
                filepath = WWW_FOLDER / name / 'project.xml'
                if filepath.exists() and filepath.is_file():
                    tar.add(filepath.as_posix(), filepath.name)

                filepath = filepath.parent / 'entities'
                if filepath.exists() and filepath.is_dir():
                    for entity_folder in filepath.iterdir():
                        if not entity_folder.is_dir():
                            continue
                        files = [
                                WWW_FOLDER / name / 'topology.xml',
                                entity_folder / 'infrastructure.xml',
                                entity_folder / 'profile.xml',
                        ]
                        for filepath in files:
                            if filepath.exists() and filepath.is_file():
                                filename = '{}/{}'.format(entity_folder.name, filepath.name)
                                tar.add(filepath.as_posix(), filename)

            in_memory.seek(0)
            dl_name = '{}.tar.gz'.format(name)
            return send_file(in_memory, attachment_filename=dl_name, as_attachment=True)
    else:
        # Do copy
        source = WWW_FOLDER / name
        if not source.exists() or not source.is_dir():
            return error('Project {} not found'.format(name), 404)

        destination = WWW_FOLDER / new_project_name
        if destination.exists():
            return error('Project {} already exists'.format(new_project_name), 409)

        shutil.copytree(source.as_posix(), destination.as_posix())

        return success(new_project_name)

    return error('Missing branch in code', 500)


@app.route('/api/project/<string:name>', methods=['DELETE'])
def delete_project(name):
    project = WWW_FOLDER / name
    if not project.exists():
        return error('Project {} not found'.format(name), 404)

    shutil.rmtree(project.as_posix())
    return success()


@app.route('/api/project', methods=['GET'])
def get_project_model():
    return get_file_content('project.xsd')


@app.route('/api/projects', methods=['GET'])
def list_projects():
    projects = [
            entry.name
            for entry in WWW_FOLDER.iterdir()
            if entry.is_dir()
    ]
    return success(projects=projects)


@app.route('/api/model/<string:filename>', methods=['GET'])
def get_model(filename):
    return get_file_content(filename)


@app.route('/api/models/', methods=['GET'])
def list_models():
    XSDs = [
            entry.stem
            for entry in MODELS_FOLDER.iterdir()
            if entry.suffix == '.xsd' and entry.is_file()
    ]
    return success(models=XSDs)


@app.route('/', defaults={'path': ''})
@app.route('/<path:path>')
def index(path):
    return app.send_static_file('index.html')


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument(
            '-f', '--folder', '--xsd-folder',
            type=Path, default=MODELS_FOLDER,
            help='path to the folder containing XSD files')
    parser.add_argument(
            '-w', '--data', '--www-folder',
            type=Path, default=WWW_FOLDER,
            help='path to the folder containing projects files')
    parser.add_argument(
            '-a', '--address', default='0.0.0.0',
            help='host address to listen on')
    parser.add_argument(
            '-p', '--port', type=int, default=8888,
            help='host port to listen on')
    args = parser.parse_args()

    MODELS_FOLDER = args.folder.resolve()
    if not MODELS_FOLDER.is_dir():
        parser.error('XSD folder: the path is not a valid directory')

    WWW_FOLDER = args.data.resolve()
    if not WWW_FOLDER.is_dir():
        parser.error('WWW folder: the path is not a valid directory')

    app.run(host=args.address, port=args.port)
