#include "SpiffsUtil.h"

SpiffsUtil::SpiffsUtil(){

}

SpiffsUtil::~SpiffsUtil(){

}

/*
  File fw = SPIFFS.open(wrfile.c_str(), "w");
  fw.println( writeStr );
  fw.close();
*/
void SpiffsUtil::begin(){
    // SPIFFS.begin();
    if(!SPIFFS.begin()){
        Serial.println("An Error has occurred while mounting SPIFFS");
        while (1);
    }    
}

File SpiffsUtil::open(const char* fileName, const char* mode) {
    fw = SPIFFS.open(fileName, mode);
    return fw;
}

size_t SpiffsUtil::write(const uint8_t *buf, size_t size) {
    return fw.write(buf, size);
}

size_t SpiffsUtil::read(uint8_t* buf, size_t size) {
    return fw.read(buf, size);
}

size_t SpiffsUtil::position(){
  return fw.position();
}

size_t SpiffsUtil::size(){
  return fw.size();  
}

void SpiffsUtil::close() {
    fw.close();
}

void SpiffsUtil::remove(const char* fileName){
  File file = SPIFFS.open(fileName, FILE_READ);
  if (!file) {
    return;
  }
  file.close();
  
  SPIFFS.remove(fileName);
}

void SpiffsUtil::listDir(const char * dirname, uint8_t levels){
    File root = SPIFFS.open(dirname);
    if(!root){
        return;
    }

    if(!root.isDirectory()){
        return;
    }
 
    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                // サブディレクトリを出力
                listDir(file.name(), levels -1);
            }
        } else {
            // ファイル名とファイルサイズを出力
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

bool SpiffsUtil::exists(const char* path){
    return SPIFFS.exists(path);
}