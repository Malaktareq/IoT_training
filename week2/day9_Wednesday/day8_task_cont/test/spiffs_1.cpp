#include <FS.h>
#include <SPIFFS.h>

// Commenting out this line should get checkIfDirectory() to print "is not a directory"
#define makeDir
void checkIfDirectory(const char *path);
void setup() {
  Serial.begin(115200);

  if (SPIFFS.begin(true)) {
    Serial.println("SPIFFS mounted successfully");

#ifdef makeDir
    File file = SPIFFS.open("/myfolder", "w");
#endif

    // Check if a specific path exists
    checkIfDirectory("/myfolder");
    checkIfDirectory("/myfolder2");

  } else {
    Serial.println("SPIFFS mount failed");
  }
}

void loop() {
  // Nothing to do here
}

void checkIfDirectory(const char *path) {

  if (SPIFFS.exists(path)) {
    Serial.println(String(path) + " is a directory");
  } else {
    Serial.println(String(path) + " is not a directory");
  }
}