#pragma once
#include <array>
#include "../../bus_types.h"

namespace nds{

    struct mat4x4{ // ROW MAJOR
        std::array<int32_t, 16> data = {};
        auto& operator()(const int i, const int j){
            return data[i * 4 + j];
        };
        const auto& operator()(const int i, const int j) const{
            return data[i * 4 + j];
        };

        mat4x4 operator*(const mat4x4 &b) const{
            mat4x4 m;
            for(int i = 0; i < 4; i++){
                for(int j = 0; j < 4; j++){
                    int64_t scalarProduct = 0;
                    for(int g = 0; g < 4; g++){
                        const auto e1 = (*this)(i,g);
                        const auto e2 = b(g,j);
                        scalarProduct += int64_t(e1) * e2;
                    }
                    scalarProduct >>= 12;
                    m(i,j) = scalarProduct;
                }
            }
            return m;
        }

        static mat4x4 identity(){
            mat4x4 m;
            for(int i = 0; i < 4; i++){
                m(i,i) = 1u << 12;
            }
            return m;
        }

    };


}
