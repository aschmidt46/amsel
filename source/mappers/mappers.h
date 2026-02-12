#pragma once

#include "mapper0.h"
#include "mapper1.h"
#include "mapper2.h"
#include "mapper3.h"
#include "mapper4.h"
#include "mapper7.h"
#include <optional>
#include <memory>
#include <vector>


class Mapper;

std::optional<std::shared_ptr<AbstractMapper>> getMapper(std::shared_ptr<Mapper> m, int number);
