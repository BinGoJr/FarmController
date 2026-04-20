from flask import Flask, request
import os
from datetime import datetime

app = Flask(__name__)
app.config['MAX_CONTENT_LENGTH'] = 2 * 1024 * 1024
main_port = номер_вашего_порта
SAVE_FOLDER = r"Путь_к_папке"
os.makedirs(SAVE_FOLDER, exist_ok=True)

ALLOWED_IPS = {'IP_cam1', 'IP_cam2'}

@app.before_request
def limit_remote_addr():
    if request.remote_addr not in ALLOWED_IPS:
        return 'Forbidden', 403

@app.route('/upload', methods=['POST'])
def upload():
    cam_name = request.headers.get('X-Camera-Name', 'unknown')
    image_data = request.data
    if not image_data:
        return 'No data', 400
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    filename = f"{cam_name}_{timestamp}.jpg"
    path = os.path.join(SAVE_FOLDER, filename)
    with open(path, 'wb') as f:
        f.write(image_data)
    print(f"Saved: {path} ({len(image_data)} bytes)")
    return 'OK', 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=main_port, debug=False)