import RPi.GPIO as GPIO
import sys
from mfrc522 import SimpleMFRC522


def read_rfid():
    reader = SimpleMFRC522()
    try:
        id, _ = reader.read()
        return id
    except Exception as e:
        print(f"오류 발생: {e}")
        return None
    finally:
        GPIO.cleanup()


if __name__ == "__main__":
    id = read_rfid()
    if id is not None:
        print(id)
    else:
        print("None")
