#include "mappers.h"

std::optional<std::shared_ptr<AbstractMapper>> getMapper(std::shared_ptr<Mapper> m, int number)
{
    switch(number){
        case 0:
            return {std::shared_ptr<AbstractMapper>(reinterpret_cast<AbstractMapper*>(new Mapper0(m)))};
            break;
        case 1:
            return {std::shared_ptr<AbstractMapper>(reinterpret_cast<AbstractMapper*>(new Mapper1(m)))};
            break;
        case 2:
            return {std::shared_ptr<AbstractMapper>(reinterpret_cast<AbstractMapper*>(new Mapper2(m)))};
            break;
        case 3:
            return {std::shared_ptr<AbstractMapper>(reinterpret_cast<AbstractMapper*>(new Mapper3(m)))};
            break;
        case 4:
            return {std::shared_ptr<AbstractMapper>(reinterpret_cast<AbstractMapper*>(new Mapper4(m)))};
            break;
        case 7:
            return {std::shared_ptr<AbstractMapper>(reinterpret_cast<AbstractMapper*>(new Mapper7(m)))};
            break;
        default:
            return std::nullopt;
    }
}