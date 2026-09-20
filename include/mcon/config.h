// Copyright (c) 2026 Håkon F. Fjellanger
// All rights reserved. See LICENSE file for details.

#ifndef MCON_CONFIG
#define MCON_CONFIG

#include "mcon/types.h"

extern const struct mcon_config mcon_config_default;

/// @brief Check whether a give configuration will pass as
/// a valid configuration if passed to mcon_create().
/// @param config The configuration to validate.
/// @return If the configuration is valid, returns true. Otherwise, returns false.
bool mcon_config_validate(const struct mcon_config config);

#endif // MCON_CONFIG
