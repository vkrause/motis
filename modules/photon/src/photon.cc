// SPDX-FileCopyrightText: 2024 Volker Krause <vkrause@kde.org>
// SPDX-License-Identifier: MIT

#include "motis/photon/photon.h"

#include <boost/url/encode.hpp>
#include <boost/url/rfc/unreserved_chars.hpp>

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "utl/to_vec.h"

#include "motis/core/common/logging.h"
#include "motis/module/context/motis_http_req.h"
#include "motis/module/event_collector.h"
#include "motis/module/ini_io.h"
#include "motis/json/json.h"

using namespace motis::address;
using namespace motis::module;
using namespace motis::logging;

namespace motis::photon {

struct photon::impl {
  explicit impl(std::string const& url) : url_(url) {}

  msg_ptr get_guesses(msg_ptr const& msg) const {
    LOG(debug) << "address query "
               << motis_content(AddressRequest, msg)->input()->str();
    auto const req =
        url_ +
        "?layer=city&layer=street&layer=house&layer=locality&layer=district&"
        "q=" +
        boost::urls::encode(motis_content(AddressRequest, msg)->input()->str(),
                            boost::urls::unreserved_chars);
    auto f = motis_http(req);
    auto v = f->val();
    LOG(debug) << "photon reply " << v.body;

    rapidjson::Document doc;
    if (doc.Parse(v.body.data(), v.body.size()).HasParseError()) {
      doc.GetParseError();
      throw utl::fail("Photon response: Bad JSON: {} at offset {}",
                      rapidjson::GetParseError_En(doc.GetParseError()),
                      doc.GetErrorOffset());
    }
    auto const array = json::get_array(doc, "features");

    message_creator fbb;
    fbb.create_and_finish(
        MsgContent_AddressResponse,
        CreateAddressResponse(
            fbb,
            fbb.CreateVector(utl::to_vec(
                array,
                [&](auto const& obj) {
                  auto const& geometry = json::get_obj(obj, "geometry");
                  auto const& coordinates =
                      json::get_array(geometry, "coordinates");
                  auto const pos = Position{coordinates[1].GetDouble(),
                                            coordinates[0].GetDouble()};
                  auto const& properties = json::get_obj(obj, "properties");
                  auto const& type = json::get_str(properties, "type");

                  struct {
                    const char* key;
                    uint32_t level;
                  } constexpr const region_level_table[]{
                      {"country", 2},
                      {"state", 4},
                      {"county", 6},
                      {"city", 8},
                  };

                  std::vector<std::pair<std::string_view, uint32_t>> regions;
                  for (auto const& key : region_level_table) {
                    auto const member = properties.FindMember(key.key);
                    if (member != properties.MemberEnd() &&
                        member->value.IsString()) {
                      regions.push_back(
                          std::make_pair(member->value.GetString(), key.level));
                    }
                  }

                  return CreateAddress(
                      fbb, &pos,
                      fbb.CreateString(json::get_str(properties, "name")),
                      fbb.CreateString(type == "city"     ? "place"
                                       : type == "street" ? "street"
                                                          : "unknown"),
                      fbb.CreateVector(utl::to_vec(
                          regions, [&](std::pair<std::string_view,
                                                 uint32_t> const& region) {
                            return CreateRegion(fbb,
                                                fbb.CreateString(region.first),
                                                region.second);
                          })));
                })))
            .Union());
    return make_msg(fbb);
  }

  std::string const& url_;
};

photon::photon() : module("Address Typeahead", "photon") {
  param(url_, "url", "Photon API endpoint");
}

photon::~photon() = default;

void photon::init(motis::module::registry& reg) {
  impl_ = std::make_unique<impl>(url_);
  reg.register_op(
      "/address",
      [this](msg_ptr const& msg) { return impl_->get_guesses(msg); }, {});
}

}  // namespace motis::photon
