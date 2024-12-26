/*--------------------------------------------------------- Include Dependencies ---------------------------------------------------------------------*/
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <LittleFS.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/


/*-------------------------------------------------------- Preprocessor Directives -------------------------------------------------------------------*/
#define BAUD_RATE                             115200
#define RX_ESP_PIN                            16
#define TX_ESP_PIN                            17
#define MQTT_PORT_NUM                         1883
#define WRITE_PACKET_SIZE                     75
#define BL_MESSAGE_SIZE                       256
#define TIME_OUT_VALUE                        2000
#define KEY_SIZE                              16 
#define NACK_CODE                             0xAB
#define ACK_CODE                              0xCD
#define SUCCESSFUL_ERASE_STATUS               0x03
#define UNSUCCESSFUL_ERASE_STATUS             0x02
#define SUCCESSFUL_PAGE_WRITE_STATUS          0x01
#define OUTPUT_PIN_RESET                      4
#define OUTPUT_PIN_LED                        5
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/


/*--------------------------------------------------------- Special Setup Variable -------------------------------------------------------------------*/
// Wi-Fi credentials
const char* WIFI_SSID     = "oppoF9";
const char* WIFI_PASSWORD = "reemreem259";

// Firebase credentials
const char* API_KEY           = "AIzaSyAhY7tnh7VDgHg9Kv7ydKveuhPw3eyz-VI";
const char* USER_EMAIL        = "shawky25557032@gmail.com";
const char* USER_PASSWORD     = "1qazzxcv";
const char* STORAGE_BUCKET_ID = "fota-431f5.appspot.com";

// MQTT credentials
//const char* MQTT_SERVER = "test.mosquitto.org"; // MQTT broker
const char* MQTT_SERVER   = "broker.hivemq.com"; // MQTT broker

// Binary File details
String FILENAME = "";
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------ Global Variables ----------------------------------------------------------------------*/
String idToken;
const char*  Topic_Name = "ELC";
WiFiClient   espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;

uint8_t flash_erase_sector = 0;
uint8_t flash_erase_num_sectors = 0;

uint32_t mem_write_address = 0;
uint8_t  mem_write_payload_length = 0;

uint32_t jump_address = 0;

// Bootloader packet buffer
uint8_t packet[WRITE_PACKET_SIZE];     // 71 bytes + 4 CRC byte
uint8_t buf[3] = {0};                  // Hexadecimal string buffer

// Command Flags
uint8_t CID_flag = 0;
uint8_t VER_flag = 0;
uint8_t Erase_flag   = 0;
uint8_t Write_status = 0;
uint8_t Write_flag = 0;
uint8_t Jump_flag  = 0;
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------- Functions Definition --------------------------------------------------------------------*/
// Function to connect to Wi-Fi
void connectToWiFi(void) 
{
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	Serial.print("Connecting to Wi-Fi");
	while (WiFi.status() != WL_CONNECTED) 
  {
		delay(1000);
		Serial.print(".");
	}
	Serial.println("\nConnected to Wi-Fi");
}

// RC4 key scheduling algorithm (KSA)
void rc4_key_schedule(const unsigned char *key, size_t key_len, unsigned char *S) 
{
    unsigned char temp;
    unsigned char j = 0;

    // Initialize the state array S
    for (int i = 0; i < 256; i++) 
    {
        S[i] = i;
    }
    // Perform the key scheduling algorithm
    for (int i = 0; i < 256; i++) {
        j = (j + S[i] + key[i % key_len]) % 256;
        // Swap S[i] and S[j]
        temp = S[i];
        S[i] = S[j];
        S[j] = temp;
    }
}

// RC4 stream cipher (encryption and decryption)
void rc4_crypt(const unsigned char *input, size_t len, const unsigned char *key, size_t key_len, unsigned char *output) 
{
    unsigned char S[256];
    unsigned char i = 0, j = 0, k;
    unsigned char temp;

    // Key scheduling
    rc4_key_schedule(key, key_len, S);

    // Encryption / Decryption
    for (size_t n = 0; n < len; n++) 
    {
        i = (i + 1) % 256;
        j = (j + S[i]) % 256;
        // Swap S[i] and S[j]
        temp = S[i];
        S[i] = S[j];
        S[j] = temp;
        // Generate keystream byte
        k = S[(S[i] + S[j]) % 256];
        // XOR keystream with input to produce output
        output[n] = input[n] ^ k;
    }
}

