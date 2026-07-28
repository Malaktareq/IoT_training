#include <FS.h>
#include <SPIFFS.h>

void listFiles(const char *dir) ;
void createAndWriteFiles() ;
void writeFile(const char *path, const char *content);

void setup() {
  Serial.begin(115200);

  if (SPIFFS.begin(true)) {
    Serial.println("SPIFFS mounted successfully");

    // Create and write data to three different files
    createAndWriteFiles();

    // List files in the root directory
    listFiles("/");
  } else {
    Serial.println("SPIFFS mount failed");
  }
}

void loop() {
  // Nothing to do here
}

void createAndWriteFiles() {
  writeFile("/file1.txt", "File 1 in the house");
  writeFile("/file2.txt", "File 2 holds cabbages?!");
  writeFile("/file3.txt", "Content for file 3 is the biggest of them all!");
}

void writeFile(const char *path, const char *content) {
  Serial.print("Creating file: ");
  Serial.println(String(path));

  File file = SPIFFS.open(path, "w");
  if (file) {
    file.print(content);
    file.close();
    Serial.println("File created and written successfully");
  } else {
    Serial.println("Failed to create file");
  }
}

void listFiles(const char *dir) {
  Serial.print("Listing files in directory: ");
  Serial.println(String(dir));

  File root = SPIFFS.open(dir);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    Serial.println("File: " + String(file.name()) + ", Size: " + file.size());
    file = root.openNextFile();
  }
}