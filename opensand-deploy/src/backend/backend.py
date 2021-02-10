import os
import shutil
import tempfile
from pathlib import Path

from flask import Flask, request, jsonify
from flask_cors import CORS

import py_opensand_conf


app = Flask(__name__)
CORS(app)


MODELS_FOLDER = Path()


def success(message='OK'):
    return jsonify({'status': message})


def error(message):
    return jsonify({'error': message})


def get_file_content(filename, expected_suffix='.xsd'):
    if not filename.endswith(expected_suffix):
        filename += expected_suffix

    filepath = MODELS_FOLDER / filename
    if not filepath.exists():
        folder = filepath.relative_to(MODELS_FOLDER).parent
        return error('cannot find {} in {}'.format(filepath.name, folder)), 404

    with filepath.open() as f:
        content = f.read()

    return jsonify({'content': content})


def write_file_content(filename, content, expected_suffix='.xml'):
    if not filename.endswith(expected_suffix):
        filename += expected_suffix

    filepath = MODELS_FOLDER.joinpath(filename)
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


def validate_model(xsd, xml, raw_xml_content=False):
    if raw_xml_content:
        with tempfile.NamedTemporaryFile(delete=False) as f:
            f.write(xml)
            f.flush()
        xml = f.name

    filepath = MODELS_FOLDER.joinpath(xsd).as_posix()
    xsd = py_opensand_conf.fromXSD(filepath)
    xml = py_opensand_conf.fromXML(xsd, xml)

    os.remove(f.name)

    if not xml:
        return error('Invalid XML data against {}'.format(filepath)), 400
    return success()


@app.route('/api/project/<string:name>/template/<string:xsd>/<string:filename>', methods=['GET'])
def get_project_template(name, xsd, filename):
    xsd = normalize_xsd_folder(xsd)
    return get_file_content(name + '/templates/' + xsd + '/' + filename, '.xml')


@app.route('/api/project/<string:name>/template/<string:xsd>/<string:filename>', methods=['PUT'])
def write_project_template(name, xsd, filename):
    xsd = normalize_xsd_folder(xsd)
    content = request.json['xml_data']
    return write_file_content(name + '/templates/' + xsd + '/' + filename, content)


@app.route('/api/project/<string:name>/template/<string:xsd>/<string:filename>', methods=['DELETE'])
def remove_project_template(name, xsd, filename):
    filepath = MODELS_FOLDER / name / 'templates' / normalize_xsd_folder(xsd) / filename
    if not filepath.suffix == '.xml':
        filepath = filepath.with_name(filepath.name + '.xml')

    if filepath.exists():
        os.remove(filepath.as_posix())

    return success()


@app.route('/api/project/<string:name>/templates', methods=['GET'])
def list_templates(name):
    templates_folder = MODELS_FOLDER / name / 'templates'
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
    return get_file_content(name + '/entities/' + entity + '/profile', '.xml')


@app.route('/api/project/<string:name>/profile/<string:entity>', methods=['PUT'])
def write_project_profile(name, entity):
    content = request.json['xml_data']
    return write_file_content(name + '/entities/' + entity + '/profile.xml', content)


@app.route('/api/project/<string:name>/profile/<string:entity>', methods=['DELETE'])
def remove_project_profile(name, entity):
    filepath = MODELS_FOLDER / name / 'entities' / entity / 'profile.xml'
    if filepath.exists():
        os.remove(filepath.as_posix())

    return success()


@app.route('/api/project/<string:name>/infrastructure/<string:entity>', methods=['GET'])
def get_project_infrastructure(name, entity):
    return get_file_content(name + '/entities/' + entity + '/infrastructure', '.xml')


@app.route('/api/project/<string:name>/infrastructure/<string:entity>', methods=['PUT'])
def write_project_infrastructure(name, entity):
    content = request.json['xml_data']
    return write_file_content(name + '/entities/' + entity + '/infrastructure.xml', content)


@app.route('/api/project/<string:name>/infrastructure/<string:entity>', methods=['DELETE'])
def remove_project_infrastructure(name, entity):
    filepath = MODELS_FOLDER / name / 'entities' / entity / 'infrastructure.xml'
    if filepath.exists():
        os.remove(filepath.as_posix())

    return success()


@app.route('/api/project/<string:name>/topology', methods=['GET'])
def get_project_topology(name):
    return get_file_content(name + '/topology', '.xml')


@app.route('/api/project/<string:name>/topology', methods=['PUT'])
def write_project_topology(name):
    content = request.json['xml_data']
    return write_file_content(name + '/topology.xml', content)


@app.route('/api/project/<string:name>/topology', methods=['DELETE'])
def remove_project_topology(name):
    filepath = MODELS_FOLDER / name / 'topology.xml'
    if filepath.exists():
        os.remove(filepath.as_posix())

    return success()


@app.route('/api/project/<string:name>', methods=['GET'])
def get_project_content(name):
    return get_file_content(name + '/project', '.xml')


@app.route('/api/project/<string:name>', methods=['PUT'])
def update_project_content(name):
    folder = MODELS_FOLDER / name
    folder.mkdir(exist_ok=True)

    topology = folder / 'topology.xml'
    if not topology.exists():
        topology_model = MODELS_FOLDER.joinpath('topology.xsd').as_posix()
        xsd = py_opensand_conf.fromXSD(topology_model)
        xml = xsd.create_data()
        py_opensand_conf.toXML(xml, topology.as_posix())

    content = request.json['xml_data']
    return write_file_content(name + '/project.xml', content)


@app.route('/api/project/<string:name>', methods=['POST'])
def validate_project(name):
    # TODO. Only validate ? Download ? Start ?
    return success()


@app.route('/api/project/<string:name>', methods=['DELETE'])
def delete_project(name):
    shutil.rmtree(MODELS_FOLDER.joinpath(name), ignore_errors=True)
    return success()


@app.route('/api/project', methods=['GET'])
def get_project_model():
    return get_file_content('project.xsd')


@app.route('/api/projects', methods=['GET'])
def list_projects():
    projects = [
            entry.name
            for entry in MODELS_FOLDER.iterdir()
            if entry.is_dir()
    ]
    return jsonify(projects)


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
    return jsonify(XSDs)


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
            '-a', '--address', default='0.0.0.0',
            help='host address to listen on')
    parser.add_argument(
            '-p', '--port', type=int, default=8888,
            help='host port to listen on')
    args = parser.parse_args()

    MODELS_FOLDER = args.folder
    if not MODELS_FOLDER.is_dir():
        parser.error('XSD folder: the path is not a valid directory')

    app.run(host=args.address, port=args.port)
