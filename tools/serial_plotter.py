import serial
import struct
import matplotlib.pyplot as plt

# Identificadores de nuestro protocolo
START_BYTE = 0xAA
MSG_IMU_BUFFER = 0x01
MSG_PRESSURE_BUFFER = 0x02

def main():
    port = "COM5"
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
                
                msg_type = header[0]
                
                # Desempaquetamos el largo (Unsigned Short, Little Endian)
                msg_len = struct.unpack('<H', header[1:3])[0]
                
                # 3. Leemos el Payload completo
                payload = ser.read(msg_len)
                if len(payload) < msg_len:
                    print("[WARNING] Trama incompleta o corrompida perdida.")
                    continue
                
                # 4. Parseamos dependiendo del Tipo de Mensaje
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
