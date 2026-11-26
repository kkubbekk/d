import cv2
from ultralytics import YOLO
import asyncio
import struct
from bleak import BleakScanner, BleakClient

# Load YOLOv8 model
model = YOLO("yolov8n.pt")

ESP_BLUETOOTH_NAME = "hihi"
YOLO_UUID = "4A02"
HOMEGPS_UUID = "4A03"



async def scan_for_device():
    devices = await BleakScanner.discover(timeout=5.0)
    for d in devices:
        if d.name and ESP_BLUETOOTH_NAME in d.name:
            print(f"Znaleziono obiekt: {d.name} [{d.address}]")
            return d.address
    return None


def detect_person(frame, height, width):
    results = model.predict(frame, imgsz=320, classes=[0], conf=0.45, verbose=False)
    found_person = False
    angle_x = 0.0
    angle_y = 0.0

    
    cx_screen, cy_screen = width // 2, height // 2
    cv2.drawMarker(frame, (cx_screen, cy_screen), (0, 255, 255), cv2.MARKER_CROSS, 20, 2)

    for r in results:
        for box in r.boxes:
            class_id = int(box.cls[0])
            confidence = box.conf[0].item()

            if class_id == 0 and confidence > 0.5:
                found_person = True
                x1, y1, x2, y2 = map(int, box.xyxy[0])

                center_x = (x1 + x2) / 2.0
                center_y = y1 + 0.3 * (y2 - y1)


                angle_x = ((center_x - (width / 2.0)) / width) * 75.0
                angle_y = ((center_y - (height / 2.0)) / height) * 46.0

                cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                cv2.circle(frame, (int(center_x), int(center_y)), 5, (0, 0, 255), -1)
                cv2.putText(frame, f"err_x: {angle_x:.1f} deg", (20, 40),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 0, 0), 2)
                cv2.putText(frame, f"err_y: {angle_y:.1f} deg", (20, 75),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 0, 0), 2)
                break

    if not found_person:
        cv2.putText(frame, "LOST TARGET (0x80)", (20, 40),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)

    return frame, found_person, angle_x, angle_y


async def main():
   
    cap = cv2.VideoCapture(2)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
    if not cap.isOpened():
        print("Nie udalo sie otworzycvideo2")
        cap = cv2.VideoCapture(0)

    if not cap.isOpened():
        print("Blad: Brak dostepnej kamery!")
        return

    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    print(f"Kamera otwarta: {width}x{height}")

    addr = await scan_for_device()
    if not addr:
        print("Nie znaleziono ESP32!")
        cap.release()
        return

    
    async with BleakClient(addr) as client:
        print("Polaczono z ESP32! Wysylam kalibracje")
        calib_pkt = struct.pack('<fff?', 50.0600, 19.9400, 45.0, True)
        await client.write_gatt_char(HOMEGPS_UUID, calib_pkt)
     

        try:
            while True:
                ret, frame = cap.read()
                if not ret:
                    break

                frame, found_person, angle_x, angle_y = detect_person(frame, height, width)

                if found_person:
                    flags = 0x00 
                    pkt = struct.pack('<ffB', angle_x, angle_y, flags)
                else:
                    flags = 0x80  
                    pkt = struct.pack('<ffB', 0.0, 0.0, flags)

                await client.write_gatt_char(YOLO_UUID, pkt, response=False)

                cv2.imshow("AI Vision Tracker", frame)
                if (cv2.waitKey(1) & 0xFF) == ord('q'):
                    break

        finally:
            cap.release()
            cv2.destroyAllWindows()


if __name__ == "__main__":
    asyncio.run(main())
