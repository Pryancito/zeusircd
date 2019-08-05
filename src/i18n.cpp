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

#include "i18n.h"

#define PATH_SEPARATOR "/"

#include <fstream>
#include <iostream>

LauGettext* LauGettext::instance_ = NULL;


LauGettext* LauGettext::instance() {
  if (instance_) return instance_;
  instance_ = new LauGettext();
  return instance_;
}


void LauGettext::destroyInstance() {
  if (instance_) delete instance_;
  instance_ = NULL;
}


LauGettext::LauGettext() {
  catalogueName_ = "locale";
  languageCode_ = "en";
  countryCode_ = "US";
  locale_ = "en_US";
}


LauGettext::~LauGettext() {

}


std::string LauGettext::moFilePath() const {
  std::string filePath = catalogueLocation();
  filePath.append(PATH_SEPARATOR);
  filePath.append(locale());
  filePath.append(PATH_SEPARATOR);
  filePath.append(catalogueName());
  filePath.append(".mo");

  std::ifstream fileTest(filePath.c_str(), std::ios::in | std::ios::binary);
  if (fileTest) return filePath;

  filePath = catalogueLocation();
  filePath.append(PATH_SEPARATOR);
  filePath.append(languageCode());
  filePath.append(PATH_SEPARATOR);
  filePath.append(catalogueName());
  filePath.append(".mo");
  std::ifstream fileTest2(filePath.c_str(), std::ios::in | std::ios::binary);
  if (fileTest2) return filePath;

  return "";
}


GettextMessage* LauGettext::getTranslation(const char* originalString, int originalLength) const {
  if (!moParser_.ready()) {
    std::string p = moFilePath();
    if (p.empty()) return NULL; // File doesn't exist or cannot be opened
    bool success = moParser_.parseFile(moFilePath().c_str());
    if (!success) {
      // The file could not be parsed
      return NULL;
    }
  }

  return moParser_.getTranslation(originalString, originalLength);
}


void LauGettext::setCatalogueName(std::string name) {
  catalogueName_ = name;
}


void LauGettext::setCatalogueLocation(std::string location) {
  catalogueLocation_ = location;
}


void LauGettext::setLocale(std::string localeCode) {
  size_t splitIndex = localeCode.find("_");
  if (splitIndex != localeCode.npos) {
    languageCode_ = localeCode.substr(0, splitIndex);
    countryCode_ = localeCode.substr(splitIndex + 1, 10);
  } else {
    languageCode_ = localeCode;
    countryCode_ = "";
  }

  if (countryCode_.empty()) {
    locale_ = languageCode_;
  } else {
    locale_ = languageCode_;
    locale_.append("_");
    locale_.append(countryCode_);
  }

  moParser_.clearData();
}
