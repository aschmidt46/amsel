#pragma once

struct Divider{
    unsigned int period = 0;
    int counter = 0;
    unsigned int divideBy = 1;
    int dbCount = divideBy;
    Divider() = default;
    Divider(unsigned int p, unsigned int db = 1);

    bool clock();
    void reset();
    void changePeriod(unsigned int p);
};