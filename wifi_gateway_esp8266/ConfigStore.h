#ifndef configstore_h
#define configstore_h

#include <Arduino.h>
#include <String.h>
#include <FS.h>

#define CONFIGSTORE_LIMIT 100

#define AUTOSAVE_ON 1
#define AUTOSAVE_OFF 0

struct ConfigItem {
  String key;
  String value;
};

class ConfigStore {
  private:
    String fname;
    bool confLoaded = false;
    bool autosaving = true;
    bool autosavingChanged = false; /* if something was changed during autosave off, then this value will indicate that */
    ConfigItem configArray[CONFIGSTORE_LIMIT];
    byte configCountItems;
    bool CreateConfig();
    bool LoadConfig();
    bool SaveConfig();
    
  public:
    ConfigStore(String FileName);
    String get(String key);
    bool set(String key, String value);
    bool remove(String key);
    void autosave(byte option);
  
};

#endif
