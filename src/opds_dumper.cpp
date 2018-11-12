/*
 * Copyright 2017 Matthieu Gautier <mgautier@kymeria.fr>
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

#include "opds_dumper.h"
#include "book.h"

#include <common/otherTools.h>

namespace kiwix
{
/* Constructor */
OPDSDumper::OPDSDumper(Library* library)
  : library(library)
{
}
/* Destructor */
OPDSDumper::~OPDSDumper()
{
}

std::string gen_date_str()
{
  auto now = time(0);
  auto tm = localtime(&now);

  std::stringstream is;
  is << std::setw(2) << std::setfill('0')
     << 1900+tm->tm_year << "-"
     << std::setw(2) << std::setfill('0') << tm->tm_mon << "-"
     << std::setw(2) << std::setfill('0') << tm->tm_mday << "T"
     << std::setw(2) << std::setfill('0') << tm->tm_hour << ":"
     << std::setw(2) << std::setfill('0') << tm->tm_min << ":"
     << std::setw(2) << std::setfill('0') << tm->tm_sec << "Z";
  return is.str();
}

void OPDSDumper::setOpenSearchInfo(int totalResults, int startIndex, int count)
{
  m_totalResults = totalResults;
  m_startIndex = startIndex,
  m_count = count;
  m_isSearchResult = true;
}

#define ADD_TEXT_ENTRY(node, child, value) (node).append_child((child)).append_child(pugi::node_pcdata).set_value((value).c_str())

pugi::xml_node OPDSDumper::handleBook(Book book, pugi::xml_node root_node) {
  auto entry_node = root_node.append_child("entry");
  ADD_TEXT_ENTRY(entry_node, "title", book.getTitle());
  ADD_TEXT_ENTRY(entry_node, "id", "urn:uuid:"+book.getId());
  ADD_TEXT_ENTRY(entry_node, "icon", rootLocation + "/meta?name=favicon&content=" + book.getHumanReadableIdFromPath());
  ADD_TEXT_ENTRY(entry_node, "updated", date);
  ADD_TEXT_ENTRY(entry_node, "summary", book.getDescription());

  auto content_node = entry_node.append_child("link");
  content_node.append_attribute("type") = "text/html";
  content_node.append_attribute("href") = (rootLocation + "/" + book.getHumanReadableIdFromPath()).c_str();

  auto author_node = entry_node.append_child("author");
  ADD_TEXT_ENTRY(author_node, "name", book.getCreator());

  if (! book.getUrl().empty()) {
    auto acquisition_link = entry_node.append_child("link");
    acquisition_link.append_attribute("rel") = "http://opds-spec.org/acquisition/open-access";
    acquisition_link.append_attribute("type") = "application/x-zim";
    acquisition_link.append_attribute("href") = book.getUrl().c_str();
    acquisition_link.append_attribute("length") = to_string(book.getSize()).c_str();
  }

  if (! book.getFaviconMimeType().empty() ) {
    auto image_link = entry_node.append_child("link");
    image_link.append_attribute("rel") = "http://opds-spec.org/image/thumbnail";
    image_link.append_attribute("type") = book.getFaviconMimeType().c_str();
    image_link.append_attribute("href") = (rootLocation + "/meta?name=favicon&content=" + book.getHumanReadableIdFromPath()).c_str();
  }
  return entry_node;
}

string OPDSDumper::dumpOPDSFeed(const std::vector<std::string>& bookIds)
{
  date = gen_date_str();
  pugi::xml_document doc;

  auto root_node = doc.append_child("feed");
  root_node.append_attribute("xmlns") = "http://www.w3.org/2005/Atom";
  root_node.append_attribute("xmlns:opds") = "http://opds-spec.org/2010/catalog";

  ADD_TEXT_ENTRY(root_node, "id", id);

  ADD_TEXT_ENTRY(root_node, "title", title);
  ADD_TEXT_ENTRY(root_node, "updated", date);

  if (m_isSearchResult) {
    ADD_TEXT_ENTRY(root_node, "totalResults", to_string(m_totalResults));
    ADD_TEXT_ENTRY(root_node, "startIndex", to_string(m_startIndex));
    ADD_TEXT_ENTRY(root_node, "itemsPerPage", to_string(m_count));
  }

  auto self_link_node = root_node.append_child("link");
  self_link_node.append_attribute("rel") = "self";
  self_link_node.append_attribute("href") = "";
  self_link_node.append_attribute("type") = "application/atom+xml";


  if (!searchDescriptionUrl.empty() ) {
    auto search_link = root_node.append_child("link");
    search_link.append_attribute("rel") = "search";
    search_link.append_attribute("type") = "application/opensearchdescription+xml";
    search_link.append_attribute("href") = searchDescriptionUrl.c_str();
  }

  if (library) {
    for (auto& bookId: bookIds) {
      handleBook(library->getBookById(bookId), root_node);
    }
  }

  return nodeToString(root_node);
}

}