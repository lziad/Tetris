#pragma once
#include <iostream>
#include <string>

#include "jsoncpp/json.h"

#define MAPHEIGHT 20
#define MAPWIDTH 10
#define INF 999999999

using namespace std;


extern int blockForEnemy;

extern struct Block result;


//abstruct input data from server log
int interpretSeverLog(Json::Value& orig);

void stateInit(int(&grid)[2][MAPHEIGHT + 2][MAPWIDTH + 2]);

namespace Sample
{
    
    extern int gridInfo[2][MAPHEIGHT + 2][MAPWIDTH + 2];
    
    extern int trans[2][4][MAPWIDTH + 2];
    
    extern int transCount[2];
    
    extern int maxHeight[2];
    
    extern int score[2];
    
    class Tetris;
    
    bool checkDirectDropTo(int color, int blockType, int x, int y, int o);
    
    void eliminate(int color);
    
    int transfer();
    
    inline bool canPut(int color, int blockType);
    
    void printField(const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2]);
    
    int sampleStrategy(
        const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2], int(&typeCount)[2][7],
        const int nextBlockType, int depth, int alpha, int beta, int role);


int negativeMaxSearch(
	const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2],int(&typeCount)[2][7],
	const int nextBlockType, int depth, int alpha, int beta, int role);

bool setAndJudge(int, int);

int gameEngineWork();
