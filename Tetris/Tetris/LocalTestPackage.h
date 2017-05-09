#pragma once
#include <iostream>
#include <string>

#include "jsoncpp/json.h"

#define MAPHEIGHT 20
#define MAPWIDTH 10

using namespace std;

struct Block;


//abstruct input data from server log
int interpretSeverLog(Json::Value& orig);

namespace Sample
{
	int sampleStrategy(
		const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2], int(&typeCount)[2][7],
		const int nextBlockType, int depth, int alpha, int beta, int role);
}

int negativeMaxSearch(
	const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2],int(&typeCount)[2][7],
	const int nextBlockType, int depth, int alpha, int beta, int role);

int gameEngineWork();