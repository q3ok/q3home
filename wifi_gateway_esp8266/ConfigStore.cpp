#include "ConfigStore.h"

ConfigStore::ConfigStore(String FileName) {
  this->fname = FileName;
}

bool ConfigStore::CreateConfig() {
  File f = SPIFFS.open(this->fname, "w");
  if (!f) {
    return false; /* cannot even create the file? */
  }
  f.println("ConfigStore=true");
  f.close();
  return true;
}

bool ConfigStore::LoadConfig() {
  if (this->confLoaded) return true;
  if ( !SPIFFS.exists(this->fname) && !this->CreateConfig() ) return false;
  
  File f = SPIFFS.open(this->fname, "r");
  if (!f) return false;

  String line, key, value;
  int pos;
  unsigned int i = 0;
  this->configCountItems = 0;

  while (f.available()) {
    line = f.readStringUntil('\n');
    line.trim();
    pos = line.indexOf("=");
    if (pos == -1) continue; /* line is not a config value ?? */
    this->configCountItems++;
    key = line.substring(0, pos);
    value = line.substring(pos+1);
    value.trim();
    this->configArray[i].key = key;
    this->configArray[i++].value = value;
  }
  f.close();
  this->confLoaded = true;
  return true;
}

bool ConfigStore::SaveConfig() {
  if (!this->confLoaded) return false;
  if ( !SPIFFS.exists(this->fname) ) {
    /* ???? how it can not be created?? someone deleted it meadwhile? */
    return false;
  }

  File f = SPIFFS.open( this->fname, "w" );
  if (!f) {
    return false;
  }
  f.println("ConfigStore");
  for (int i=0; i < this->configCountItems; i++) {
    f.print( this->configArray[i].key );
    f.print( "=" );
    f.println( this->configArray[i].value );
  }
  f.close();
  return true;
}

String ConfigStore::get(String key) {
  if ( !this->LoadConfig() ) {
    return "error occured"; /* unbelieave weak error handling here */
  }
  for (int i=0; i < this->configCountItems; i++) {
    if ( this->configArray[i].key == key ) {
      return this->configArray[i].value;
    }
  }
  return ""; /* item not found !! */
}

bool ConfigStore::remove(String key) {
  if ( !this->LoadConfig() ) {
    return false;
  }
  for (int i=0; i < this->configCountItems; i++) {
    if ( this->configArray[i].key == key ) {
      this->configArray[i].key = this->configArray[ (this->configCountItems - 1) ].key;
      this->configArray[i].value = this->configArray[ (this->configCountItems - 1) ].value;
      this->configCountItems--;
      return true;
    } // if
  } // for i
  return false;
}

bool ConfigStore::set(String key, String value) {
  if ( !this->LoadConfig() ) {
    return false;
  }
  if (this->configCountItems >= CONFIGSTORE_LIMIT) {
    return false; /* limit reached */
  }
  if (value.length() < 1) {
    return false; /* dont save empty value */
  }
  key.trim(); value.trim();

  this->remove(key);
  
  this->configArray[this->configCountItems].key = key;
  this->configArray[this->configCountItems].value = value;
  this->configCountItems++;

  if (this->autosaving) this->SaveConfig();
  else this->autosavingChanged = true;

  return true;
  
}

void ConfigStore::autosave(byte option) {
  if ( AUTOSAVE_ON == option ) {
    this->autosaving = true;
    if ( this->autosavingChanged ) {
      this->SaveConfig();
      this->autosavingChanged = false;
    }
  } else
  if ( AUTOSAVE_OFF == option ) {
    this->autosaving = false;
  }
}


