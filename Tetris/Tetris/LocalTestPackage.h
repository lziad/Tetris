#pragma once
#include <iostream>
#include <string>

#include "jsoncpp/json.h"

#define MAPHEIGHT 20
#define MAPWIDTH 10
#define INF 999999999

using namespace std;

//abstruct input data from server log
int interpretSeverLog(Json::Value& orig);

void stateInit(int(&grid)[2][MAPHEIGHT + 2][MAPWIDTH + 2]);
/*
namespace AI
{
	int sampleStrategy(
		const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2], int(&typeCount)[2][7],
		const int nextBlockType, int depth, int alpha, int beta, int role);

    void printField(const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2]);
    
    bool checkDirectDropTo(int color, int blockType, int x, int y, int o);
}
*/
int negativeMaxSearch(
	const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2],
	const int nextBlockType, int depth, int alpha, int beta, int role);

int gameEngineWork();
