﻿#include "LocalTestPackage.h"

using std::cout;

int interpretSeverLog(Json::Value & orig)
{
	if (orig.type() == Json::nullValue)
		return 1;
	Json::Value ret;
	//0 or 1
	int myTeam;
	auto dataSize = orig.size() / 2;
	cout << "Local: Please enter team id: "; cin >> myTeam;
	ret["requests"][0] = orig[0]["output"]["content"][to_string(myTeam)];
	for (auto i = 1u; i <= dataSize; i++)
	{
		ret["requests"][i] = orig[2 * i - 1][to_string(1 - myTeam)]["response"];
	}
	for (auto i = 0u; i < dataSize; i++)
	{
		ret["responses"][i] = orig[2 * i + 1][to_string(myTeam)]["response"];
	}

	//Json::FastWriter writer;cout << writer.write(ret);

	orig = ret;
	return 0;
}

int gameEngineWork()
{
	// init
	State curState[2];
	for (auto &i : curState) 
	{
		i.init();
		i.nextType = rand() % 7;
	}
	int loser = -1;
	stateInit(Sample::gridInfo);

	auto ais = new AI[2];

	// Game start
	while (true) {
		// 决策、放方块，检测是否有人挂了
        ais[0].GenerateStrategy(curState[0], curState[1], 2);
		gridsTransfer(Sample::gridInfo[0], curState[0]);
		if (!setAndJudge(ais[0].bestChoice, curState[0], 0)) {
			cout << "Player 0 lose!" << endl;
			break;
		}
		gridsTransfer(curState[0], Sample::gridInfo[0]);
		ais[1].GenerateStrategy(curState[1], curState[0], 2);
		gridsTransfer(Sample::gridInfo[1], curState[1]);
		if (!setAndJudge(ais[1].bestChoice, curState[1], 1)) {
			cout << "Player 1 lose!" << endl;
			break;
		}
		gridsTransfer(curState[1], Sample::gridInfo[1]);

		// 每回合输出一次
		printField(Sample::gridInfo, PrintFieldDelay);

		curState[0].nextType = ais[1].blockForEnemy;
		curState[1].nextType = ais[0].blockForEnemy;

		// 检查消去
		Sample::eliminate(0);
		Sample::eliminate(1);

		// 进行转移
		loser = Sample::transfer();
		if (loser != -1) {
			cout << "Player " << loser << " lose!" << endl;
			break;
		}
        
        gridsTransfer(curState[0], Sample::gridInfo[0]);
        gridsTransfer(curState[1], Sample::gridInfo[1]);
	}

	// 终局图
    gridsTransfer(Sample::gridInfo[0], curState[0]);
    gridsTransfer(Sample::gridInfo[1], curState[1]);
	printField(Sample::gridInfo, PrintFieldDelay);
	// 输出一下分数
	cout << "Scores: " << Sample::score[0] << " vs " << Sample::score[1] << endl;

	return 0;
}

bool setAndJudge(const Block &choice, State &state, int role) {
    Block mdzz = Block(choice.x+1, choice.y+1, choice.o);
	Sample::Tetris tmp(state.nextType, role);
	tmp.set(mdzz);
	state.typeCount[state.nextType]++;
	return tmp.place();
}

// 打印场地用于调试
void printField(const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2], int delayMs, bool clean)
{
	if (delayMs)
		sleep(delayMs);
	if (clean)
		system("cls");

	static const char *i2s[] = { "~~","~~","  ","[]","{}" };

	cout << endl;
	for (int y = MAPHEIGHT + 1; y >= 0; y--)
	{
		for (int x = 0; x <= MAPWIDTH + 1; x++)
			cout << i2s[gridInfo[0][y][x] + 2];
		for (int x = 0; x <= MAPWIDTH + 1; x++)
			cout << i2s[gridInfo[1][y][x] + 2];
		cout << endl;
	}
	cout << endl;

	//cout << "（图例： ~~：墙，[]：块，{}：新块）" << endl << endl;
}

void printField(const State(&state)[2], int delayMs, bool clean)
{
	if (delayMs)
		sleep(delayMs);
	if (clean)
		system("cls");

	static const char *i2s[] = { "~~","~~","  ","[]","{}" };

	cout << endl;
	for (int y = MAPHEIGHT; y >= 0; y--)
	{
		for (int x = 0; x <= MAPWIDTH; x++)
			cout << i2s[state[0].grids[y][x] + 2];
		for (int x = 0; x <= MAPWIDTH; x++)
			cout << i2s[state[1].grids[y][x] + 2];
		cout << endl;
	}
	cout << endl;
	//cout << "（图例： ~~：墙，[]：块，{}：新块）" << endl << endl;
}

void gridsTransfer(int(&dst)[MAPHEIGHT + 2][MAPWIDTH + 2], const State &state)
{
	for (int i = 0; i < MAPHEIGHT; i++)
		for (int j = 0; j < MAPWIDTH; j++)
		{
			dst[i + 1][j + 1] = state.grids[i][j];
		}
}

void gridsTransfer(State &state, const int(&dst)[MAPHEIGHT + 2][MAPWIDTH + 2])
{
	for (int i = 0; i < MAPHEIGHT; i++)
		for (int j = 0; j < MAPWIDTH; j++)
		{
			state.grids[i][j] = dst[i + 1][j + 1];
		}
}
