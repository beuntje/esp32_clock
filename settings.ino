#include <Preferences.h>

Preferences preferences;

void setupSettings() {
  preferences.begin("clock", false);
}

void saveString( char* naam, String waarde) {
  preferences.putString(naam, waarde);
}

String loadString(char* naam) {
  return preferences.getString(naam, "");
}

void saveInt( char* naam, int waarde) {
  preferences.putInt(naam, waarde);
}

int loadInt(char* naam) {
  return preferences.getInt(naam, 0);
}

void saveBool( char* naam, bool waarde) {
  preferences.putBool(naam, waarde);
}

bool loadBool(char* naam) {
  return preferences.getBool(naam, false);
}
