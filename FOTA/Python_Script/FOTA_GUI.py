import tkinter as tk
from tkinter import simpledialog
from tkinter import messagebox
import time
import paho.mqtt.client as mqtt

# Callback when the client connects to the broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        log_message("Connected to broker successfully")
    else:
        log_message(f"Failed to connect with result code {rc}")

# Callback when a message is received
def on_message(client, userdata, msg):
    if msg.topic == current_topic and msg.payload.decode() in sent_messages:
        sent_messages.remove(msg.payload.decode())
        return
    log_message(f"Received message: {msg.payload.decode()}")

# Log messages to the text area
def log_message(message):
    if message == "Received message: 10,11,12,15,16":
        text_area.insert(tk.END, "Received message: \n0x10: GET_VERSION  \n0x11: GET_HELP   \n0x12: GET_CID  \n0x14: JUMP_TO_ADDRESS  \n0x15: FLASH_ERASE  \n0x16: MEM_WRITE\n")
    else:
        text_area.insert(tk.END, message + "\n")
    text_area.see(tk.END)

# Update the topic
def update_topic():
    global current_topic
    new_topic = topic_entry.get()
    if new_topic:
        if current_topic:
            client.unsubscribe(current_topic)
            log_message(f"Unsubscribed from topic: {current_topic}")
        current_topic = new_topic
        client.subscribe(current_topic)
        log_message(f"Subscribed to topic: {current_topic}")

# Send a basic message
def send_message(message):
    if current_topic:
        sent_messages.append(message)
        client.publish(current_topic, message)
        log_message(f"Sent message: {message}")

# Handle FLASH_ERASE button
def flash_erase():
    sector_number = simpledialog.askinteger("FLASH_ERASE", "Enter Page Number (0-200):", minvalue=0, maxvalue=200)
    if sector_number is None:
        return
    num_sectors = simpledialog.askinteger("FLASH_ERASE", "Enter Number of Pages to Erase :", minvalue=1, maxvalue=200)
    if num_sectors is None:
        return
    send_message("FLASH_ERASE")
    time.sleep(0.5)
    send_message(f"SECTOR {sector_number}")
    time.sleep(0.5)
    send_message(f"NUM_SECTORS {num_sectors}")

# Handle MEM_WRITE button
def mem_write():
    address = simpledialog.askstring("MEM_WRITE", "Enter Address (e.g., 0x8008000):")
    if address is None:
        return

    send_message("MEM_WRITE")
    time.sleep(0.5)
    send_message(f"ADDRESS_WRITE {address}")

# Handle JUMP_TO_ADDRESS button
def jump_to_add():
    address = simpledialog.askstring("JUMP_TO_ADDRESS", "Enter Address (e.g., 0x8008000):")
    if address is None:
        return
    send_message("JUMP_TO_ADDRESS")
    time.sleep(0.5)
    send_message(f"ADDRESS_JUMP {address}")

# Handle DOWNLOAD_FROM_FIREBASE button
def download_from_firebase():
    name = simpledialog.askstring("FILE_NAME", "Enter the binary file name (e.g., file.bin)")
    if name is None:
        return
    send_message("DOWNLOAD_FROM_FIREBASE")
    time.sleep(0.5)
    send_message(f"FILE_NAME {name}")

# Handle DISPLAY_MESSAGE button
def application():
    input_mess = simpledialog.askstring("DISPLAY_MESSAGE", "Enter the message:")
    if input_mess is None:
        return
    send_message(f"DISPLAY_MESSAGE {input_mess}")

# Handle FLASH_MASS_ERASE button
def flash_mass_erase():
    send_message("FLASH_MASS_ERASE")

# Clear the log messages
def clear_log():
    text_area.configure(state="normal")
    text_area.delete(1.0, tk.END)

# GUI setup
root = tk.Tk()
root.title("FOTA Control Panel")
root.geometry("750x750")
root.configure(bg="#212121")  # Dark background for the main window

# Title Label
title_label = tk.Label(root, text="FOTA Control Panel", font=("Arial", 20, "bold"), fg="#ffcc00", bg="#212121")
title_label.pack(pady=10)

