#pragma once
#include <iostream>

#include <cstring>
#include <string>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <functional>

#include <algorithm>
#include <map>
#include <unordered_map>

#include "jsoncpp/json.h"

#define MAPHEIGHT 20
#define MAPWIDTH 10
#define INF 999999999 //TODO modify max scores
#define DEPTH 4
#define LostValue -1000
#define ULL unsigned long long
#define sleep(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms))
#define DefaultMode 2

using namespace std;
using namespace std::placeholders;

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

struct State;

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

		bool isValid(const State &curState, int x, int y, int o);

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

	inline bool canContinue(int color, int blockType);

	int sampleStrategy(
		const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2], int(&typeCount)[2][7],
		const int nextBlockType, int depth, int alpha, int beta, int role);
}

//abstruct input data from server log
int interpretSeverLog(Json::Value& orig);

void stateInit(int(&grids)[2][MAPHEIGHT + 2][MAPWIDTH + 2]);


bool setAndJudge(int, int);

int gameEngineWork();

bool canReach(int color, int blockType, int x, int y, int o);

int evaluate(const State &state, int role);

void printField(const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2], int delayMs = 0, bool clean = true);

struct Int256
{
	ULL data[4];
	Int256() :data{ 0,0,0,0 } {}
};

//template<class T>struct std::hash;
// 20*10 + 7*2 + 3 = 217 bits in total

bool operator== (const State &a, const State &b);

struct State
{
	friend struct std::hash<State>;
	friend bool operator== (const State &a, const State &b);

	bool grids[20][10];
	//TODO simplized to 0 1 2 3 (normally won't exceed 2)
	short typeCount[7];
	short nextType;
	State() {}
	State(const int(&_grid)[MAPHEIGHT][MAPWIDTH],
		const int(&_typeCount)[7], const int nextType);
	operator Int256()const;
	void init();

};

namespace std
{
	template<>
	struct hash<State>
	{
		typedef unsigned result_type;
		typedef State argument_type;
		unsigned operator()(const State &state)const;

	};
}

struct AI
{
	unordered_map<State, int> mp;

	Block bestChoice;

	void GenerateStrategy(const State(&states)[2]);

	int negativeMaxSearch(const State &curState, int depth, int alpha, int beta, int role);

	struct StateInfo
	{
		State state;
		Block choice;
		int score;
		int id;
	};

	struct IndexCmp
	{
		const StateInfo(&info)[180];
		IndexCmp(const StateInfo(&info)[180]) :info(info) {}
		bool operator()(const int a, const int b)const;
	};

	void GenerateAllPossibleMove(const State &curState, StateInfo *info, int &totInfo, int role);


};