void handle_encrypt(String input_mess)
{
  unsigned char key[KEY_SIZE] = "ThisIsASecret";  		// Example key (128-bit)  
  unsigned char encrypted_message[KEY_SIZE];
  char buf[KEY_SIZE];

  input_mess.toCharArray(buf, KEY_SIZE,0);
  // Encrypt the message
  rc4_crypt((unsigned char*)buf, KEY_SIZE, key, KEY_SIZE, encrypted_message);
  Serial2.write(encrypted_message, sizeof(encrypted_message));
  encrypted_message[KEY_SIZE]= '\0';  
  Serial.printf("Encrypted message is %s\n", encrypted_message);
}

void handle_response(void) 
{
	uint8_t ack_packet[2] = {0};           // Assuming 2 bytes: 1 for ACK/NACK, 1 for message length
	uint8_t message[256]  = {0};           // Buffer for the message (adjust size as needed)
  uint8_t message_length = 0;            // The length of the message received from BL
  unsigned long start_time;
  
	// Step 1: Read the ACK packet
	Serial.println("Waiting for ACK...");
	start_time = millis();
  // Wait for ACK for certain time-out 
	while (Serial2.available() < 1) 
  {     
		if (millis() - start_time > TIME_OUT_VALUE) 
    { 
			Serial.println("Timeout waiting for ACK packet!");
			return;
		}
	}
	Serial2.readBytes(&ack_packet[0], 1);  // Read the first byte (ACK or NACK)
	// Check ACK/NACK
  switch (ack_packet[0])
  {
  case ACK_CODE:
    Serial.println("Received ACK");       
    while (Serial2.available() < 1) 
    {     
      if (millis() - start_time > TIME_OUT_VALUE) 
      { 
        Serial.println("Timeout waiting for Message Length!");
        return;
      }
    }
    Serial2.readBytes(&ack_packet[1], 1);      // Read the second byte (message length)
    break;

  case NACK_CODE:
    Serial.println("Received NACK!");
    return;     // Return from the hole function
    break;

  default:
    Serial.println("Error in receiving ACK/NACK from BootLoader!");
    return;     // Return from the hole function
  }

	// Get the message length from ACK packet
	message_length = ack_packet[1];
	Serial.printf("Received ACK. Message length: %d\n", message_length);

	// Step 2: Read the message packet
	if (message_length > 0) 
  {
		Serial.println("Waiting for message...");
		start_time = millis();
		while (Serial2.available() < message_length) 
    {
			if (millis() - start_time > TIME_OUT_VALUE) 
      { 
				Serial.println("Timeout waiting for Message packet!");
				return;
			}
		}
		Serial2.readBytes(message, message_length);  // Read the full message

   handle_publish_BL_mess(message, message_length);     
  }
}


void handle_publish_BL_mess(uint8_t * message, uint8_t message_length)
{
	// Create a String to hold the payload
	String payload = "";

	if (VER_flag == 1)
	{
		for (int i = 0; i < message_length; i++) 
		{
			payload += String(message[i]);
			if (i < message_length - 1) payload += ".";     // Add Dot separator between values
		}
		VER_flag = 0;
	}

	else if (CID_flag == 1)
	{
		payload += "0x";
		payload += String(*((uint16_t *)message), HEX);
		CID_flag = 0;
	}

	else if (Erase_flag == 1)
	{
		if (message[0] == SUCCESSFUL_ERASE_STATUS)
		{
			payload = "Successful Erase";
		}
		else if(message[0] == UNSUCCESSFUL_ERASE_STATUS)
		{
			payload = "Unsuccessful Erase !!";
		}
		else
		{
			payload = "Invalid Erase !!";
		}
		Erase_flag = 0;
	}
	else if (Write_flag == 1)
	{
		if (message[0] == SUCCESSFUL_PAGE_WRITE_STATUS)
		{
			Write_status = 1;
		}
		else
		{
			Write_status = 0;
		}
	}
  else if (Jump_flag == 1)
  {
    if (message[0] == 0x01)
		{
			payload = "Successful Jump";
		}
		else
		{
			payload = "Invalid Jump !!";
		}
		Jump_flag = 0;
  }
	else    // For Help command
	{
		for (int i = 0; i < message_length; i++) 
    {
			// Convert each byte to a hexadecimal string and append it
			if (message[i] < 0x10) payload += "0"; // Add leading zero for single-digit hex
			payload += String(message[i], HEX);
				
      if (i < message_length - 1) payload += ","; // Add comma separator between values
		}
	}
  if (Write_flag != 1)
  {
    // Publish the payload
    if (client.publish(Topic_Name, payload.c_str())) 
    {
      Serial.println("Bootloader message published successuflly as string");
      Serial.println(payload);
    } 
    else 
    {
      Serial.println("Failed to publish bootloader message");
    } 
  }
}


