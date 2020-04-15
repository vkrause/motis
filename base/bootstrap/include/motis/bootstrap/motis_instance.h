#pragma once

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "cista/memory_holder.h"

#include "boost/asio/io_service.hpp"

#include "motis/core/schedule/schedule.h"
#include "motis/module/controller.h"
#include "motis/module/message.h"
#include "motis/module/module.h"
#include "motis/module/remote.h"
#include "motis/loader/loader_options.h"

namespace motis::bootstrap {

struct motis_instance : public motis::module::controller {
  motis_instance();

  motis_instance(motis_instance const&) = delete;
  motis_instance& operator=(motis_instance const&) = delete;

  motis_instance(motis_instance&&) = delete;
  motis_instance& operator=(motis_instance&&) = delete;

  ~motis_instance() override = default;

  void stop_remotes();

  void on_remotes_registered(std::function<void()> fn) {
    on_remotes_registered_ = std::move(fn);
  }

  std::vector<motis::module::module*> modules() const;
  std::vector<std::string> module_names() const;

  void init_schedule(motis::loader::loader_options const& dataset_opt);
  void init_modules(std::vector<std::string> const& modules,
                    std::vector<std::string> const& exclude_modules = {},
                    unsigned num_threads = std::thread::hardware_concurrency());
  void init_remotes(
      std::vector<std::pair<std::string, std::string>> const& remotes);

  module::msg_ptr call(
      std::string const& target,
      unsigned num_threads = std::thread::hardware_concurrency());
  module::msg_ptr call(
      module::msg_ptr const&,
      unsigned num_threads = std::thread::hardware_concurrency());

  void publish(std::string const& target,
               unsigned num_threads = std::thread::hardware_concurrency());
  void publish(module::msg_ptr const&,
               unsigned num_threads = std::thread::hardware_concurrency());

  cista::memory_holder schedule_buf_;
  schedule_ptr schedule_;
  std::vector<std::shared_ptr<motis::module::remote>> remotes_;
  std::function<void()> on_remotes_registered_;
  unsigned connected_remotes_{0};
};

using motis_instance_ptr = std::unique_ptr<motis_instance>;

}  // namespace motis::bootstrap