#include "LocalTestPackage.h"

#define PrintFieldDelay 500

using std::cout;

int interpretSeverLog(Json::Value & orig)
{
	if (orig["log"].type() == Json::nullValue)
		return 1;
	Json::Value ret;
	//0 or 1
	int myTeam;
	auto dataSize = orig["log"].size() / 2;
	cout << "Local: Please enter team id: "; cin >> myTeam;
	ret["requests"][0] = orig["log"][0]["output"]["content"][to_string(myTeam)];
	for (auto i = 1u; i <= dataSize; i++)
	{
		ret["requests"][i] = orig["log"][2 * i - 1][to_string(1 - myTeam)]["response"];
	}
	for (auto i = 0u; i < dataSize; i++)
	{
		ret["responses"][i] = orig["log"][2 * i + 1][to_string(myTeam)]["response"];
	}

	//Json::FastWriter writer;cout << writer.write(ret);

	orig = ret;
	return 0;
}

int gameEngineWork()
{
	// init
	State curState[2];
	//int nextType[2] = { 0 };
	//int typeCount[2][7] = { 0 };
	int loser = -1;
	stateInit(Sample::gridInfo);
	printField(Sample::gridInfo);

	auto ais = new AI[2];

	// Game start
	while (true) {
		// 决策、放方块，检测是否有人挂了
		ais[0].negativeMaxSearch(curState[0], 4, -INF, INF, 0);
		ais[0].negativeMaxSearch(curState[1], 4, -INF, INF, 1);
		int tmp = blockForEnemy;
		if (!setAndJudge(curState[0].nextType, 0)) {
			cout << "Player 0 lose!" << endl;
			break;
		}
		ais[1].negativeMaxSearch(curState[0], 4, -INF, INF, 1);
		ais[1].negativeMaxSearch(curState[1], 4, -INF, INF, 0);
		if (!setAndJudge(curState[1].nextType, 1)) {
			cout << "Player 1 lose!" << endl;
			break;
		}

		// 每回合输出一次
		printField(Sample::gridInfo, PrintFieldDelay);

		curState[0].nextType= blockForEnemy;
		curState[1].nextType= tmp;

		// 检查消去
		Sample::eliminate(0);
		Sample::eliminate(1);

		// 进行转移
		loser = Sample::transfer();
		if (loser != -1) {
			cout << "Player " << loser << " lose!" << endl;
			break;
		}
	}

	// 终局图
	printField(Sample::gridInfo, PrintFieldDelay);
	// 输出一下分数
	cout << "Scores: " << Sample::score[0] << " vs " << Sample::score[1] << endl;

	return 0;
}

bool setAndJudge(int nextType, int role) {
	Sample::Tetris tmp(nextType, role);
	tmp.set(result);
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