# Main frame
main_frame = tk.Frame(root, padx=15, pady=15, bg="#333333", bd=10, relief="ridge")
main_frame.pack(fill="both", expand=True, padx=15, pady=15)

# Topic Entry
topic_frame = tk.LabelFrame(main_frame, text="MQTT Topic", padx=10, pady=10, bg="#444444", fg="#ffffff", font=("Arial", 12))
topic_frame.grid(row=0, column=0, columnspan=2, sticky="ew", pady=10)

main_frame.columnconfigure(1, weight=1)
topic_label = tk.Label(topic_frame, text="Topic:", fg="#ffffff", bg="#444444", font=("Arial", 12))
topic_label.grid(row=0, column=0, sticky="w")

topic_entry = tk.Entry(topic_frame, width=40, bg="#ffffff", fg="#000000", font=("Arial", 12))
topic_entry.grid(row=0, column=1, sticky="ew", padx=10)

topic_button = tk.Button(topic_frame, text="Set Topic", command=update_topic, bg="#ff6347", fg="#ffffff", font=("Arial", 10))
topic_button.grid(row=0, column=2, padx=10)

# Buttons for predefined messages
commands_frame = tk.LabelFrame(main_frame, text="Predefined Commands", padx=10, pady=10, bg="#444444", fg="#ffffff", font=("Arial", 12))
commands_frame.grid(row=1, column=0, columnspan=2, sticky="ew", pady=10)

commands = ["GET_VER", "GET_HELP", "GET_CID", "RESET"]
for idx, command in enumerate(commands):
    button = tk.Button(commands_frame, text=command, command=lambda cmd=command: send_message(cmd), bg="#4CAF50", fg="#ffffff", font=("Arial", 10))
    button.grid(row=0, column=idx, padx=10, pady=10)

# Functional buttons
functions_frame = tk.LabelFrame(main_frame, text="Functions", padx=10, pady=10, bg="#444444", fg="#ffffff", font=("Arial", 12))
functions_frame.grid(row=2, column=0, columnspan=2, sticky="ew", pady=10)

buttons_data = [
    ("FLASH_ERASE", flash_erase, "#007ACC"),
    ("FLASH_MASS_ERASE", flash_mass_erase, "#007ACC"),
    ("MEM_WRITE", mem_write, "#007ACC"),
    ("JUMP_TO_ADDRESS", jump_to_add, "#007ACC"),
    ("DOWNLOAD_FROM_FIREBASE", download_from_firebase, "#007ACC"),
    ("DISPLAY_MESSAGE", application, "#007ACC")
]

for idx, (text, func, color) in enumerate(buttons_data):
    button = tk.Button(functions_frame, text=text, command=func, bg=color, fg="#ffffff", font=("Arial", 10))
    button.grid(row=idx // 3, column=idx % 3, padx=5, pady=5, sticky="ew")

# Text area for logs
log_frame = tk.LabelFrame(main_frame, text="Log Messages", padx=5, pady=5, bg="#444444", fg="#ffffff", font=("Arial", 12))
log_frame.grid(row=3, column=0, columnspan=2, sticky="nsew", pady=5)

# Configure row/column to ensure text_area grows properly
log_frame.grid_rowconfigure(0, weight=1)
log_frame.grid_columnconfigure(0, weight=1)

text_area = tk.Text(log_frame, height=10, wrap="word", bg="#000000", fg="#00FF00", font=("Arial", 12))
text_area.grid(row=0, column=0, sticky="nsew")

scrollbar = tk.Scrollbar(log_frame, command=text_area.yview)
scrollbar.grid(row=0, column=1, sticky="ns")
text_area["yscrollcommand"] = scrollbar.set

# Clear Log button
clear_log_button = tk.Button(main_frame, text="Clear Log", command=clear_log, bg="#ff6347", fg="#ffffff", font=("Arial", 11))
clear_log_button.grid(row=4, column=0, columnspan=2, pady=5)

# MQTT setup
broker = "broker.hivemq.com"
port = 1883
sent_messages = []
current_topic = None

client = mqtt.Client(client_id="PythonMQTTClient")
client.on_connect = on_connect
client.on_message = on_message
client.on_disconnect = lambda client, userdata, rc: log_message("Disconnected from broker")

client.connect(broker, port, 300)
client.loop_start()

root.mainloop()