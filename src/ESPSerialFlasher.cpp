#include "ESPSerialFlasher.h"

#include "serial_io.h"
#include "serial_comm.h"
#include "esp_loader.h"


static int32_t s_time_end;

const int RXD2=16;
const int TXD2=17;
const int RESET_ESP_PIN=4;
const int yelLED_pin = 33;
const int greenLED_pin = 32;
const int redLED_pin = 22;

Print * ESPDebugPort = &Serial;
bool ESPDebug = false;

void ESPFlasherInit( bool _debug, Print * _debugPort ){
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
    //SerialNina.begin(115200);
    pinMode(yelLED_pin, OUTPUT);
    digitalWrite(yelLED_pin, HIGH);
    pinMode(RESET_ESP_PIN, OUTPUT);
    pinMode(greenLED_pin, OUTPUT);
    digitalWrite(greenLED_pin, HIGH);
    //pinMode(NINA_GPIO0, OUTPUT);
    ESPDebug = _debug;
    ESPDebugPort = _debugPort;
    if(ESPDebug) ESPDebugPort->println("ESP Flasher Init");
}

void yelLED(bool state){
    pinMode(yelLED_pin, OUTPUT);
    digitalWrite(yelLED_pin, state);
}


void redLED(bool state){
    pinMode(redLED_pin, OUTPUT);
    digitalWrite(redLED_pin, state);
}
void greenLED(bool state){
    pinMode(greenLED_pin, OUTPUT);
    digitalWrite(greenLED_pin, state);
}
void greenLEDflash(){
    pinMode(greenLED_pin, OUTPUT);
    digitalWrite(greenLED_pin, !digitalRead(greenLED_pin));
}

bool ESPFlasherConnect(){
    //loader_port_reset_target();
    //esp_loader_change_baudrate(115200);
    if(connect_to_target(921600) != ESP_LOADER_SUCCESS)
        return false;
    else
        return true;
}

bool ESPFlashBin(const char* binFilename, unsigned long startAddress){
    //ESPDebugPort->println("erasing all");
    //ESPDebugPort->println(loader_eraseAllFlash_cmd());
    //ESPDebugPort->println("erasing done");
    
	if(ESPDebug) ESPDebugPort->println("WARNING! DO NOT INTERRUPT OR WIFI-MODULE WILL BE CORRUPT");
	if(SD.exists(binFilename)){
        if(ESPDebug) ESPDebugPort->print("Found File ");
        if(ESPDebug) ESPDebugPort->println(binFilename);
        File UpdateFile = SD.open(binFilename, FILE_READ);
        size_t size = UpdateFile.size();
        if(ESPDebug) ESPDebugPort->print("File Size ");
        if(ESPDebug) ESPDebugPort->println(size);
        if(size <= 0x247000){
            if(flash_binary(UpdateFile,  size,  startAddress)==ESP_LOADER_SUCCESS){
                UpdateFile.close();
                loader_port_reset_target();
                return true;
            }
            else{
                UpdateFile.close();
                loader_port_reset_target();
                return false;
            }
        } else {
            if(ESPDebug) ESPDebugPort->println("File too large for partition");
            return false;
        }
        UpdateFile.close();
        loader_port_reset_target();
        
	}
    else{
        if(ESPDebug) ESPDebugPort->println("File doesnt exist");
        loader_port_reset_target();
        return false;
    }
    
}

void ESPFlashCert(const char* certFilename){
	if(ESPDebug) ESPDebugPort->println("WARNING! DO NOT INTERRUPT OR WIFI-MODULE WILL BE CORRUPT");
    if(SD.exists(certFilename)){
        File CertFile = SD.open(certFilename, FILE_READ);
        size_t size = CertFile.size();
        if(size <= 0x20000){
        flash_binary(CertFile,  size,  0x10000);
        } else {
            if(ESPDebug) ESPDebugPort->println("File too large for partition");
        }
        CertFile.close();
     }
     else if(ESPDebug) ESPDebugPort->println("File doesnt exist");
     loader_port_reset_target();
	
}

void ESPFlashCertFromMemory(const char* Certificates, unsigned long size){
    	if(ESPDebug) ESPDebugPort->println("WARNING! DO NOT INTERRUPT OR WIFI-MODULE WILL BE CORRUPT");

        if(size <= 0x20000){
        flash_binary_from_memory((const uint8_t*) Certificates,  size,  0x10000);
        } else {
            if(ESPDebug) ESPDebugPort->println("File too large for partition");
        
     }
    
     loader_port_reset_target();
    
}