// Function to authenticate with Firebase
bool authenticateWithFirebase() 
{
	HTTPClient http;
	String url = "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=" + String(API_KEY);

	String payload = "{\"email\":\"" + String(USER_EMAIL) + "\",\"password\":\"" + String(USER_PASSWORD) + "\",\"returnSecureToken\":true}";
	http.begin(url);
	http.addHeader("Content-Type", "application/json");

	int httpResponseCode = http.POST(payload);
	if (httpResponseCode == 200) {
		String response = http.getString();
		int startIndex = response.indexOf("\"idToken\":\"") + 10;
		int endIndex = response.indexOf("\",", startIndex);
		idToken = response.substring(startIndex, endIndex);
		Serial.println("Firebase authentication successful");
		http.end();
		return true;
	} else {
		Serial.println("Firebase authentication failed");
		Serial.println(http.getString());
		http.end();
		return false;
	}
}

// MQTT reconnect
void reconnect() 
{
	while (!client.connected()) 
  {
    digitalWrite(OUTPUT_PIN_LED, LOW);   // Initialize pin to LOW (inactive)
		Serial.print("Attempting MQTT connection...");
		// Generate a unique client ID
		String clientId = "ESPClient_" + String(random(0xffff), HEX);
		if (client.connect(clientId.c_str())) 
    {
			Serial.println("Connected to MQTT broker with client ID: " + clientId);
			client.subscribe(Topic_Name);
      digitalWrite(OUTPUT_PIN_LED, HIGH);   // Initialize pin to HIGH (active)
		} 
    else 
    {
			Serial.print("Failed, rc=");
			Serial.print(client.state());
      digitalWrite(OUTPUT_PIN_LED, LOW);   // Initialize pin to LOW (inactive)
			Serial.println(" retrying in 5 seconds...");
			delay(5000); // Wait before retrying
		}  
	}
}

// Function to download file from Firebase Storage
bool downloadFile() 
{
	HTTPClient http;
	String url = "https://firebasestorage.googleapis.com/v0/b/" + String(STORAGE_BUCKET_ID) + "/o/" + (FILENAME) + "?alt=media";  http.begin(url);
	http.addHeader("Authorization", "Bearer " + idToken);

	int httpResponseCode = http.GET();
	if (httpResponseCode == 200) 
  {
		Serial.println("File download successful");

		// Save file to LittleFS
		File file = LittleFS.open("/" + (FILENAME), FILE_WRITE);
		if (!file) 
    {
			Serial.println("Failed to open file for writing");
			return false;
		}

		// Read data in chunks and write to file
		WiFiClient* stream = http.getStreamPtr();
		uint8_t buffer[128]; // Buffer size
		size_t writtenBytes = 0;
		while (http.connected() && (stream->available() > 0 || writtenBytes < http.getSize())) 
    {
			size_t len = stream->readBytes(buffer, sizeof(buffer));
			file.write(buffer, len);
			writtenBytes += len;
		}
		file.close();
		Serial.println("File saved to LittleFS");
		http.end();
		return true;
	} 
  else 
  {
		Serial.println("File download failed");
		Serial.println(http.getString());
		http.end();
		return false;
	}
}


uint32_t calculate_crc(uint8_t *Buffer, size_t Buffer_Length) 
{
	uint32_t CRC_Value = 0xFFFFFFFF; // Initial CRC value
	uint8_t DataElem;
	uint8_t DataElemBitLen;

	for (size_t i = 0; i < Buffer_Length; i++) 
  {
		DataElem = Buffer[i];
		CRC_Value ^= ((uint32_t)DataElem << 24); // Align 8-bit data to the MSB of 32-bit CRC

		for (DataElemBitLen = 0; DataElemBitLen < 8; DataElemBitLen++) 
    {
			if (CRC_Value & 0x80000000) { // Check the MSB
				CRC_Value = (CRC_Value << 1) ^ 0x04C11DB7; // Polynomial XOR
			} else {
				CRC_Value = (CRC_Value << 1); // Shift left
			}
		}
	}
	return CRC_Value;
}


