import time
import serial
import serial.tools.list_ports
import sys

def find_esp32_port():
    """Attempt to auto-detect the ESP32 COM port."""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        # Common USB-to-UART bridge descriptors for ESP32 and similar boards
        desc = port.description.upper()
        if "CP210" in desc or "CH340" in desc or "USB TO UART" in desc:
            return port.device

    # Fallback to the first available port if exact match not found
    if ports:
        print(f"Warning: Could not definitively identify ESP32 port. Using first available: {ports[0].device}")
        return ports[0].device

    return None

def main():
    print("Searching for ESP32 serial port...")
    port_name = find_esp32_port()

    if not port_name:
        print("Error: No serial ports found. Is the ESP32 connected?")
        sys.exit(1)

    print(f"Using port: {port_name}")

    # Calculate exact current UTC UNIX timestamp
    unix_time = int(time.time())
    command = f"TIME {unix_time}\n"

    try:
        ser = serial.Serial()
        ser.port = port_name
        ser.baudrate = 115200
        ser.timeout = 1

        # Explicitly disable hardware flow control / reset signals BEFORE opening
        ser.dtr = False
        ser.rts = False

        ser.open()

        try:
            print("Waiting 1.0 second for connection to stabilize without resetting...")
            time.sleep(1.0)

            # Clear out the ESP32's buffer
            ser.reset_input_buffer()

            print(f"Sending command: {command.strip()}")
            ser.write(command.encode('utf-8'))
            ser.flush()

            # Wait to ensure delivery and processing
            time.sleep(0.5)

            print("Reading response:")
            while ser.in_waiting > 0:
                response = ser.readline().decode('utf-8', errors='ignore').strip()
                if response:
                    print(f"ESP32: {response}")

            print("Time synchronization sequence completed!")
        finally:
            ser.close()

    except serial.SerialException as e:
        print(f"Error opening serial port {port_name}: {e}")
        print("Make sure no other program (like PlatformIO Serial Monitor) is using the port.")
        sys.exit(1)

if __name__ == "__main__":
    main()
