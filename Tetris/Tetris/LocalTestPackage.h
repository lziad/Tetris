#pragma once
#include <iostream>
#include <string>

#include "jsoncpp/json.h"

using namespace std;

//abstruct input data from server log
int interpretSeverLog(Json::Value& orig);