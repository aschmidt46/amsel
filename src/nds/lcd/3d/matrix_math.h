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

    struct vec4{
        int32_t x = 0, y = 0, z = 0, w = 0x1000;

        vec4(){};
        vec4(const int x, const int y, const int z) : x(x), y(y), z(z){};
        vec4(const int x, const int y, const int z, const int w) : x(x), y(y), z(z), w(w){};

        vec4 operator*(const mat4x4 &rhs) const{
            vec4 v;
            
            int64_t lx = int64_t(x) * rhs(0,0) + int64_t(y) * rhs(1,0) + int64_t(z) * rhs(2,0) + int64_t(w) * rhs(3,0);
            int64_t ly = int64_t(x) * rhs(0,1) + int64_t(y) * rhs(1,1) + int64_t(z) * rhs(2,1) + int64_t(w) * rhs(3,1);
            int64_t lz = int64_t(x) * rhs(0,2) + int64_t(y) * rhs(1,2) + int64_t(z) * rhs(2,2) + int64_t(w) * rhs(3,2);
            int64_t lw = int64_t(x) * rhs(0,3) + int64_t(y) * rhs(1,3) + int64_t(z) * rhs(2,3) + int64_t(w) * rhs(3,3);

            v.x = lx >> 12; v.y = ly >> 12; v.z = lz >> 12; v.w = lw >> 12;

            return v;
        }
    };

}
