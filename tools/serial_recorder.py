import os
import sys
import time
import datetime
import struct
import csv
import json
import serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
from matplotlib.widgets import Button

# =============================================================================
# CONSTANTES DEL PROTOCOLO BINARIO
# =============================================================================
START_BYTE           = 0xAA
MSG_IMU_BUFFER       = 0x01
MSG_PRESSURE_BUFFER  = 0x02
MSG_TEMPERATURE      = 0x03
MSG_ALARM            = 0x04
MSG_METRICS          = 0x05
MSG_KEEP_ALIVE       = 0x06
MSG_WARNING          = 0x08
MSG_PANIC_BTN_PRESS  = 0x09
MSG_BATTERY_DATA     = 0x0B
MSG_NOTIF_BTN_PRESS  = 0x0C

# Tabla de búsqueda para CRC-8 (Polinomio 0x07)
CRC8_TABLE = [
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65, 0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2, 0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42, 0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C, 0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B, 0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
    0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
]

def calculate_crc8(data: bytes, initial_crc: int = 0x00) -> int:
    crc = initial_crc
    for byte in data:
        crc = CRC8_TABLE[crc ^ byte]
    return crc

# =============================================================================
# INTERFAZ GRÁFICA / CONSOLA DE CONFIGURACIÓN INICIAL
# =============================================================================
def ask_config_gui():
    """Muestra un diálogo simple con Tkinter para elegir Modo, Puerto y Carpeta de Salida."""
    tools_dir = os.path.dirname(os.path.abspath(__file__))
    default_dir = os.path.join(tools_dir, "capturas")

    try:
        import tkinter as tk
        from tkinter import ttk, filedialog

        root = tk.Tk()
        root.title("Configuración de Grabación")
        root.resizable(False, False)

        # Centrar ventana
        window_width = 460
        window_height = 310
        screen_width = root.winfo_screenwidth()
        screen_height = root.winfo_screenheight()
        pos_x = int((screen_width - window_width) / 2)
        pos_y = int((screen_height - window_height) / 2)
        root.geometry(f"{window_width}x{window_height}+{pos_x}+{pos_y}")

        selected_mode = tk.StringVar(value="Caida")
        
        # Puertos COM detectados
        available_ports = [p.device for p in serial.tools.list_ports.comports()]
        default_port = "COM6" if "COM6" in available_ports else (available_ports[0] if available_ports else "COM6")
        selected_port = tk.StringVar(value=default_port)

        selected_dir = tk.StringVar(value=default_dir)

        result = {
            "mode": "Caida",
            "port": default_port,
            "output_dir": default_dir,
            "confirmed": False
        }

        # Frame de contenido
        frame = ttk.Frame(root, padding="15")
        frame.pack(fill=tk.BOTH, expand=True)

        # 1. Tipo de evento a grabar
        ttk.Label(frame, text="¿Qué tipo de evento vas a grabar?", font=("Arial", 10, "bold")).pack(anchor=tk.W, pady=(0, 4))
        ttk.Radiobutton(frame, text="Caídas (Nombre: 'Caida_HHmmSS.json')", variable=selected_mode, value="Caida").pack(anchor=tk.W, padx=10, pady=2)
        ttk.Radiobutton(frame, text="Eventos normales (Nombre: 'NoCaida_HHmmSS.json')", variable=selected_mode, value="NoCaida").pack(anchor=tk.W, padx=10, pady=2)

        # 2. Selección de Puerto Serie
        ttk.Label(frame, text="Puerto Serie:", font=("Arial", 10, "bold")).pack(anchor=tk.W, pady=(8, 4))
        port_combo = ttk.Combobox(frame, textvariable=selected_port, values=available_ports or ["COM6"], state="readonly" if available_ports else "normal", width=25)
        port_combo.pack(anchor=tk.W, padx=10)

        # 3. Selección de Carpeta de Destino
        ttk.Label(frame, text="Carpeta de guardado (Outputs):", font=("Arial", 10, "bold")).pack(anchor=tk.W, pady=(8, 4))
        dir_frame = ttk.Frame(frame)
        dir_frame.pack(fill=tk.X, padx=10)

        dir_entry = ttk.Entry(dir_frame, textvariable=selected_dir)
        dir_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 5))

        def browse_directory():
            folder = filedialog.askdirectory(initialdir=selected_dir.get(), title="Seleccionar Carpeta de Destino")
            if folder:
                selected_dir.set(os.path.abspath(folder))

        ttk.Button(dir_frame, text="Examinar...", command=browse_directory).pack(side=tk.RIGHT)

        def on_confirm():
            result["mode"] = selected_mode.get()
            result["port"] = selected_port.get()
            result["output_dir"] = os.path.abspath(selected_dir.get().strip() or default_dir)
            result["confirmed"] = True
            root.destroy()

        ttk.Button(frame, text="Iniciar Grabador", command=on_confirm).pack(pady=(16, 0))

        root.protocol("WM_DELETE_WINDOW", root.destroy)
        root.mainloop()

        if result["confirmed"]:
            return result["mode"], result["port"], result["output_dir"]
    except Exception as e:
        print(f"[INFO] GUI no disponible ({e}), pasando a modo consola.")

    # Fallback por consola
    return ask_config_console(default_dir)

