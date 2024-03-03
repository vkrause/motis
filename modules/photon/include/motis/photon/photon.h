// SPDX-FileCopyrightText: 2024 Volker Krause <vkrause@kde.org>
// SPDX-License-Identifier: MIT

#pragma once

#include "motis/module/module.h"

namespace motis::photon {

struct photon : public motis::module::module {
  photon();
  ~photon() override;

  void init(motis::module::registry&) override;

private:
  struct impl;
  std::unique_ptr<impl> impl_;

  std::string url_;
};

}  // namespace motis::photon