esp_loader_error_t loader_port_serial_write(const uint8_t *data, uint16_t size, uint32_t timeout)
{
    

   size_t err = Serial2.write((const char *)data, size);

    if (err == size) {
        return ESP_LOADER_SUCCESS;
    } else 
        return ESP_LOADER_ERROR_FAIL;
    
}


esp_loader_error_t loader_port_serial_read(uint8_t *data, uint16_t size, uint32_t timeout)
{
	Serial2.setTimeout(timeout);
    int read = Serial2.readBytes( data, size);

    if (read < 0) {
        return ESP_LOADER_ERROR_FAIL;
    } else if (read < size) {
        return ESP_LOADER_ERROR_TIMEOUT;
    } else {
        return ESP_LOADER_SUCCESS;
    }
}


// Set GPIO0 LOW, then
// assert reset pin for 50 milliseconds.
void loader_port_enter_bootloader(void)
{
    digitalWrite(RESET_ESP_PIN, 0);
    loader_port_reset_target();
    loader_port_delay_ms(50);
    digitalWrite(RESET_ESP_PIN, 1);
}


void loader_port_reset_target(void)
{
    digitalWrite(RESET_ESP_PIN, 0);
    loader_port_delay_ms(100);
    digitalWrite(RESET_ESP_PIN, 1);
}


void loader_port_delay_ms(uint32_t ms)
{
    delay(ms );
}


void loader_port_start_timer(uint32_t ms)
{
    s_time_end = millis() + ms;
}


uint32_t loader_port_remaining_time(void)
{
    int32_t remaining = (s_time_end - millis()) ;
    return (remaining > 0) ? (uint32_t)remaining : 0;
}


void loader_port_debug_print(const char *str)
{
    if(ESPDebug) ESPDebugPort->print( str);
}

esp_loader_error_t loader_port_change_baudrate(uint32_t baudrate)
{
    Serial2.updateBaudRate(baudrate);
    //Serial2.end();
    //Serial2.begin(baudrate, SERIAL_8N1, RXD2, TXD2);
    //Serial2.begin(baudrate);
    //int err = Serial2;
    //return (err == true) ? ESP_LOADER_SUCCESS : ESP_LOADER_ERROR_FAIL;
    return ESP_LOADER_SUCCESS;
}

esp_loader_error_t connect_to_target(uint32_t higher_baudrate)
{
 esp_loader_connect_args_t connect_config = ESP_LOADER_CONNECT_DEFAULT();

    esp_loader_error_t err = esp_loader_connect(&connect_config);
    if (err != ESP_LOADER_SUCCESS) {
        if(ESPDebug) ESPDebugPort->print("Cannot connect to target. Error:");
        if(ESPDebug) ESPDebugPort->println(err);
        return err;
    }
    if(ESPDebug) ESPDebugPort->print("Connected to target\n");
    if(ESPDebug) ESPDebugPort->println(esp_loader_get_target());

    if (higher_baudrate && esp_loader_get_target() != ESP8266_CHIP) {
        //if(ESPDebug) ESPDebugPort->print("changing baudrate\n");
        err = esp_loader_change_baudrate(higher_baudrate);
       // if(ESPDebug) ESPDebugPort->print("almost done changing baudrate\n");
        
        if (err == ESP_LOADER_ERROR_UNSUPPORTED_FUNC) {
            if(ESPDebug) ESPDebugPort->print("ESP8266 does not support change baudrate command.");
            return err;
        } else if (err != ESP_LOADER_SUCCESS) {
            if(ESPDebug) ESPDebugPort->print("Unable to change baud rate on target.");
            return err;
        } else {
          //  if(ESPDebug) ESPDebugPort->print("changing loader baud\n");
            err = loader_port_change_baudrate(higher_baudrate);
           // if(ESPDebug) ESPDebugPort->print("done changing loader\n");
            if (err != ESP_LOADER_SUCCESS) {
                if(ESPDebug) ESPDebugPort->print("Unable to change baud rate.");
                return err;
            }
            printf("Baudrate changed\n");
        }
    }

    return ESP_LOADER_SUCCESS;
}