// Function to Build Command Packet
void build_command_packet(uint8_t command_code, uint8_t *payload, size_t payload_length, uint8_t *packet, size_t *packet_length) 
{
	*packet_length = payload_length + 2 + 4; // Length byte + Command Code + CRC (4 bytes)
	packet[0] = payload_length + 1;         // Length excluding the Length byte itself
	packet[1] = command_code;
	// Copy Payload
	memcpy(&packet[2], payload, payload_length);
	// Calculate CRC
	uint32_t crc = calculate_crc(packet, payload_length + 2);
	memcpy(&packet[payload_length + 2], &crc, 4);
}


// Function to Send Command Packet
void send_command_packet(uint8_t *packet, size_t packet_length) 
{
	Serial.println("Sending command packet:");
	for (size_t i = 0; i < packet_length; i++) 
  {
		Serial.printf("0x%02X ", packet[i]);
	}
	Serial.println();
	// Send packet to STM32 (via UART )
	Serial2.write(packet, packet_length);
}


// Command Handling Functions
void handle_get_version() 
{
	uint8_t packet[6]; // Fixed packet size of 6 bytes
	VER_flag = 1;
	// B1: Packet length excluding this byte
	packet[0] = 0x05;

	// B2: Command code
	packet[1] = 0x10;

	// Calculate CRC for B1 and B2
	uint32_t crc = calculate_crc(packet, 2);

	// B3-B6: CRC 
	memcpy(&packet[2], &crc, 4);

	Serial.println("Sending command packet:");
	for (size_t i = 0; i < 6; i++) 
  {
		Serial.printf("0x%02X ", packet[i]);
	}
	Serial.println();
	// Sending the entire packet over Serial2
	Serial2.write(packet, sizeof(packet));
	handle_response();
}


void handle_get_help() 
{
	uint8_t packet[6]; // Fixed packet size of 6 bytes
	// B1: Packet length excluding this byte
	packet[0] = 0x05;
	// B2: Command code
	packet[1] = 0x11;
	// Calculate CRC for B1 and B2
	uint32_t crc = calculate_crc(packet, 2);
	// B3-B6: CRC 
	memcpy(&packet[2], &crc, 4);
	Serial.println("Sending command packet:");
	for (size_t i = 0; i < 6; i++) 
  {
		Serial.printf("0x%02X ", packet[i]);
	}
	Serial.println();
	// Sending the entire packet over Serial2
	Serial2.write(packet, sizeof(packet));
	handle_response();
}

void handle_get_cid() 
{
	uint8_t packet[6]; // Fixed packet size of 6 bytes

	CID_flag = 1;
	// B1: Packet length excluding this byte
	packet[0] = 0x05;

	// B2: Command code
	packet[1] = 0x12;

	// Calculate CRC for B1 and B2
	uint32_t crc = calculate_crc(packet, 2);

	// B3-B6: CRC 
	memcpy(&packet[2], &crc, 4);

	Serial.println("Sending command packet:");
	for (size_t i = 0; i < 6; i++) 
  {
		Serial.printf("0x%02X ", packet[i]);
	}
	Serial.println();
	// Sending the entire packet over Serial2
	Serial2.write(packet, sizeof(packet));
	handle_response();
}

void handle_flash_erase(uint8_t sector, uint8_t num_sectors) 
{
	uint8_t packet[8]; // Fixed packet size of 8 bytes
	Erase_flag = 1;

	// B1: Packet length excluding this byte
	packet[0] = 0x07;
	// B2: Command code
	packet[1] = 0x15;
	// B3: Sector number 
	packet[2] = sector ;
	// B4: number of sectors 
	packet[3] =  num_sectors ;
	// Calculate CRC for B1 and B2
	uint32_t crc = calculate_crc(packet, 4);
	// B5-B8: CRC 
	memcpy(&packet[4], &crc, 4);

	Serial.println("Sending command packet:");
	for (size_t i = 0; i < sizeof(packet); i++) 
  {
		Serial.printf("0x%02X ", packet[i]);
	}
	Serial.println();
	// Sending the entire packet over Serial2
	Serial2.write(packet, sizeof(packet));
	handle_response();
}

