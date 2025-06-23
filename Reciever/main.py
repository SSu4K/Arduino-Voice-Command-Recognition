# totally vibecoded just to test if the encryption works

import serial
from Crypto.Cipher import AES
import time

key = bytes([
    0x60, 0x3d, 0xeb, 0x10,
    0x15, 0xca, 0x71, 0xbe,
    0x2b, 0x73, 0xae, 0xf0,
    0x85, 0x7d, 0x77, 0x81
])

IV_SIZE = 16
HEADER = b'\xAA\x55'

ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.1)

def find_header(ser):
    """Read bytes until HEADER is found."""
    buffer = b""
    while True:
        byte = ser.read(1)
        if not byte:
            continue
        buffer += byte
        if len(buffer) > len(HEADER):
            buffer = buffer[-len(HEADER):]  # keep only last bytes
        if buffer == HEADER:
            return

def read_n_bytes(ser, n):
    """Read exactly n bytes from serial, waiting as necessary."""
    data = b""
    while len(data) < n:
        chunk = ser.read(n - len(data))
        if not chunk:
            continue
        data += chunk
    return data

def calculate_checksum(data: bytes) -> int:
    checksum = 0
    for b in data:
        checksum ^= b
    return checksum

while True:
    # Find header
    find_header(ser)

    # Read length
    length_bytes = read_n_bytes(ser, 1)
    msg_len = length_bytes[0]

    # Read IV + ciphertext + checksum
    iv = read_n_bytes(ser, IV_SIZE)
    ciphertext = read_n_bytes(ser, msg_len)
    received_checksum = read_n_bytes(ser, 1)[0]

    # Compute checksum over length + iv + ciphertext
    checksum_data = length_bytes + iv + ciphertext
    calc_checksum = calculate_checksum(checksum_data)

    if calc_checksum != received_checksum:
        print(f"Checksum mismatch! Expected {received_checksum:02X}, got {calc_checksum:02X}. Discarding message.")
        continue  # skip this message

    # Decrypt
    initial_value = int.from_bytes(iv, byteorder='big')
    cipher = AES.new(key, AES.MODE_CTR, nonce=b'', initial_value=initial_value)
    plaintext = cipher.decrypt(ciphertext)

    try:
        text = plaintext.decode('utf-8')
        print("Decrypted JSON:", text)
    except UnicodeDecodeError:
        print("Failed to decode UTF-8:", plaintext)