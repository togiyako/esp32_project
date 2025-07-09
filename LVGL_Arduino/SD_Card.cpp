#include "SD_Card.h"
#include "SD.h"
#define LOG_BUFFER_SIZE 1024
#include "esp_timer.h"

char logBuffer[LOG_BUFFER_SIZE];
char currentLogFilename[32] = "/log_data_0.csv";
void GenerateNewLogFilename();
size_t bufferIndex = 0;
File logFile;
int64_t lastWriteTime = 0;
const int64_t INACTIVITY_TIMEOUT_US = 300LL * 1000000; 
uint16_t SDCard_Size;
uint16_t Flash_Size;

bool SD_Init() {
  // SD
  pinMode(SD_CS, OUTPUT);    
  digitalWrite(SD_CS, HIGH);               
  if (SD.begin(SD_CS, SPI)) {
    printf("SD card initialization successful!\r\n");
  } else {
    printf("SD card initialization failed!\r\n");
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
    printf("No SD card attached\r\n");
    return false;
  }
  else{
    printf("SD Card Type: ");
    if(cardType == CARD_MMC){
      printf("MMC\r\n");
    } else if(cardType == CARD_SD){
      printf("SDSC\r\n");
    } else if(cardType == CARD_SDHC){
      printf("SDHC\r\n");
    } else {
      printf("UNKNOWN\r\n");
    }
    uint64_t totalBytes = SD.totalBytes();
    uint64_t usedBytes = SD.usedBytes();
    SDCard_Size = totalBytes/(1024*1024);
    printf("Total space: %llu\n", totalBytes);
    printf("Used space: %llu\n", usedBytes);
    printf("Free space: %llu\n", totalBytes - usedBytes);
  }
  GenerateNewLogFilename();
  return true;
}

void Log_OpenFile() {
  if (!logFile || !logFile) {
    logFile = SD.open(currentLogFilename, FILE_APPEND);
    if (logFile) {
      printf(" File open for recording: %s\r\n", currentLogFilename);
    } else {
      printf(" Error opening file: %s\r\n", currentLogFilename);
    }
  }
}

void Log_CloseFile() {
  if (logFile) {
    logFile.close();
    printf(" File closed due to inactivity.\r\n");
  }
}

void GenerateNewLogFilename() {
  File root = SD.open("/");
  int maxIndex = -1;

  while (true) {
    File file = root.openNextFile();
    if (!file) break;

    const char* name = file.name();
    if (strstr(name, "log_data_") && strstr(name, ".csv")) {
      int index = -1;
      
      sscanf(name, "log_data_%d.csv", &index);
      if (index > maxIndex) maxIndex = index;
    }
    file.close(); 
  }
  root.close();

  snprintf(currentLogFilename, sizeof(currentLogFilename), "/log_data_%d.csv", maxIndex + 1);
  printf("New log file will be: %s\r\n", currentLogFilename);
}


void Log_FlushToSD();

void Log_Write(const char* data) {

  uint16_t dataLength = strlen(data);
  
  if (bufferIndex + dataLength < LOG_BUFFER_SIZE) {
    strncpy(logBuffer + bufferIndex, data, dataLength);
    bufferIndex += dataLength;
  } else {
    Log_FlushToSD();
    
    strncpy(logBuffer + bufferIndex, data, dataLength);
    bufferIndex += dataLength;
  }

  if (bufferIndex >= LOG_BUFFER_SIZE * 0.95) {
    Log_FlushToSD();  
  }
}

void Log_FlushToSD() {
  if (bufferIndex > 0) {
    int64_t startTime = esp_timer_get_time(); 

    Log_OpenFile(); 
    if (!logFile) return;

    logFile.write((const uint8_t*)logBuffer, bufferIndex);
    logFile.flush();

    lastWriteTime = esp_timer_get_time(); 

    int64_t endTime = esp_timer_get_time(); 
    float duration_sec = (endTime - startTime) / 1000000.0;
    float speed_kbps = (bufferIndex / 1024.0) / duration_sec;

    printf(" Data recorded: %d bytes | %.2f KB/s\r\n", bufferIndex, speed_kbps);

    memset(logBuffer, 0, LOG_BUFFER_SIZE);
    bufferIndex = 0;
  }
}

void Log_CheckInactivity() {
  int64_t now = esp_timer_get_time();
  if (logFile && (now - lastWriteTime > INACTIVITY_TIMEOUT_US)) {
    Log_CloseFile();
  }
}


/*bool File_Search(const char* directory, const char* fileName)    
{
  File Path = SD.open(directory);
  if (!Path) {
    printf("Path: <%s> does not exist\r\n",directory);
    return false;
  }
  File file = Path.openNextFile();
  while (file) {
    if (strcmp(file.name(), fileName) == 0) {                           
      if (strcmp(directory, "/") == 0)
        printf("File '%s%s' found in root directory.\r\n",directory,fileName);  
      else
        printf("File '%s/%s' found in root directory.\r\n",directory,fileName); 
      Path.close();                                                     
      return true;                                                     
    }
    file = Path.openNextFile();                                        
  }
  if (strcmp(directory, "/") == 0)
    printf("File '%s%s' not found in root directory.\r\n",directory,fileName);           
  else
    printf("File '%s/%s' not found in root directory.\r\n",directory,fileName);          
  Path.close();                                                         
  return false;                                                         
}
*/
/*uint16_t Folder_retrieval(const char* directory, const char* fileExtension, char File_Name[][100],uint16_t maxFiles)    
{
  File Path = SD.open(directory);
  if (!Path) {
    printf("Path: <%s> does not exist\r\n",directory);
    return false;
  }
  
  uint16_t fileCount = 0;
  char filePath[100];
  File file = Path.openNextFile();
  while (file && fileCount < maxFiles) {
    if (!file.isDirectory() && strstr(file.name(), fileExtension)) {
      strncpy(File_Name[fileCount], file.name(), sizeof(File_Name[fileCount])); 
      if (strcmp(directory, "/") == 0) {                                      
        snprintf(filePath, 100, "%s%s", directory, file.name());   
      } else {                                                            
        snprintf(filePath, 100, "%s/%s", directory, file.name());
      }
      printf("File found: %s\r\n", filePath);
      fileCount++;
    }
    file = Path.openNextFile();                                      
  }
  Path.close();                                                         
  if (fileCount > 0) {
    printf(" %d <%s> files were retrieved\r\n",fileCount,fileExtension);
    return fileCount;                                                 
  } else {
    printf("No files with extension '%s' found in directory: %s\r\n", fileExtension, directory);
    return 0;                                                         
  }
}
*/
/*void remove_file_extension(char *file_name) {
  char *last_dot = strrchr(file_name, '.');
  if (last_dot != NULL) {
    *last_dot = '\0'; 
  }
}
*/
/*
void Flash_test() {
  printf("********** RAM Test**********\r\n");
  
  // Get Flash size
  uint32_t flashSize = ESP.getFlashChipSize();
  Flash_Size = flashSize / 1024 / 1024;  // Make sure Flash_Size is declared and used properly

  printf("Flash size: %d MB \r\n", Flash_Size);

  printf("******* RAM Test Over********\r\n\r\n");
}
*/