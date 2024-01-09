/*
 * Copyright 2022 Veloman Yunkan <veloman.yunkan@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU  General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "i18n.h"

#include "tools/otherTools.h"

#include <algorithm>
#include <map>

namespace kiwix
{

const char* I18nStringTable::get(const std::string& key) const
{
  const I18nString* const begin = entries;
  const I18nString* const end = begin + entryCount;
  const I18nString* found = std::lower_bound(begin, end, key,
      [](const I18nString& a, const std::string& k) {
        return a.key < k;
  });
  return (found == end || found->key != key) ? nullptr : found->value;
}

namespace i18n
{
// this data is generated by the i18n resource compiler
extern const I18nStringTable stringTables[];
extern const size_t langCount;
}

namespace
{

class I18nStringDB
{
public: // functions
  I18nStringDB() {
    for ( size_t i = 0; i < kiwix::i18n::langCount; ++i ) {
      const auto& t = kiwix::i18n::stringTables[i];
      lang2TableMap[t.lang] = &t;
    }
    enStrings = lang2TableMap.at("en");
  };

  std::string get(const std::string& lang, const std::string& key) const {
    const char* s = getStringsFor(lang)->get(key);
    if ( s == nullptr ) {
      s = enStrings->get(key);
      if ( s == nullptr ) {
        throw std::runtime_error("Invalid message id");
      }
    }
    return s;
  }

  size_t getStringCount(const std::string& lang) const {
    try {
      return lang2TableMap.at(lang)->entryCount;
    } catch(const std::out_of_range&) {
      return 0;
    }
  }

private: // functions
  const I18nStringTable* getStringsFor(const std::string& lang) const {
    try {
      return lang2TableMap.at(lang);
    } catch(const std::out_of_range&) {
      return enStrings;
    }
  }

private: // data
  std::map<std::string, const I18nStringTable*> lang2TableMap;
  const I18nStringTable* enStrings;
};

const I18nStringDB& getStringDb()
{
  static const I18nStringDB stringDb;
  return stringDb;
}

} // unnamed namespace

std::string getTranslatedString(const std::string& lang, const std::string& key)
{
  return getStringDb().get(lang, key);
}

namespace i18n
{

std::string expandParameterizedString(const std::string& lang,
                                      const std::string& key,
                                      const Parameters& params)
{
  kainjow::mustache::object mustacheParams;
  for( const auto& kv : params ) {
    mustacheParams[kv.first] = kv.second;
  }
  const std::string tmpl = getTranslatedString(lang, key);
  return render_template(tmpl, mustacheParams);
}

} // namespace i18n

std::string ParameterizedMessage::getText(const std::string& lang) const
{
  return i18n::expandParameterizedString(lang, msgId, params);
}

namespace
{

LangPreference parseSingleLanguagePreference(const std::string& s)
{
  const size_t langStart = s.find_first_not_of(" \t\n");
  if ( langStart == std::string::npos ) {
    return {"", 0};
  }

  const size_t langEnd = s.find(';', langStart);
  if ( langEnd == std::string::npos ) {
    return {s.substr(langStart), 1};
  }

  const std::string lang = s.substr(langStart, langEnd - langStart);
  // We don't care about langEnd == langStart which will result in an empty
  // language name - it will be dismissed by parseUserLanguagePreferences()

  float q = 1.0;
  int nCharsScanned;
  if ( 1 == sscanf(s.c_str() + langEnd + 1, "q=%f%n", &q, &nCharsScanned)
       && langEnd + 1 + nCharsScanned == s.size() ) {
    return {lang, q};
  }

  return {"", 0};
}

} // unnamed namespace

UserLangPreferences parseUserLanguagePreferences(const std::string& s)
{
  UserLangPreferences result;
  std::istringstream iss(s);
  std::string singleLangPrefStr;
  while ( std::getline(iss, singleLangPrefStr, ',') )
  {
    const auto langPref = parseSingleLanguagePreference(singleLangPrefStr);
    if ( !langPref.lang.empty() && langPref.preference > 0 ) {
      result.push_back(langPref);
    }
  }

  return result;
}

std::string selectMostSuitableLanguage(const UserLangPreferences& prefs)
{
  if ( prefs.empty() ) {
    return "en";
  }

  std::string bestLangSoFar("en");
  float bestScoreSoFar = 0;
  const auto& stringDb = getStringDb();
  for ( const auto& entry : prefs ) {
    const float score = entry.preference * stringDb.getStringCount(entry.lang);
    if ( score > bestScoreSoFar ) {
      bestScoreSoFar = score;
      bestLangSoFar = entry.lang;
    }
  }
  return bestLangSoFar;
}

} // namespace kiwix
