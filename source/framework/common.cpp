#include "common.h"

void setDefaultBindings(SettingsConfig &config)
{
    config.controller1 = std::vector<std::pair<int, int>>(8, {0, 0});
    config.controller2 = std::vector<std::pair<int, int>>(8, {0, 0});
    // Standard Belegung Controller 1
    config.controller1[0].first = 265;
    config.controller1[1].first = 264;
    config.controller1[2].first = 263;
    config.controller1[3].first = 262;
    config.controller1[4].first = 83;
    config.controller1[5].first = 65;
    config.controller1[6].first = 257;
    config.controller1[7].first = 259;

    config.controller1[0].second = 23;
    config.controller1[1].second = 25;
    config.controller1[2].second = 26;
    config.controller1[3].second = 24;
    config.controller1[4].second = 12;
    config.controller1[5].second = 14;
    config.controller1[6].second = 19;
    config.controller1[7].second = 18;

    // Standard Belegung Controller 2
    config.controller2[0].first = 328;
    config.controller2[1].first = 325;
    config.controller2[2].first = 324;
    config.controller2[3].first = 326;
    config.controller2[4].first = 88;
    config.controller2[5].first = 90;
    config.controller2[6].first = 335;
    config.controller2[7].first = 334;

    config.controller2[0].second = 23;
    config.controller2[1].second = 25;
    config.controller2[2].second = 26;
    config.controller2[3].second = 24;
    config.controller2[4].second = 12;
    config.controller2[5].second = 14;
    config.controller2[6].second = 19;
    config.controller2[7].second = 18;
}
