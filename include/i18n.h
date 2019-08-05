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

#pragma once

#include "MoPsr.h"


class LauGettext {

public:

  static LauGettext* instance();
  static void destroyInstance();

  LauGettext();
  ~LauGettext();
  void setCatalogueLocation(std::string location);
  void setCatalogueName(std::string name);
  void setLocale(std::string localeCode);
  std::string moFilePath() const;
  inline std::string locale() const { return locale_; }
  inline std::string languageCode() const { return languageCode_; }
  inline std::string countryCode() const { return countryCode_; }
  inline std::string catalogueName() const { return catalogueName_; }
  inline std::string catalogueLocation() const { return catalogueLocation_; }

  GettextMessage* getTranslation(const char* originalString, int originalLength) const;

protected:

  mutable GettextMoParser moParser_;

private:

  std::string catalogueLocation_;
  std::string languageCode_;
  std::string countryCode_;
  std::string locale_;
  std::string catalogueName_;

  static LauGettext* instance_;

};
