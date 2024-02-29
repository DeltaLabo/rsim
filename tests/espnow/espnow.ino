#include <esp_now.h>
#include <WiFi.h>

// REPLACE WITH THE MAC Address of your receiver 
uint8_t broadcastAddress[] = {0xC0, 0x4E, 0x30, 0x3A, 0x03, 0x34}; // For Xiao
//uint8_t broadcastAddress[] = {0x48, 0x27, 0xE2, 0xE6, 0xDC, 0x84}; // For YD

// Define variables to store readings to be sent
int colorTX;
int counterTX;

// Define variables to store incoming readings
int colorRX;
int counterRX;

// Variable to store if sending data was successful
String success;

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    int color;
    int counter;
} struct_message;

// Create a struct_message for local readings
struct_message localReadings;

// Create a struct_message to hold incoming readings
struct_message incomingReadings;

esp_now_peer_info_t peerInfo;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  Serial.print("\nBytes received: ");
  Serial.print(len);
  colorRX = incomingReadings.color;
  counterRX = incomingReadings.counter;
  Serial.print(" | Color: ");
  Serial.print(colorRX);
  Serial.print(" | Counter: ");
  Serial.println(counterRX);
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200);
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

  localReadings.color = 0; // For Xiao
  //localReadings.color = 1; // For YD
  localReadings.counter = 0;
}
 
void loop() {
  localReadings.counter++;

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &localReadings, sizeof(localReadings));
  
  if (result == ESP_OK) {
    Serial.println("\nSent with success");
  }
  else {
    Serial.println("\nError sending the data");
  }
  delay(2000);
}