void handle_jump_to_addr(uint32_t address) 
{
	uint8_t packet[10]; // Fixed packet size of 10 bytes
  Jump_flag = 1;
	// B1: Packet length excluding this byte
	packet[0] = 0x09;

	// B2: Command code
	packet[1] = 0x14;

	// B3:B6 : address 
  *((uint32_t *)(packet + 2)) = address;
	// Calculate CRC 
	uint32_t crc = calculate_crc(packet, 6);
	// B7-B10: CRC 
	memcpy(&packet[6], &crc, 4);

	Serial.println("Sending command packet:");
	for (size_t i = 0; i < 10; i++) 
  {
		Serial.printf("0x%02X ", packet[i]);
	}
	Serial.println();
	// Sending the entire packet over Serial2
	Serial2.write(packet, sizeof(packet));

  handle_response();
}


void handle_mem_write(uint32_t address) 
{
  File file = LittleFS.open("/" + (FILENAME), "r");
	Write_flag = 1;
	while (file.available()) 
  {
		file.size();
		char counterByte = 0;
		packet[0] = 74;
		packet[1] = 0x16;
    *((uint32_t *)(packet + 2)) = address;
		packet[6] = 64;
		char var = 0xff;
		while (counterByte < 64) 
    {
			memcpy(buf, &var, 2);
			file.readBytes((char*)buf, 2);
			char HexData = strtol((const char*)buf, NULL, 16);
			packet[7 + counterByte] = HexData;
			counterByte++;
		}
		uint32_t crc = calculate_crc(packet, 71);
		memcpy(&packet[71], &crc, 4);
		counterByte = 0;
		Serial2.write(packet, sizeof(packet));
		handle_response();

		if (Write_status == 0)
		{
			Serial.println("Unsuccessful Write !!");
      String payload = "Unsuccessful Write";
      client.publish(Topic_Name, payload.c_str());
			return;
		}
    address += 64;
	}
	Serial.println("Successful Write ");
  String payload = "Successful Write ";
  client.publish(Topic_Name, payload.c_str());
  Write_flag = 0 ;
}


void handle_reset(void)
{
  Serial.println("Please Wait For Green LED ON ...");
  String payload = "Please Wait For Green LED ON ...";
  client.publish(Topic_Name, payload.c_str());

  // Reset the STM
  digitalWrite(OUTPUT_PIN_RESET, LOW);
  delay(500);
  digitalWrite(OUTPUT_PIN_RESET, HIGH);
  //Reset the ESP
  esp_restart();
}