def ask_config_console(default_dir=None):
    """Pregunta por consola la configuración inicial."""
    if default_dir is None:
        tools_dir = os.path.dirname(os.path.abspath(__file__))
        default_dir = os.path.join(tools_dir, "capturas")

    print("\n" + "=" * 60)
    print(" CONFIGURACIÓN DE GRABADOR DE TELEMETRÍA")
    print("=" * 60)
    
    ports = [p.device for p in serial.tools.list_ports.comports()]
    default_port = "COM6" if "COM6" in ports else (ports[0] if ports else "COM6")
    
    print(f"Puertos COM disponibles: {', '.join(ports) if ports else 'Ninguno detectado'}")
    user_port = input(f"Seleccione puerto [{default_port}]: ").strip()
    port = user_port if user_port else default_port

    print("\n¿Qué tipo de eventos vas a grabar?")
    print("  [1] Caídas (Prefijo: 'Caida_HHmmSS.json')")
    print("  [2] Eventos normales / No Caída (Prefijo: 'NoCaida_HHmmSS.json')")
    choice = input("Opción [1]: ").strip()
    mode = "NoCaida" if choice == "2" else "Caida"

    print(f"\nCarpeta de destino para los JSONs [{default_dir}]: ")
    user_dir = input("Ruta: ").strip()
    out_dir = os.path.abspath(user_dir) if user_dir else default_dir

    return mode, port, out_dir

# =============================================================================
# GUARDADO DE DATOS EN JSON
# =============================================================================
def save_dataset_to_json(data_batch, mode_prefix, output_root="capturas"):
    """
    Guarda todos los datos del lote en un único archivo JSON unificado.
    Nombre de archivo: {mode_prefix}_{HHmmSS}.json (ej. Caida_143025.json)
    Contiene:
      - metadatos (timestamp, tipo de evento, flags de alarma, etc.)
      - imu: matriz/vector de vectores [[x0, y0, z0], [x1, y1, z1], ...]
      - pressure: vector de muestras [p0, p1, p2, ...]
      - temperature_c: float
    """
    now = datetime.datetime.now()
    time_str = now.strftime("%H%M%S") # HHmmSS
    date_str = now.strftime("%Y-%m-%d")
    filename = f"{mode_prefix}_{time_str}.json"
    
    os.makedirs(output_root, exist_ok=True)
    filepath = os.path.join(output_root, filename)

    imu_samples = data_batch.get("imu", [])
    pressure_samples = data_batch.get("pressure", [])
    temp_val = data_batch.get("temperature", None)
    alarm_id = data_batch.get("alarm_id", None)

    event_data = {
        "event_id": f"{mode_prefix}_{time_str}",
        "event_type": mode_prefix,
        "date": date_str,
        "time": now.strftime("%H:%M:%S"),
        "timestamp_iso": now.isoformat(),
        "temperature_c": round(temp_val, 2) if temp_val is not None else None,
        "alarm_flags": alarm_id,
        "total_imu_samples": len(imu_samples),
        "total_pressure_samples": len(pressure_samples),
        "imu": [
            [x, y, z] for (x, y, z) in imu_samples
        ],
        "pressure": [
            round(p, 4) for p in pressure_samples
        ]
    }

    with open(filepath, mode="w", encoding="utf-8") as f:
        json.dump(event_data, f, indent=4)

    return filename, event_data

