import serial
import numpy as np
import cv2
import argparse


def receive_images(args):
    # Open the serial port
    ser = serial.Serial(port=args.port, baudrate=args.baudrate, timeout=args.timeout)
    print(f"Serial port {ser.name} opened. Baudrate: {ser.baudrate}")

    # Initialize an empty list to accumulate data
    n_images = 0
    IMAGE_WIDTH = args.width
    IMAGE_HEIGHT = args.width
    IMAGE_CHANNELS = 3
    IMAGE_TOTAL_PIXELS = IMAGE_WIDTH * IMAGE_HEIGHT * IMAGE_CHANNELS

    # First read the number of images
    try: 
        data = ser.read(2) # uint16_t
        n_images = np.frombuffer(data, dtype=np.uint16)[0]
    except KeyboardInterrupt:
        print("Reception interrupted by user.")
    
    print(f"Number of images to receive: {n_images}")
    
    if n_images > 100:
        print("Too many images to receive. Exiting.")
        ser.close()
        return
    images_received = np.zeros((n_images, 224, 224, 3), dtype=np.uint8)

    # Read the images
    for i in range(n_images):
        rcv_list = []
        print(f"Receiving image {i+1}/{n_images}")
        # Read the data
        try:
            data = ser.read(IMAGE_TOTAL_PIXELS)
            rcv_list.append(np.frombuffer(data, dtype=np.uint8))
        except KeyboardInterrupt:
            print("Reception interrupted by user.")
            break
        # Convert the list to a numpy array
        rcv_array = np.concatenate(rcv_list)

        print(f"Data received. Length: {len(rcv_array)}")

        # Reshape the array to the original image shape
        img = rcv_array.reshape(IMAGE_HEIGHT, IMAGE_WIDTH, IMAGE_CHANNELS)
        images_received[i] = img

    # Close the serial port
    ser.close()

    return images_received

def display_images(images):
    for i, img in enumerate(images):
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        cv2.imshow(f"Image {i}", img)
        cv2.waitKey(0)
        cv2.destroyAllWindows()

if __name__ == "__main__":
    # Define the command line arguments for the script
    parser = argparse.ArgumentParser()
    parser.add_argument('--port', '-p', type=str, default= "/dev/ttyUSB0", help='Serial port to connect to')
    parser.add_argument('--baudrate', '-b', type=int, default=921600, help='Baudrate for the serial connection')
    parser.add_argument('--timeout', '-t', type=int, default=5, help='Timeout for the serial connection')
    parser.add_argument('--width', '-w', type=int, default=224, help='Width of the image')
    args = parser.parse_args()

    images_received = receive_images(args)
    if images_received is not None:
        display_images(images_received)
