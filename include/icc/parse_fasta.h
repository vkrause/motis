#pragma once

#include <filesystem>
#include <string_view>

#include "geo/latlng.h"

#include "utl/enumerate.h"

#include "osr/lookup.h"
#include "osr/ways.h"

#include "boost/json.hpp"

#include "icc/types.h"

namespace icc {

nigiri::vector_map<elevator_idx_t, elevator> parse_fasta(std::string_view s) {
  auto ret = nigiri::vector_map<elevator_idx_t, elevator>{};
  for (auto const& [i, e] : utl::enumerate(boost::json::parse(s).as_array())) {
    if (e.at("type") != "ELEVATOR") {
      continue;
    }

    try {
      auto const& o = e.as_object();

      if (!o.contains("geocoordY") || !o.contains("geocoordX") ||
          !o.contains("state")) {
        std::cout << "skip: missing attributes: " << o << "\n";
        continue;
      }

      auto const id = o.contains("equipmentnumber")
                          ? e.at("equipmentnumber").to_number<std::int64_t>()
                          : 0U;
      ret.emplace_back(elevator{
          id,
          {e.at("geocoordY").to_number<double>(),
           e.at("geocoordX").to_number<double>()},
          e.at("state") == "INACTIVE" ? status::kInactive : status::kActive,
          o.contains("description")
              ? std::string{o.at("description").as_string()}
              : ""});
    } catch (std::exception const& ex) {
      std::cout << "error on value: " << e << ": " << ex.what() << "\n";
    }
  }
  return ret;
}

nigiri::vector_map<elevator_idx_t, elevator> parse_fasta(
    std::filesystem::path const& p) {
  return parse_fasta(
      cista::mmap{p.generic_string().c_str(), cista::mmap::protection::READ}
          .view());
}

}  // namespace icc