// MQTT Callback
void callback(char* topic, byte* payload, unsigned int length) 
{
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("]: ");
	for (int i = 0; i < length; i++) 
  {
		Serial.print((char)payload[i]);
	}
	Serial.println();

	// Convert payload to a String
	String message;
	for (int i = 0; i < length; i++) 
  {
		message += (char)payload[i];
	}

	// Command Parsing
	if (message.equals("GET_VER")) 
  {
		Serial.println("GET_VER command received.");
		handle_get_version();
	} 
  else if (message.equals("GET_HELP")) 
  {
		Serial.println("GET_HELP command received.");
		handle_get_help();
	} 
  else if (message.equals("GET_CID")) 
  {
		Serial.println("GET_CID command received.");
		handle_get_cid();
	} 
  else if (message.equals("FLASH_ERASE")) 
  {
		Serial.println("FLASH_ERASE command received. Waiting for sector and num_sectors...");
		// Reset the values
    flash_erase_sector = 0;    
		flash_erase_num_sectors = 0;
	} 
  else if (message.startsWith("SECTOR")) 
  {
		flash_erase_sector = message.substring(message.indexOf(' ') + 1).toInt();
		Serial.print("Sector set to: ");
		Serial.println(flash_erase_sector);
	} 
  else if (message.startsWith("NUM_SECTORS")) 
  {
		flash_erase_num_sectors = message.substring(message.indexOf(' ') + 1).toInt();
		Serial.print("Number of sectors set to: ");
		Serial.println(flash_erase_num_sectors);
		// Once both parameters are received, send the command
		handle_flash_erase(flash_erase_sector, flash_erase_num_sectors);
	}
  else if (message.equals("FLASH_MASS_ERASE")) 
  {
		Serial.println("FLASH_MASS_ERASE command received.");
		flash_erase_sector = 0xFF; 
		flash_erase_num_sectors = 0;
    handle_flash_erase(flash_erase_sector, flash_erase_num_sectors);
	} 
  else if (message.equals("MEM_WRITE")) 
  {
		Serial.println("MEM_WRITE command received. Waiting for address and payload length...");
    // Reset the values
		mem_write_address = 0;
		mem_write_payload_length = 0;
	} 
  else if (message.startsWith("ADDRESS_WRITE")) 
  {
    // Extract the address string
    String address_string = message.substring(message.indexOf(' ') + 1);
    // Convert the extracted string to an integer using strtol with base 16
    mem_write_address = strtol(address_string.c_str(), nullptr, 16);
    Serial.print("Base memory address set to: ");
    Serial.println(mem_write_address, HEX);  // Print in hexadecimal format for clarity
    // Fetch data from LittleFS and send the command
    handle_mem_write(mem_write_address);
}
  else if (message.equals("JUMP_TO_ADDRESS")) 
  {
		Serial.println("JUMP_TO_ADDRESS command received. Waiting for address ...");
		jump_address = 0;
	} 
  else if (message.startsWith("ADDRESS_JUMP")) 
  {
    // Extract the address string
    String address_string = message.substring(message.indexOf(' ') + 1);
    // Convert the extracted string to an integer using strtol with base 16
    jump_address = strtol(address_string.c_str(), nullptr, 16);
    Serial.print("Base memory address set to: ");
    Serial.println(jump_address, HEX);  // Print in hexadecimal format for clarity
    // Fetch data from LittleFS and send the command
    handle_jump_to_addr(jump_address);
  }
  else if (message.equals("DOWNLOAD_FROM_FIREBASE")) 
  {
		Serial.println("DOWNLOAD_FROM_FIREBASE command received.");
	}
  else if (message.startsWith("FILE_NAME")) 
  {
    // Extract the address string
    FILENAME = message.substring(message.indexOf(' ') + 1);
    Serial.print("Base memory address set to: ");
    Serial.println(FILENAME);  
    // Fetch data from LittleFS and send the command
    if (downloadFile() == true)
    {
      Serial.println("Successful Download");
      String payload = "Successful Download";
      client.publish(Topic_Name, payload.c_str());
    }
    else
    {
      Serial.println("Download Failed");
      String payload = "Download Failed";
      client.publish(Topic_Name, payload.c_str());
    }
  }
  else if (message.equals("RESET")) 
  {
		Serial.println("RESET command received for both STM and ESP.");
    handle_reset();
	} 
  else if (message.startsWith("DISPLAY_MESSAGE")) 
  {
    String input_mess = message.substring(message.indexOf(' ') + 1);

    Serial.print("DISPLAY_MESSAGE command recived ");
    handle_encrypt(input_mess);  
  }
  else 
  {
    // Nothing
	}
}
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------- Entry Point --------------------------------------------------------------------------*/
void setup() 
{
	Serial.begin(BAUD_RATE);
	Serial2.begin(BAUD_RATE, SERIAL_8N1, RX_ESP_PIN, TX_ESP_PIN); 

  pinMode(OUTPUT_PIN_RESET, OUTPUT);      // Set pin as output
  digitalWrite(OUTPUT_PIN_RESET, HIGH);   // Initialize pin to LOW (inactive)

  pinMode(OUTPUT_PIN_LED, OUTPUT);        // Set pin as output
  digitalWrite(OUTPUT_PIN_LED, LOW);      // Initialize pin to LOW (inactive)

	// Initialize LittleFS
	if (!LittleFS.begin(true)) 
  {
		Serial.println("An error occurred while mounting LittleFS");
		return;
	}
	// Connect to Wi-Fi
	connectToWiFi();
	// Authenticate with Firebase
	if (!authenticateWithFirebase()) 
  {
		Serial.println("Authentication failed");
		return;
	}
	// Setup MQTT
	client.setServer(MQTT_SERVER, MQTT_PORT_NUM);
	client.setCallback(callback);
}


void loop() 
{
	if (!client.connected()) 
  {
		reconnect();
	}
  client.loop();
}