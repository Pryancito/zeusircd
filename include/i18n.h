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
