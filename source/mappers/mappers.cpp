#include "mappers.h"

std::optional<AbstractMapper *> getMapper(std::shared_ptr<Mapper> m, int number)
{
    switch(number){
        case 0:
            return {(AbstractMapper*) new Mapper0(m)};
            break;
        case 1:
            return {(AbstractMapper*) new Mapper1(m)};
            break;
        case 2:
            return {(AbstractMapper*) new Mapper2(m)};
            break;
        case 3:
            return {(AbstractMapper*) new Mapper3(m)};
            break;
        default:
            return std::nullopt;
    }
}