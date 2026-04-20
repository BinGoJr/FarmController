import serial
import time
import os
from datetime import datetime

# ===== НАСТРОЙКИ =====
PORT = 'COM4'               # замените на ваш порт
BAUDRATE = 57600
SAVE_FOLDER = r"Путь_к_папке"
# =====================

# Создаём папку, если её нет
os.makedirs(SAVE_FOLDER, exist_ok=True)

# Имена файлов с текущей датой и временем
data_file_name = 'datalog.txt'
system_file_name = 'system.txt'

data_file =os.path.join(SAVE_FOLDER, data_file_name)
system_file =os.path.join(SAVE_FOLDER, system_file_name)

def main():
    try:
        ser = serial.Serial(PORT, BAUDRATE, timeout=1)
        time.sleep(2)  # ждём инициализацию Arduino
        print(f"Подключено к {PORT}")
        print(f"Папка сохранения: {SAVE_FOLDER}")
        print(f"Данные датчиков -> {data_file}")
        print(f"Системный лог    -> {system_file}")
        print("Для остановки нажмите Ctrl+C\n")

        with open(data_file, 'a', encoding='utf-8') as csv_file, \
             open(system_file, 'a', encoding='utf-8') as log_file:

            while True:
                if ser.in_waiting:
                    raw_line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if not raw_line:
                        continue

                    if raw_line.startswith("DATA:"):
                        # Убираем префикс "DATA:" и пишем в CSV
                        data_line = raw_line[5:]
                        csv_file.write(data_line + '\n')
                        csv_file.flush()
                        print(f"[DATA] {data_line}")
                    else:
                        # Служебное сообщение – добавляем временную метку
                        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                        log_entry = f"[{timestamp}] {raw_line}"
                        log_file.write(log_entry + '\n')
                        log_file.flush()
                        print(f"[LOG] {raw_line}")

    except serial.SerialException as e:
        print(f"Ошибка порта {PORT}: {e}")
        print("Проверьте, что порт правильный и Arduino подключена.")
    except KeyboardInterrupt:
        print("\nОстановка скрипта пользователем.")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Порт закрыт.")

if __name__ == "__main__":
    main()