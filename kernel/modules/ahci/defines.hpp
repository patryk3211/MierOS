#pragma once

#include <types.h>

#define COMMAND_TABLE_PAGES 2
constexpr size_t max_prdt_size = (COMMAND_TABLE_PAGES*4096-128)/16;
