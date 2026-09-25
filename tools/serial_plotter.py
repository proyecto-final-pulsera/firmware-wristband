import serial
import struct
import matplotlib.pyplot as plt

# Identificadores de nuestro protocolo
START_BYTE = 0xAA
MSG_IMU_BUFFER = 0x01
MSG_PRESSURE_BUFFER = 0x02
MSG_TEMPERATURE = 0x03
MSG_ALARM = 0x04
MSG_METRICS = 0x05
MSG_KEEP_ALIVE = 0x06

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

def main():
    port = "COM6"
    baud = 115200

    # Inicializar Puerto Serie
    try:
        ser = serial.Serial(port, baud, timeout=1)
        print(f"[OK] Escuchando en {port} a {baud} bps...")
    except Exception as e:
        print(f"[ERROR] No se pudo abrir el puerto: {e}")
        return

    # Preparar el modo interactivo de Matplotlib
    plt.ion()
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
    fig.suptitle('Visor de Telemetría - Snapshot de Caída')
    
    # Para almacenar los datos temporalmente
    imu_x, imu_y, imu_z = [], [], []
    press_data = []
    total_steps = 0  # Contador acumulativo de pasos

    try:
        while True:
            # 1. Esperamos pacientemente el byte de sincronismo
            b = ser.read(1)
            if not b:
                continue # Timeout, volvemos a intentar
                
            if b[0] == START_BYTE:
                # 2. ¡Atrapamos un posible inicio de trama! Leemos el Header [TYPE][LEN_L][LEN_H]
                header = ser.read(3) 
                if len(header) < 3:
                    continue
                
                # Leemos el CRC del Header
                header_crc_raw = ser.read(1)
                if not header_crc_raw:
                    continue
                header_crc_received = header_crc_raw[0]

                # Verificamos CRC del Header
                calc_h_crc = calculate_crc8(b)               # START_BYTE
                calc_h_crc = calculate_crc8(header, calc_h_crc) # TYPE + LEN
                
                if calc_h_crc != header_crc_received:
                    print(f"[ERROR] Header alterado. CRC recibido: {header_crc_received:02X}, calculado: {calc_h_crc:02X}. Ignorando.")
                    # Vaciamos el buffer del puerto serie. 
                    # Como no sabemos el 'len' real (el header está corrupto), no podemos 
                    # saltear el payload de forma exacta. Vaciar el buffer evita que leamos 
                    # el payload residual como si fuera basura, previniendo que Python se trabe.
                    ser.reset_input_buffer()
                    continue
                
                msg_type = header[0]
                
                # Desempaquetamos el largo (Unsigned Short, Little Endian)
                msg_len = struct.unpack('<H', header[1:3])[0]
                
                # 3. Leemos el Payload completo (si lo hay)
                payload = b''
                if msg_len > 0:
                    payload = ser.read(msg_len)
                    if len(payload) < msg_len:
                        print("[WARNING] Trama incompleta o corrompida perdida.")
                        ser.reset_input_buffer()
                        continue
                    
                    # 4. Leemos el CRC del Payload
                    p_crc_raw = ser.read(1)
                    if not p_crc_raw:
                        print("[WARNING] Trama sin Payload CRC.")
                        ser.reset_input_buffer()
                        continue
                    p_crc_received = p_crc_raw[0]
                    
                    # 5. Verificamos el CRC del Payload
                    calc_p_crc = calculate_crc8(payload)
                    if calc_p_crc != p_crc_received:
                        print(f"[ERROR] Payload alterado. CRC recibido: {p_crc_received:02X}, calculado: {calc_p_crc:02X}. Ignorando trama.")
                        ser.reset_input_buffer()
                        continue
                
                # 6. Parseamos dependiendo del Tipo de Mensaje
                if msg_type == MSG_IMU_BUFFER:
                    print(f"\n[OK] Trama IMU recibida. Tamaño: {msg_len} bytes")
                    
                    # Un DataXYZ tiene 3 int16_t (2 bytes c/u) = 6 bytes en total por muestra
                    num_samples = msg_len // 6
                    imu_x.clear()
                    imu_y.clear()
                    imu_z.clear()
                    
                    for i in range(num_samples):
                        offset = i * 6
                        # Desempaquetar 3 enteros cortos con signo (Little Endian)
                        x, y, z = struct.unpack('<hhh', payload[offset:offset+6])
                        imu_x.append(x)
                        imu_y.append(y)
                        imu_z.append(z)
                        
                    # Refrescar gráfico
                    ax1.clear()
                    ax1.plot(imu_x, label='X', color='red', alpha=0.7)
                    ax1.plot(imu_y, label='Y', color='green', alpha=0.7)
                    ax1.plot(imu_z, label='Z', color='blue', alpha=0.7)
                    ax1.set_title("Acelerómetro (IMU)")
                    ax1.legend(loc='upper right')
                    ax1.grid(True)
                    
                    fig.canvas.draw()
                    fig.canvas.flush_events()
                    
                elif msg_type == MSG_PRESSURE_BUFFER:
                    print(f"[OK] Trama PRESIÓN recibida. Tamaño: {msg_len} bytes")
                    
                    # Un float tiene 4 bytes
                    num_samples = msg_len // 4
                    press_data.clear()
                    
                    for i in range(num_samples):
                        offset = i * 4
                        # Desempaquetar 1 float (Little Endian)
                        p = struct.unpack('<f', payload[offset:offset+4])[0]
                        press_data.append(p)
                        
                    # Refrescar gráfico
                    ax2.clear()
                    ax2.plot(press_data, color='orange', label='Presión (hPa)')
                    ax2.set_title("Barómetro")
                    ax2.legend(loc='upper right')
                    ax2.grid(True)
                    
                    fig.canvas.draw()
                    fig.canvas.flush_events()
                    
                elif msg_type == MSG_TEMPERATURE:
                    if msg_len >= 4:
                        temp_val = struct.unpack('<f', payload[0:4])[0]
                        print(f"\n[TELEMETRÍA] Temperatura: {temp_val:.2f} °C")
                        
                elif msg_type == MSG_METRICS:
                    if msg_len >= 4:
                        new_steps = struct.unpack('<I', payload[0:4])[0]
                        total_steps += new_steps
                        print(f"\n[TELEMETRÍA] Métricas (Pasos): +{new_steps} | Total Acumulado: {total_steps}")
                        
                elif msg_type == MSG_ALARM:
                    print(f"\n================================================")
                    print(f" [ALERTA CRÍTICA] ¡EVENTO DE CAÍDA DETECTADO!")
                    print(f"================================================")
                    
                elif msg_type == MSG_KEEP_ALIVE:
                    # Imprimimos de manera discreta para no ensuciar mucho la consola
                    print(f"[SYS] Keep Alive recibido. (Dispositivo vivo)")
                    
            else:
                # Imprimir el Debug ASCII.
                # Como usamos print, evitamos colisionar con excepciones de decodificación.
                try:
                    char = bytes([b[0]]).decode('utf-8')
                    # Imprime el texto de debug en consola en la misma línea
                    print(char, end='', flush=True)
                except:
                    pass

    except KeyboardInterrupt:
        print("\n[CERRANDO] Saliendo del script...")
    finally:
        ser.close()
        plt.close()

if __name__ == '__main__':
    main()
