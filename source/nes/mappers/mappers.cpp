#include "mappers.h"

std::optional<std::unique_ptr<AbstractMapper>> getMapper(std::shared_ptr<Mapper> m, int number)
{
    switch(number){
        case 0:
            return {std::make_unique<Mapper0>(m)};
            break;
        case 1:
            return {std::make_unique<Mapper1>(m)};
            break;
        case 2:
            return {std::make_unique<Mapper2>(m)};
            break;
        case 3:
            return {std::make_unique<Mapper3>(m)};
            break;
        case 4:
            return {std::make_unique<Mapper4>(m)};
            break;
        case 7:
            return {std::make_unique<Mapper7>(m)};
            break;
        default:
            return std::nullopt;
    }
}
