# 🚀 Firmware Over-the-Air (FOTA) Project

## 📚 Table of Contents
- [👥 Contributors ](#Contributors )
- [📖 Project Overview](#project-overview)
- [🎯 Objectives](#objectives)
- [✨ Key Features](#key-features)
- [🔧 Technical Components](#technical-components)
- [🔄 Workflow](#workflow)
- [💡 Benefits](#benefits)
- [🚀 Future Enhancements](#future-enhancements)
- [⚙️ Setup and Usage](#setup-and-usage)
  - [🔑 Prerequisites](#prerequisites)
  - [📝 Steps to Run](#steps-to-run)
- [📂 Repository Structure](#repository-structure)
- [📥 Clone the Repository](#clone-the-repository)
- [🧪 Test Application](#test-application)
- [📽️ Demo Video ](#demovideo )
- [🤝 Contributions](#contributions)


---
### 👥 Contributors  

**Reem Mohy Eldin Abdelrahman**  
[GitHub Profile](https://github.com/ReemMohy259)  

**Mohamed Gamal Mohamed Ahmed**  
[GitHub Profile](https://github.com/Mohamed100100) 

**Mohammed Ahmed Mohamed Shawky**  
[GitHub Profile](https://github.com/AboAlsho2) 

---

## 📖 Project Overview

The **Firmware Over-the-Air (FOTA)** project enables remote firmware updates for an **STM32 ARM Cortex M3** microcontroller. This system allows firmware updates to be deployed wirelessly, eliminating the need for physical intervention. It is designed to be scalable and robust for real-world embedded applications such as IoT, industrial automation, and automotive systems.

---

## 🎯 Objectives

- **Develop a bootloader** for STM32 to manage firmware updates efficiently.
- **Enable remote firmware updates** using an ESP module for wireless communication.
- **Ensure reliability** in firmware transmission and installation.
- **Provide a user-friendly Python-based GUI** for managing and monitoring updates.
- **Implement cloud integration** for firmware storage and retrieval.

---

## ✨ Key Features

### 🛠️ Bootloader
- Manages firmware updates and application execution.
- Verifies firmware integrity before installation.
- Executes the new firmware after a successful update.

### 📡 Wireless Firmware Updates
- **ESP Module** handles Wi-Fi-based communication.
- **Cloud storage** enables remote firmware retrieval.

### 🖥️ Graphical User Interface (GUI)
- Developed using **Python (Tkinter)** with **MQTT integration**.
- Provides an interface for:
  - **Viewing firmware versions**.
  - **Initiating and monitoring updates**.
  - **Debugging and logging**.
    
![image](https://github.com/user-attachments/assets/c9944ca8-e271-4e26-ad77-f8ed4132bc79)

### ☁️ Cloud Integration
- **ESP module downloads firmware binaries** from Firebase.
- **Ensures devices remain updated** with the latest firmware.

### 🔄 Supported Commands
- **GetVersion 📄**: Retrieve the current firmware version.
- **GetHelp 🆘**: List all available commands.
- **GetCID** : Return the chip identification number.
- **Flash Mass Erase 🧹**: Erase all flash memory.
- **Flash Erase (specific pages) 🗂️**: Erase selected flash memory pages.
- **Memory Write ✍️**: Write new firmware data to memory.
- **JUMP_TO_ADDRESS** : Jump to a specific address to start executing the program.

---

## 🔧 Technical Components

### 🖧 Hardware
- **STM32 ARM Cortex M3** microcontroller.
- **ESP Module** for Wi-Fi-based communication.
- **LCD Module** for message display after decryption.
- **Design a PCB Layout** for integrate all HW components routing.
  ![image](https://github.com/user-attachments/assets/c2bddd91-8463-4167-bf23-046dea1d2d45)


### 🧑‍💻 Software
- **STM32CubeMX** for peripheral configuration.
- **STM32CubeIDE** for STM bootloader programing and test application.
- **Arduino IDE** for ESP module programming.
- **Python (Tkinter, paho-mqtt)** for GUI development.

### 🔄 Communication Protocols
- **MQTT**: Manages communication between GUI and ESP module.
- **UART**: Handles serial communication between STM32 and ESP.

---

## 🔄 Workflow

1. The **GUI sends a firmware update request**.
2. The **ESP module fetches the firmware from Firebase**.
3. The **firmware is transferred to the STM32 microcontroller**.
4. The **STM32 bootloader validates and writes the firmware**.
5. The **device restarts and executes the new firmware**.
![image](https://github.com/user-attachments/assets/eefdeec2-87e0-46d1-a325-5ef3c974181f)

---

## 💡 Benefits

- **🌐 Remote updates** reduce maintenance costs.
- **🔄 Seamless integration** with cloud services.
- **🖥️ User-friendly interface** enhances accessibility.
- **📈 Scalable design** supports multiple devices.

---

## 🚀 Future Enhancements  

- 🌍 **Extend support to additional microcontroller platforms**.  
- ⚡ **Optimize firmware transfer efficiency and reliability**.  
- 🔒 **Implement a secure bootloader for enhanced system protection**.  
- 📡 **Integrate with additional cloud platforms (AWS, Azure, etc.)**.
- 
---

## ⚙️ Setup and Usage

### 🔑 Prerequisites

- **STM32 Development Board**.
- **ESP32 Module**.
- **Python (Tkinter, paho-mqtt, etc.)**.
- **Arduino IDE for ESP programming**.
- **MQTT Broker (e.g., Mosquitto)**.

### 📝 Steps to Run

1. **Flash the STM32 bootloader** to handle firmware updates.
2. **Flash the ESP module with the communication firmware**, ensuring that you update the **Wi-Fi credentials** in the `ESP.ino` file before uploading.

![image](https://github.com/user-attachments/assets/ea53ce9c-d515-4234-aaba-4d03c2a11203)

3. **Configure Firebase storage** for storing firmware binaries.
4. **Run the Python GUI** to manage and monitor updates.

---

## 📂 Repository Structure

```
/Boot-Loader 📁 - STM32 bootloader code
/ESP_Program 📁 - ESP module firmware
/Python_Script 📁 - Python-based GUI
/Test_Application 📁 - Decryption testing for STM32 
```

---

## 📥 Clone the Repository

To get started, clone this repository:

```sh
git clone https://github.com/ReemMohy259/FirmWareOverTheAir-FOTA-.git
```

---

## 🧪 Test Application

### 🔒 Encryption Test

This test validates the encryption and decryption process using the **RC4 algorithm** to ensure correct firmware transmission.

#### Process:

1. **Encryption on ESP**:  
   - The firmware data is **encrypted using the RC4 algorithm** on the ESP module before transmission.  
   - The encrypted data is then sent to the STM32 microcontroller via UART.  

2. **Decryption on STM32**:  
   - The STM32 receives the encrypted firmware data.  
   - The **RC4 decryption algorithm** is executed on STM32 to restore the original data.  

3. **Output on LCD**:  
   - The decrypted message is displayed on the **LCD module**, ensuring correct data reception and processing.  

This test ensures that the firmware transfer process maintains data integrity by verifying that the decrypted output matches the original firmware content before encryption.

---

## 📽️ Demo Video  

Watch the full demonstration of the **Firmware Over-the-Air (FOTA) Project**:  

🔗 **[Watch the Video Here] (https://drive.google.com/drive/folders/1S-VVVFW-HdrNtTotwaMDuzinoIGRefiO?usp=sharing)**  

---

## 🤝 Contributions

Contributions, bug reports, and feature requests are welcome! Feel free to open an issue or submit a pull request.

✨ Happy coding! 🚀