def load_event(json_path):
    """
    Función de utilidad para cargar fácilmente un evento guardado.
    
    Ejemplo de uso:
        import numpy as np
        import pandas as pd
        from tools.serial_recorder import load_event
        
        data = load_event("tools/capturas/Caida_143025.json")
        
        # Con NumPy:
        imu_array = np.array(data["imu"])          # Matriz (N, 3) -> [X, Y, Z]
        press_array = np.array(data["pressure"])   # Vector (M,)
        temp = data["temperature_c"]              # Float en °C
        
        # Con Pandas:
        df_imu = pd.DataFrame(data["imu"], columns=["x", "y", "z"])
        df_press = pd.Series(data["pressure"], name="pressure_hpa")
    """
    with open(json_path, "r", encoding="utf-8") as f:
        return json.load(f)

# =============================================================================
# PROGRAMA PRINCIPAL
# =============================================================================
class SerialRecorderApp:
    def __init__(self, port, baud, current_mode, output_dir):
        self.port = port
        self.baud = baud
        self.current_mode = current_mode  # "Caida" o "NoCaida"
        self.output_dir = output_dir

        self.ser = None
        self.running = True

        # Buffers temporales para almacenar la ráfaga de datos de un evento
        self.current_batch = {
            "imu": [],
            "pressure": [],
            "temperature": None,
            "alarm_id": None,
            "last_packet_time": 0
        }
        self.batch_pending = False

        # Datos acumulados para métricas y display
        self.total_saved_events = 0
        self.last_saved_name = "Ninguno"

    def set_mode(self, new_mode):
        self.current_mode = new_mode
        print(f"\n>>> [MODO CAMBIADO] Ahora grabando como: {self.current_mode} <<<")
        self.update_plot_title()

    def update_plot_title(self):
        color_tag = "🔴 [GRABANDO: CAÍDA]" if self.current_mode == "Caida" else "🟢 [GRABANDO: NO CAÍDA (NORMAL)]"
        info = f"{color_tag} | Último Guardado: {self.last_saved_name} (Total: {self.total_saved_events})"
        self.fig.suptitle(f"Grabador de Telemetría - {info}", fontsize=12, fontweight='bold')
        self.fig.canvas.draw_idle()

    def flush_and_save_current_batch(self):
        """Guarda en JSON el lote actual si contiene datos de sensores."""
        if not self.batch_pending:
            return

        has_data = len(self.current_batch["imu"]) > 0 or len(self.current_batch["pressure"]) > 0
        if has_data:
            filename, event_data = save_dataset_to_json(self.current_batch, self.current_mode, self.output_dir)
            self.total_saved_events += 1
            self.last_saved_name = filename

            print("\n" + "=" * 65)
            print(f" [OK] ¡EVENTO GUARDADO! -> {filename}")
            print(f" Archivo: {os.path.abspath(os.path.join(self.output_dir, filename))}")
            print(f"   • Muestras IMU: {event_data['total_imu_samples']}")
            print(f"   • Muestras Presión: {event_data['total_pressure_samples']}")
            if event_data['temperature_c'] is not None:
                print(f"   • Temperatura: {event_data['temperature_c']} °C")
            print("=" * 65 + "\n")

            self.update_plot_title()

        # Limpiar lote
        self.current_batch = {
            "imu": [],
            "pressure": [],
            "temperature": None,
            "alarm_id": None,
            "last_packet_time": 0
        }
        self.batch_pending = False

    def setup_plot(self):
        plt.ion()
        self.fig, (self.ax1, self.ax2) = plt.subplots(2, 1, figsize=(11, 8))
        self.fig.subplots_adjust(bottom=0.15, hspace=0.35)

        # Botones interactivos en la ventana para cambiar modo en tiempo real
        ax_btn_caida = plt.axes([0.18, 0.03, 0.28, 0.05])
        ax_btn_nocaida = plt.axes([0.54, 0.03, 0.28, 0.05])

        self.btn_caida = Button(ax_btn_caida, 'Grabar como: CAÍDA', color='#ff9999', hovercolor='#ff6666')
        self.btn_nocaida = Button(ax_btn_nocaida, 'Grabar como: NO CAÍDA', color='#99ff99', hovercolor='#66ff66')

        self.btn_caida.on_clicked(lambda event: self.set_mode("Caida"))
        self.btn_nocaida.on_clicked(lambda event: self.set_mode("NoCaida"))

        # Evento de teclado (c = Caida, n = NoCaida)
        def on_key(event):
            if event.key in ['c', 'C']:
                self.set_mode("Caida")
            elif event.key in ['n', 'N']:
                self.set_mode("NoCaida")

        self.fig.canvas.mpl_connect('key_press_event', on_key)
        self.update_plot_title()

    def _read_exact(self, n, timeout=2.5):
        """Lee exactamente n bytes del puerto serie o expira tras timeout segundos."""
        start = time.time()
        buf = bytearray()
        while len(buf) < n:
            chunk = self.ser.read(n - len(buf))
            if chunk:
                buf.extend(chunk)
            elif (time.time() - start) > timeout:
                break
        return bytes(buf)

    def run(self):
        os.makedirs(self.output_dir, exist_ok=True)

        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=0.1)
            print(f"\n[OK] Conectado a {self.port} @ {self.baud} bps")
            print(f"[OK] Carpeta de guardado: {os.path.abspath(self.output_dir)}")
            print(f"[MODO ACTIVO] {self.current_mode} (Presiona 'C' para Caída o 'N' para NoCaída en el gráfico)\n")
        except Exception as e:
            print(f"[ERROR] No se pudo abrir el puerto serie {self.port}: {e}")
            return

        self.setup_plot()

        imu_x, imu_y, imu_z = [], [], []
        press_data = []

        try:
            while self.running:
                # Comprobar si hay un lote pendiente y ya pasó más de 500ms sin nuevos paquetes de sensores
                if self.batch_pending and (time.time() - self.current_batch["last_packet_time"] > 0.5):
                    self.flush_and_save_current_batch()

                # Atender eventos de la ventana gráfica
                self.fig.canvas.flush_events()

                # Lectura serie byte a byte buscando START_BYTE
                b = self.ser.read(1)
                if not b:
                    continue

                if b[0] == START_BYTE:
                    # Leer cabecera de 3 bytes: [TYPE][LEN_L][LEN_H]
                    header = self._read_exact(3, timeout=0.5)
                    if len(header) < 3:
                        print("[AVISO] Cabecera incompleta tras START_BYTE.")
                        continue

                    header_crc_raw = self._read_exact(1, timeout=0.2)
                    if not header_crc_raw:
                        print("[AVISO] Falta CRC de cabecera.")
                        continue

                    header_crc_recv = header_crc_raw[0]
                    calc_h_crc = calculate_crc8(b)
                    calc_h_crc = calculate_crc8(header, calc_h_crc)

                    if calc_h_crc != header_crc_recv:
                        print(f"[AVISO] Header corrupto (CRC erróneo: recibido {header_crc_recv:02X}, calc {calc_h_crc:02X}).")
                        continue

                    msg_type = header[0]
                    msg_len = struct.unpack('<H', header[1:3])[0]

                    payload = b''
                    if msg_len > 0:
                        # Para payloads grandes (ej. 4800 bytes), esperamos hasta 2.5s
                        payload = self._read_exact(msg_len, timeout=2.5)
                        if len(payload) < msg_len:
                            print(f"[AVISO] Payload incompleto: esperado {msg_len} bytes, recibido {len(payload)}.")
                            continue

                        p_crc_raw = self._read_exact(1, timeout=0.5)
                        if not p_crc_raw:
                            print("[AVISO] Falta CRC de payload.")
                            continue
                        
                        calc_p_crc = calculate_crc8(payload)
                        if calc_p_crc != p_crc_raw[0]:
                            print(f"[AVISO] Payload corrupto (CRC erróneo: recibido {p_crc_raw[0]:02X}, calc {calc_p_crc:02X}).")
                            continue

                    # ---------------------------------------------------------
                    # PROCESAMIENTO DE TRAMAS
                    # ---------------------------------------------------------
                    now_t = time.time()

                    if msg_type == MSG_IMU_BUFFER:
                        num_samples = msg_len // 6
                        print(f"[IMU] Trama de buffer recibida: {num_samples} muestras ({msg_len} bytes)")
                        
                        imu_x.clear()
                        imu_y.clear()
                        imu_z.clear()
                        imu_tuples = []

                        for i in range(num_samples):
                            offset = i * 6
                            x, y, z = struct.unpack('<hhh', payload[offset:offset+6])
                            imu_x.append(x)
                            imu_y.append(y)
                            imu_z.append(z)
                            imu_tuples.append((x, y, z))

                        self.current_batch["imu"] = imu_tuples
                        self.current_batch["last_packet_time"] = now_t
                        self.batch_pending = True

                        # Actualizar gráfico de IMU
                        self.ax1.clear()
                        self.ax1.plot(imu_x, label='X (Lateral)', color='red', alpha=0.8, linewidth=1.2)
                        self.ax1.plot(imu_y, label='Y (Longitudinal)', color='green', alpha=0.8, linewidth=1.2)
                        self.ax1.plot(imu_z, label='Z (Vertical)', color='blue', alpha=0.8, linewidth=1.2)
                        self.ax1.set_title(f"Acelerómetro (IMU) - {num_samples} muestras", fontsize=10, fontweight='bold')
                        self.ax1.set_ylabel("Cuentas LSB")
                        self.ax1.legend(loc='upper right')
                        self.ax1.grid(True, linestyle='--', alpha=0.6)
                        self.fig.canvas.draw_idle()

                    elif msg_type == MSG_PRESSURE_BUFFER:
                        num_samples = msg_len // 4
                        print(f"[PRESIÓN] Trama de barómetro recibida: {num_samples} muestras ({msg_len} bytes)")

                        press_data.clear()
                        for i in range(num_samples):
                            offset = i * 4
                            p = struct.unpack('<f', payload[offset:offset+4])[0]
                            press_data.append(p)

                        self.current_batch["pressure"] = press_data
                        self.current_batch["last_packet_time"] = now_t
                        self.batch_pending = True

                        # Actualizar gráfico de Presión
                        self.ax2.clear()
                        self.ax2.plot(press_data, label='Presión Barométrica (hPa)', color='darkorange', linewidth=1.5)
                        self.ax2.set_title(f"Barómetro - {num_samples} muestras", fontsize=10, fontweight='bold')
                        self.ax2.set_ylabel("Presión (hPa)")
                        self.ax2.legend(loc='upper right')
                        self.ax2.grid(True, linestyle='--', alpha=0.6)
                        self.fig.canvas.draw_idle()

                    elif msg_type == MSG_TEMPERATURE:
                        if msg_len >= 4:
                            temp_val = struct.unpack('<f', payload[0:4])[0]
                            print(f"[TEMP] Temperatura: {temp_val:.2f} °C")
                            self.current_batch["temperature"] = temp_val
                            self.current_batch["last_packet_time"] = now_t
                            self.batch_pending = True

                    elif msg_type == MSG_ALARM:
                        if msg_len >= 1:
                            alarm_id = payload[0]
                            print(f"\n[ALERTA CAÍDA] ¡Alarma detectada! Flags: 0x{alarm_id:02X}")
                            self.current_batch["alarm_id"] = alarm_id
                            # La alarma concluye la secuencia de envío de caída: guardamos inmediatamente
                            self.flush_and_save_current_batch()

                    elif msg_type == MSG_KEEP_ALIVE:
                        print("[SYS] Keep-Alive recibido.")

                    elif msg_type == MSG_METRICS:
                        if msg_len >= 2:
                            steps = struct.unpack('<H', payload[0:2])[0]
                            print(f"[METRICAS] Pasos detectados: +{steps}")

                    elif msg_type == MSG_BATTERY_DATA:
                        if msg_len >= 2:
                            charge = payload[0]
                            charging = "Sí" if payload[1] != 0 else "No"
                            print(f"[BATERÍA] Carga: {charge}% | Cargando: {charging}")

                    elif msg_type == MSG_PANIC_BTN_PRESS:
                        print("\n[BOTÓN] ¡Botón de Pánico Presionado!")

                    elif msg_type == MSG_NOTIF_BTN_PRESS:
                        print("\n[BOTÓN] ¡Botón de Notificación Presionado!")

                    elif msg_type == MSG_WARNING:
                        if msg_len >= 1:
                            print(f"[ADVERTENCIA] Código: {payload[0]}")

                else:
                    # Carácter de debug ASCII recibido
                    try:
                        char = bytes([b[0]]).decode('utf-8')
                        print(char, end='', flush=True)
                    except:
                        pass

        except KeyboardInterrupt:
            print("\n[DETENIENDO] Cerrando conexión y saliendo...")
        finally:
            if self.batch_pending:
                self.flush_and_save_current_batch()
            if self.ser and self.ser.is_open:
                self.ser.close()
            plt.close('all')
            print("[CERRADO] Script finalizado correctamente.")

def main():
    # 1. Configuración inicial (Modo, Puerto y Carpeta de Salida)
    selected_mode, selected_port, selected_output_dir = ask_config_gui()

    # 2. Lanzar grabador
    app = SerialRecorderApp(
        port=selected_port,
        baud=115200,
        current_mode=selected_mode,
        output_dir=selected_output_dir
    )
    app.run()

if __name__ == '__main__':
    main()
