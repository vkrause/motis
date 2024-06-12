#include "icc/endpoints/osr_routing.h"

#include "utl/pipes.h"

#include "osr/geojson.h"
#include "osr/routing/route.h"

namespace json = boost::json;

namespace icc::ep {

osr::location parse_location(json::value const& v) {
  auto const& obj = v.as_object();
  return {obj.at("lat").as_double(), obj.at("lng").as_double(),
          obj.contains("level")
              ? osr::to_level(obj.at("level").to_number<float>())
              : osr::level_t::invalid()};
}

json::value osr_routing::operator()(json::value const& query) const {
  auto const& q = query.as_object();
  auto const profile_it = q.find("profile");
  auto const profile =
      osr::to_profile(profile_it == q.end() || !profile_it->value().is_string()
                          ? to_str(osr::search_profile::kFoot)
                          : profile_it->value().as_string());
  auto const direction_it = q.find("direction");
  auto const dir = osr::to_direction(direction_it == q.end() ||
                                             !direction_it->value().is_string()
                                         ? to_str(osr::direction::kForward)
                                         : direction_it->value().as_string());
  auto const from = parse_location(q.at("start"));
  auto const to = parse_location(q.at("destination"));
  auto const max_it = q.find("max");
  auto const max = static_cast<osr::cost_t>(
      max_it == q.end() ? 3600 : max_it->value().as_int64());
  auto const blocked = blocked_;
  auto const p = route(w_, l_, profile, from, to, max, dir, 8, blocked.get());
  return p.has_value()
             ? json::value{{"type", "FeatureCollection"},
                           {"features",
                            utl::all(p->segments_)  //
                                |
                                utl::transform([&](auto&& s) {
                                  return json::value{
                                      {"type", "Feature"},
                                      {"properties",
                                       {{"level", to_float(s.from_level_)},
                                        {"way",
                                         s.way_ == osr::way_idx_t::invalid()
                                             ? 0U
                                             : to_idx(
                                                   w_.way_osm_idx_[s.way_])}}},
                                      {"geometry",
                                       osr::to_line_string(s.polyline_)}};
                                })  //
                                | utl::emplace_back_to<json::array>()}}
             : json::value{{"error", "no path found"}};
}

}  // namespace icc::ep