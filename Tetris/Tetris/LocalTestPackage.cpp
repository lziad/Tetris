#include "LocalTestPackage.h"

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
    int nextType[2] = { 0 };
    int typeCount[2][7] = { 0 };
    int loser = -1;
    stateInit(Sample::gridInfo);
	Sample::printField(Sample::gridInfo);
    
    // Game start
    while (true) {
        // 决策、放方块，检测是否有人挂了
        Sample::sampleStrategy(Sample::gridInfo, typeCount, nextType[0], 1, -INF, INF, 0);
        int tmp = blockForEnemy;
        if (!setAndJudge(nextType[0], 0)) {
            cout << "Player 0 lose!" << endl;
            break;
        }
        
        Sample::sampleStrategy(Sample::gridInfo, typeCount, nextType[1], 1, -INF, INF, 1);
        if (!setAndJudge(nextType[1], 1)) {
            cout << "Player 1 lose!" << endl;
            break;
        }
        
        // 每回合输出一次
        Sample::printField(Sample::gridInfo);
        
        nextType[0] = blockForEnemy;
        nextType[1] = tmp;
        
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
    Sample::printField(Sample::gridInfo);
    // 输出一下分数
    cout << "Scores: " << Sample::score[0] << " vs " << Sample::score[1] << endl;
    
	return 0;
}
