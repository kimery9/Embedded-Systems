#    Quest 5: NFC Scooter Key Fob
#    BU ENG EC 444 Spring 2023
#    Team 2: John Culley, Ananth Sanjay, Jeffrey Zhang, Kai Imery
#    04/25/2023 

import io
import time
import requests
from picamera import PiCamera
from PIL import Image
from zbarlight import scan_codes

# Initialize the camera
camera = PiCamera()

# Set camera resolution
camera.resolution = (640, 480)

# Set camera framerate
camera.framerate = 30

# Create a stream to capture video frames
stream = io.BytesIO()

# Start video preview
camera.start_preview()

# Define the URL of the Node.js server
url = 'http://192.168.1.15:8081/receive_qr_code_data'

# Loop to continuously capture and process video frames
while True:
    # Capture a video frame from the camera
    camera.capture(stream, format='jpeg', use_video_port=True)
    stream.seek(0)

    # Load the captured image
    image = Image.open(stream)

    # Convert the image to grayscale
    gray_image = image.convert('L')

    # Scan for QR codes in the image
    codes = scan_codes('qrcode', gray_image)

    # Check if QR codes are found
    if codes is not None:
        qr_code_data = codes[0].decode('utf-8')
        print('QR code scanned:', qr_code_data)

        # Send the QR code data to the Node.js server
        data = {'qrCodeData': qr_code_data}
        response = requests.post(url, json=data)

        if response.status_code == 200:
            print('QR code data sent successfully')
        else:
            print('Failed to send QR code data:', response.status_code, response.text)
    else:
        print('No QR code detected.')

    # Reset the stream for the next capture
    stream.seek(0)
    stream.truncate()

    # Sleep for a short duration to control frame rate
    time.sleep(0.1)

# Clean up
camera.stop_preview()
camera.close()