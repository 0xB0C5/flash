import sys
import random
import time

from serial.tools import list_ports
import serial


def echo_test(ser):
    for i in range(10):
        random_bytes = bytes(random.randint(0, 255) for i in range(4))
        payload = b'E' + random_bytes
        ser.write(payload)
        response = ser.read(5)
        if response == payload:
            return True

        print('Echo test failed: sent', payload, 'but received', response)

    return False

def main():
    if len(sys.argv) != 3:
        print('Usage: python flash.py <port> <file>')
        print('Available ports:')
        for port in list_ports.comports():
            print('   ', port.device)

        sys.exit(0)

    data = open(sys.argv[2], 'rb').read()

    data_pages = [
        data[i:i+256]
        for i in range(0, len(data), 256)
    ]

    with serial.Serial(sys.argv[1], timeout=1) as ser:
        if not echo_test(ser):
            print('Connection failed.')
            return

        print('Connected. Checking data validity.')

        is_valid = True
        needs_clear = False
        for page_index, data_page in enumerate(data_pages):
            ser.write(b'R' + bytes([page_index]))
            while ser.read(1) != b'R':
                pass

            flash_page = b''
            while len(flash_page) < 256:
                flash_page += ser.read(256 - len(flash_page))

            if flash_page != data_page:
                is_valid = False

                if any(b != 0xff for b in flash_page):
                    print('Page', page_index, 'contains incorrect data.')
                    print(flash_page)
                    needs_clear = True
                else:
                    print('Page', page_index, 'is clear.')

        if is_valid:
            print('Flash data is valid.')
            return

        if needs_clear:
            print('Flash data invalid. Clearing.')
            ser.write(b'C')
            while ser.read(1) != b'C':
                pass

            print('Flash data cleared.')
        else:
            print('Flash data is clear.')

        print('Writing.')
        for page_index, data_page in enumerate(data_pages):
            print('Writing page', (page_index+1), 'of', len(data_pages))
            ser.write(b'W' + bytes([page_index]) + data_page)

            while ser.read(1) != b'W':
                pass

        print('Write complete.')

if __name__ == '__main__':
    main()
