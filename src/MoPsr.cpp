/* 
 * This file is part of the ZeusiRCd distribution (https://github.com/Pryancito/zeusircd).
 * Copyright (c) 2019 Rodrigo Santidrian AKA Pryan.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <ctype.h>
#include <fstream>
#include <sys/stat.h>

#include "MoPsr.h"


using namespace std;


const int32_t GettextMoParser::HEADER_MAGIC_NUMBER    = 0x950412de;
const int32_t GettextMoParser::HEADER_MAGIC_NUMBER_SW = 0xde120495;


GettextMoParser::GettextMoParser() {
  swappedBytes_ = false;
  moFileHeader_ = NULL;
  moData_ = NULL;
  charset_ = NULL;
  charsetParsed_ = false;
  ready_ = false;
}


int32_t GettextMoParser::swap_(int32_t ui) const {
  return swappedBytes_ ? (ui << 24) | ((ui & 0xff00) << 8) |
                         ((ui >> 8) & 0xff00) | (ui >> 24)
                         : ui;
}


void GettextMoParser::clearData() {
  if (moData_) delete[] moData_;
  if (charset_) delete[] charset_;

  swappedBytes_ = false;
  moFileHeader_ = NULL;
  moData_ = NULL;
  charset_ = NULL;
  charsetParsed_ = false;

  for (int i = 0; i < (int)messages_.size(); i++) {
    TranslatedMessage* message = messages_.at(i);
    if (message->original) {
      delete message->original;
    }
    if (message->translated) {
      delete message->translated;
    }
    delete message;
  }
  messages_.clear();

  ready_ = false;
}


bool GettextMoParser::parseFile(const char* filePath) {
  clearData();

  struct stat fileInfo;

  if (stat(filePath, &fileInfo) != 0) {
    // Cannot get file size
    clearData();
    return false;
  }

  char* moData = new char[fileInfo.st_size];

  ifstream moFile(filePath, ios::in | ios::binary);
  if (!moFile.read(moData, fileInfo.st_size)) {
    // Cannot read file data
    delete[] moData;
    clearData();
    return false;
  }

  return parse(moData);
}


bool GettextMoParser::parse(char* moData) {
  moData_ = moData;

  moFileHeader_ = (MoFileHeader*)moData_;
  if (moFileHeader_->magic != HEADER_MAGIC_NUMBER && moFileHeader_->magic != HEADER_MAGIC_NUMBER_SW) {
    // Invalid header
    clearData();
    return false;
  }

  // If the magic number bytes are swapped, all the other numbers will have
  // to be swapped
  swappedBytes_ = moFileHeader_->magic == HEADER_MAGIC_NUMBER_SW;

  moFileHeader_->magic = swap_(moFileHeader_->magic);
  moFileHeader_->revision = swap_(moFileHeader_->revision);
  moFileHeader_->numStrings = swap_(moFileHeader_->numStrings);
  moFileHeader_->offsetOriginalStrings = swap_(moFileHeader_->offsetOriginalStrings);
  moFileHeader_->offsetTranslatedStrings = swap_(moFileHeader_->offsetTranslatedStrings);
  moFileHeader_->hashTableSize = swap_(moFileHeader_->hashTableSize);
  moFileHeader_->offsetHashTable = swap_(moFileHeader_->offsetHashTable);

  ready_ = true;

  return true;
}


GettextMoParser::~GettextMoParser() {
  clearData();
}


char* GettextMoParser::charset() const {
  if (charset_ || charsetParsed_) return charset_;
  if (!moData_) return NULL;

  charsetParsed_ = true;

  MoOffsetTableItem* translationTable = (MoOffsetTableItem*)(moData_ + moFileHeader_->offsetTranslatedStrings);
  translationTable->length = swap_(translationTable->length);
  translationTable->offset = swap_(translationTable->offset);

  char* infoBuffer = (char*)(moData_ + translationTable->offset);
  std::string info(infoBuffer);
  size_t contentTypePos = info.find("Content-Type: text/plain; charset=");
  if (contentTypePos == info.npos) return NULL;

  size_t stringStart = contentTypePos + 34; // strlen("Content-Type: text/plain; charset=")
  size_t stringEnd = info.find('\n', stringStart);
  if (stringEnd == info.npos) return NULL;

  int charsetLength = stringEnd - stringStart;
  if (charsetLength == 0) return NULL;

  charset_ = new char[charsetLength + 1];
  info.copy(charset_, charsetLength, stringStart);

  charset_[charsetLength] = '\0';

  if (strcmp(charset_, "CHARSET") == 0) {
    delete[] charset_;
    charset_ = NULL;
  }

  // To lowercase
  for(int i = 0; i < (int)strlen(charset_); ++i) charset_[i] = tolower(charset_[i]);

  return charset_;
}


GettextMessage* GettextMoParser::getTranslation(std::string originalString, int originalLength) {

  if (originalLength <= 0) return NULL;

  // Check if the string has already been looked up, in which case it is in the messages_
  // vector. If found, return it and exit now.

  for (int i = 0; i < (int)messages_.size(); i++) {
    TranslatedMessage* message = messages_.at(i);
    if (strcmp(originalString.c_str(), message->original->string.c_str()) == 0) {
      return message->translated;
    }
  }

  // Look for the original message

  MoOffsetTableItem* originalTable = (MoOffsetTableItem*)(moData_ + moFileHeader_->offsetOriginalStrings);
  originalTable->length = swap_(originalTable->length);
  originalTable->offset = swap_(originalTable->offset);

  int stringIndex;
  bool found = false;

  for (stringIndex = 0; stringIndex < moFileHeader_->numStrings; stringIndex++) {
    std::string currentString(moData_ + originalTable->offset);

    if (strcmp(currentString.c_str(), originalString.c_str()) == 0) {
      found = true;
      break;
    }

    originalTable++;
    originalTable->length = swap_(originalTable->length);
    originalTable->offset = swap_(originalTable->offset);
  }


  TranslatedMessage* message = new TranslatedMessage();

  GettextMessage* mOriginal = new GettextMessage();
  mOriginal->length = originalTable->length;
  mOriginal->string = originalString;

  message->original = mOriginal;

  if (!found) {
    // Couldn't find orignal string
    messages_.push_back(message);
    message->translated = NULL;
    return NULL;
  }

  // At this point, stringIndex is the index of the message and originalTable points to
  // the original message data.

  // Get the translated string and build and the output message structure

  MoOffsetTableItem* translationTable = (MoOffsetTableItem*)(moData_ + moFileHeader_->offsetTranslatedStrings);
  translationTable += stringIndex;
  translationTable->length = swap_(translationTable->length);
  translationTable->offset = swap_(translationTable->offset);

  std::string translatedString(moData_ + translationTable->offset);

  GettextMessage* mTranslated = new GettextMessage();
  mTranslated->length = translationTable->length;
  mTranslated->string = translatedString;

  message->translated = mTranslated;

  messages_.push_back(message);

  return message->translated;
}
