# /home/pi/rc522_read.py
import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522
import time


# 태그가 계속 되는 상태여도 ID와 notag가 번갈아 나오므로 주의 필요
def read_rfid():
    reader = SimpleMFRC522()
    try:
        id = reader.read_id()  # Non-blocking read
        if id:
            print(id, flush=True)
    finally:
        GPIO.cleanup()


if __name__ == "__main__":
    read_rfid()
