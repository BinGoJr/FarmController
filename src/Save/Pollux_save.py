import requests
import time
import os
from datetime import datetime

CAMERA_IP = "10.0.0.1"
SAVE_FOLDER = r"Путь_к_папке"
SHOOT_TIMEOUT = 45          # сколько секунд пытаться сделать снимки
os.makedirs(SAVE_FOLDER, exist_ok=True)
LOG_FILE = os.path.join(SAVE_FOLDER, "camera_log.txt")

def log(msg):
    ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(f"[{ts}] {msg}\n")
    print(f"[{ts}] {msg}")

def online():
    try:
        return requests.get(f"http://{CAMERA_IP}/", timeout=2).status_code == 200
    except:
        return False

def shoot():
    url = f"http://{CAMERA_IP}/api/options/testSensors"
    payload = {"CameraAutomode":1,"CameraExposureTime":250,"CameraGain":0,"CameraMirrorX":0,"CameraMirrorY":1}
    try:
        r = requests.post(url, json=payload, timeout=10)
        return r.status_code == 200
    except:
        return False

def download():
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    ok = False
    for i in range(5):
        try:
            r = requests.get(f"http://{CAMERA_IP}/images/img{i}.jpg", timeout=10)
            if r.status_code == 200:
                path = os.path.join(SAVE_FOLDER, f"pollux_{ts}_img{i}.jpg")
                with open(path, "wb") as f:
                    f.write(r.content)
                log(f"Сохранено img{i}")
                ok = True
        except:
            pass
    return ok


while True:
    log("Ожидание включения камеры...")
    while not online():
        time.sleep(2)
    log("Камера обнаружена")
    time.sleep(2)

    start = time.time()
    success = False
    while time.time() - start < SHOOT_TIMEOUT:
        if shoot():
            time.sleep(3)
            if download():
                success = True
                break
        time.sleep(3)

    if success:
        log("Снимки сделаны")

    else:
        log("Не удалось сделать снимки")

    log("Ожидание отключения камеры...")
    while online():
        time.sleep(2)
    log("----------------Камера отключена----------------")
    time.sleep(180)