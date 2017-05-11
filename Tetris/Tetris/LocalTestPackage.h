#pragma once
#include <iostream>
#include <algorithm>
#include <cstring>
#include <string>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <thread>

#include "jsoncpp/json.h"

#define MAPHEIGHT 20
#define MAPWIDTH 10
#define INF 999999999 //TODO modify max scores
#define sleep(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms))

using namespace std;

// 7种形状(长L| 短L| 反z| 正z| T| 直一| 田格)，4种朝向(上左下右)，8:每相邻的两个分别为x，y
const int blockShape[7][4][8] = {
	{ { 0,0,1,0,-1,0,-1,-1 },{ 0,0,0,1,0,-1,1,-1 },{ 0,0,-1,0,1,0,1,1 },{ 0,0,0,-1,0,1,-1,1 } },
	{ { 0,0,-1,0,1,0,1,-1 },{ 0,0,0,-1,0,1,1,1 },{ 0,0,1,0,-1,0,-1,1 },{ 0,0,0,1,0,-1,-1,-1 } },
	{ { 0,0,1,0,0,-1,-1,-1 },{ 0,0,0,1,1,0,1,-1 },{ 0,0,-1,0,0,1,1,1 },{ 0,0,0,-1,-1,0,-1,1 } },
	{ { 0,0,-1,0,0,-1,1,-1 },{ 0,0,0,-1,1,0,1,1 },{ 0,0,1,0,0,1,-1,1 },{ 0,0,0,1,-1,0,-1,-1 } },
	{ { 0,0,-1,0,0,1,1,0 },{ 0,0,0,-1,-1,0,0,1 },{ 0,0,1,0,0,-1,-1,0 },{ 0,0,0,1,1,0,0,-1 } },
	{ { 0,0,0,-1,0,1,0,2 },{ 0,0,1,0,-1,0,-2,0 },{ 0,0,0,1,0,-1,0,-2 },{ 0,0,-1,0,1,0,2,0 } },
	{ { 0,0,0,1,-1,0,-1,1 },{ 0,0,-1,0,0,-1,-1,-1 },{ 0,0,0,-1,1,-0,1,-1 },{ 0,0,1,0,0,1,1,1 } }
};

// 一次性消去行数对应分数
const int elimBonus[] = { 0, 1, 3, 5, 7 };

/****************************************************************************************************/
/******************************                                        ******************************/
/******************************               declaration              ******************************/
/******************************                                        ******************************/
/****************************************************************************************************/

struct Block;

extern int MODE;

extern int blockForEnemy;

extern Block result;

struct Block
{
	int x, y, o;

	Block() {}
	Block(const int &x, const int &y, const int &o);

	Block(const Json::Value& jv);

	operator Json::Value()const;
};

namespace Sample
{
	extern int gridInfo[2][MAPHEIGHT + 2][MAPWIDTH + 2];

	extern int trans[2][4][MAPWIDTH + 2];

	extern int transCount[2];

	extern int maxHeight[2];

	extern int score[2];

	class Tetris
	{
	public:
		const int blockType;   // 标记方块类型的序号 0~6
		Block block;
		const int(*shape)[8]; // 当前类型方块的形状定义

		int color;

		Tetris(int t, int color) : blockType(t), shape(blockShape[t]), color(color) {}

		Tetris &set(int x = -1, int y = -1, int o = -1);

		Tetris &set(const Block& _block = { -1,-1,-1 });

		// 判断当前位置是否合法
		bool isValid(int x = -1, int y = -1, int o = -1);

		// 判断是否落地
		bool onGround();

		// 将方块放置在场地上
		bool place();

		// 检查能否逆时针旋转自己到o
		bool rotation(int o);
	};

	inline bool checkDirectDropTo(int color, int blockType, int x, int y, int o);

	void eliminate(int color);

	int transfer();

	inline bool canPut(int color, int blockType);

	void printField(const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2], int delayMs = 0, bool clean = true);

	int sampleStrategy(
		const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2], int(&typeCount)[2][7],
		const int nextBlockType, int depth, int alpha, int beta, int role);
}

//abstruct input data from server log
int interpretSeverLog(Json::Value& orig);

void stateInit(int(&grid)[2][MAPHEIGHT + 2][MAPWIDTH + 2]);

int negativeMaxSearch(
	const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2], int(&typeCount)[2][7],
	const int nextBlockType, int depth, int alpha, int beta, int role);

bool setAndJudge(int, int);

int gameEngineWork();
