import os
import tempfile
from pathlib import Path

from flask import Flask, request, jsonify
from flask_cors import CORS

import ConfigurationPy


app = Flask(__name__)
CORS(app)


MODELS_FOLDER = Path()


@app.route('/model/<string:filename>', methods=['POST'])
def validate_model(filename):
    with tempfile.NamedTemporaryFile(delete=False) as f:
        f.write(request.json['xml_data'])
        f.flush()

    filepath = MODELS_FOLDER.joinpath(filename).as_posix()
    xsd = ConfigurationPy.fromXSD(filepath)
    xml = ConfigurationPy.fromXML(xsd, f.name)

    os.remove(f.name)

    if not xml:
        return jsonify({'error': f'Invalid XML data against {filepath}'}), 400
    return jsonify({'status': 'OK'})


@app.route('/model/<string:filename>', methods=['GET'])
def get_model(filename):
    if not filename.endswith('.xsd'):
        filename += '.xsd'

    with MODELS_FOLDER.joinpath(filename).open() as f:
        xsd_content = f.read()

    return jsonify({'content': xsd_content})


@app.route('/models/', methods=['GET'])
def list_models():
    XSDs = [
            entry.stem
            for entry in MODELS_FOLDER.iterdir()
            if entry.suffix == '.xsd' and entry.is_file()
    ]
    return jsonify(XSDs)


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
    app.run(host=args.address, port=args.port)