esp_loader_error_t flash_binary(File file, size_t size, unsigned long address)
{
    digitalWrite(greenLED_pin, LOW);
	
    esp_loader_error_t err;
    uint8_t payload[1024];
    
    if(ESPDebug) ESPDebugPort->print("Erasing flash (this may take a while)...\n");
    err = esp_loader_flash_start(address, size, sizeof(payload));
    if (err != ESP_LOADER_SUCCESS) {
        if(ESPDebug) ESPDebugPort->print("Erasing flash failed with error : ");if(ESPDebug) ESPDebugPort->println(err);
        return err;
    }
    if(ESPDebug) ESPDebugPort->print("Start programming\n");
	if(ESPDebug) ESPDebugPort->print("\rProgress: ");
    size_t binary_size = size;
    size_t written = 0;
    int previousProgress = -1;
    while (size > 0) {
        digitalWrite(yelLED_pin, !digitalRead(yelLED_pin));
        size_t to_read = MIN(size, sizeof(payload));
        file.read(payload,to_read);
        err = esp_loader_flash_write(payload, to_read);
        if (err != ESP_LOADER_SUCCESS) {
            if(ESPDebug) ESPDebugPort->print("\nPacket could not be written! Error : ");
            if(ESPDebug) ESPDebugPort->println( err);
            return err;
        }

        size -= to_read;
        written += to_read;

        int progress = (int)(((float)written / binary_size) * 100);
        if(previousProgress != progress)
        {
		previousProgress = progress;
        if(ESPDebug) ESPDebugPort->print(progress);if(ESPDebug) ESPDebugPort->print(",");
       	}
    };
    digitalWrite(yelLED_pin, LOW);
    digitalWrite(greenLED_pin, HIGH);
    if(ESPDebug) ESPDebugPort->print("\nFinished programming\n");

#if MD5_ENABLED
    err = esp_loader_flash_verify();
    if (err == ESP_LOADER_ERROR_UNSUPPORTED_FUNC) {
        if(ESPDebug) ESPDebugPort->print("ESP8266 does not support flash verify command.");
        return err;
    } else if (err != ESP_LOADER_SUCCESS) {
        if(ESPDebug) ESPDebugPort->print("MD5 does not match. err: %d\n");
        if(ESPDebug) ESPDebugPort->print( err);
        return err;
    }
    if(ESPDebug) ESPDebugPort->print("Flash verified\n");
#endif

    return ESP_LOADER_SUCCESS;
}

esp_loader_error_t flash_binary_from_memory(const uint8_t *bin, size_t size, size_t address){
    
		
	
    esp_loader_error_t err;
    uint8_t payload[1024];
    const uint8_t *bin_addr = bin;
    
    if(ESPDebug) ESPDebugPort->print("Erasing flash (this may take a while)...\n");
    err = esp_loader_flash_start(address, size, sizeof(payload));
    if (err != ESP_LOADER_SUCCESS) {
        if(ESPDebug) ESPDebugPort->print("Erasing flash failed with error : ");if(ESPDebug) ESPDebugPort->println(err);
        return err;
    }
    if(ESPDebug) ESPDebugPort->print("Start programming\n");
   	if(ESPDebug) ESPDebugPort->print("\rProgress: ");
    size_t binary_size = size;
    size_t written = 0;
    int previousProgress = -1;
    while (size > 0) {
        size_t to_read = MIN(size, sizeof(payload));
        memcpy(payload, bin_addr, to_read);
   
  
        err = esp_loader_flash_write(payload, to_read);
        if (err != ESP_LOADER_SUCCESS) {
            if(ESPDebug) ESPDebugPort->print("\nPacket could not be written! Error : ");
            if(ESPDebug) ESPDebugPort->println( err);
            return err;
        }

        size -= to_read;
        bin_addr += to_read;
        written += to_read;

        int progress = (int)(((float)written / binary_size) * 100);
        if(previousProgress != progress)
        {
		previousProgress = progress;
        if(ESPDebug) ESPDebugPort->print(progress);if(ESPDebug) ESPDebugPort->print(",");
       	}
    };

    if(ESPDebug) ESPDebugPort->print("\nFinished programming\n");

#if MD5_ENABLED
    err = esp_loader_flash_verify();
    if (err == ESP_LOADER_ERROR_UNSUPPORTED_FUNC) {
        if(ESPDebug) ESPDebugPort->print("ESP8266 does not support flash verify command.");
        return err;
    } else if (err != ESP_LOADER_SUCCESS) {
        if(ESPDebug) ESPDebugPort->print("MD5 does not match. err: %d\n");
        if(ESPDebug) ESPDebugPort->print( err);
        return err;
    }
    if(ESPDebug) ESPDebugPort->print("Flash verified\n");
#endif

    return ESP_LOADER_SUCCESS;
}    
    
