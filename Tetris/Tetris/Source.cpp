




/****************************************************************************************************/
/******************************                                        ******************************/
/******************************               Source File              ******************************/
/******************************                                        ******************************/
/****************************************************************************************************/


/*
 * https://wiki.botzone.org/index.php?title=Tetris
 * 注意：x的范围是1~MAPWIDTH，y的范围是1~MAPHEIGHT
 * 数组是先行（y）后列（c）
 * 坐标系：原点在左下角
 * 平台保证所有输入都是合法输入
 */

#ifndef _BOTZONE_ONLINE
#include "LocalTestPackage.h"
#endif // !_BOTZONE_ONLINE

 /* 0     botzone.org			*/
 /* 1     simulate botzone.org	*/
 /* 2     Host local game		*/
int MODE;

int blockForEnemy;

Block result;

Block::Block(const int &x, const int &y, const int &o)
		:x(x), y(y), o(o) {}

Block::Block(const Json::Value& jv)
	{
		x = jv["x"].asInt();
		y = jv["y"].asInt();
		o = jv["o"].asInt();
	}

Block::operator Json::Value()const
	{
		Json::Value jv;
		jv["x"] = x;
		jv["y"] = y;
		jv["o"] = o;
		return jv;
	}

namespace Sample
{
	/* 先y后x，记录地图状态，0为空，1为以前放置，2为刚刚放置，负数为越界	  */
	/* （2用于在清行后将最后一步撤销再送给对方）						  */
	int gridInfo[2][MAPHEIGHT + 2][MAPWIDTH + 2] = { 0 };

	// 代表分别向对方转移的行
	int trans[2][4][MAPWIDTH + 2] = { 0 };
	// 转移行数
	int transCount[2] = { 0 };
	// 运行eliminate后的当前高度
	int maxHeight[2] = { 0 };
	// 双方分数，用于平局时判断
	int score[2] = { 0 };

	Tetris &Tetris::set(int x, int y, int o)
	{

		block.x = x == -1 ? block.x : x;
		block.y = y == -1 ? block.y : y;
		block.o = o == -1 ? block.o : o;
		return *this;
	}

	Tetris &Tetris::set(const Block& _block)
	{

		block.x = _block.x == -1 ? block.x : _block.x;
		block.y = _block.y == -1 ? block.y : _block.y;
		block.o = _block.o == -1 ? block.o : _block.o;
		return *this;
	}

	// 判断当前位置是否合法
	inline bool Tetris::isValid(int x, int y, int o)
	{

		x = x == -1 ? block.x : x;
		y = y == -1 ? block.y : y;
		o = o == -1 ? block.o : o;
		if (o < 0 || o > 3)
			return false;

		int i, tmpX, tmpY;
		for (i = 0; i < 4; i++)
		{
			tmpX = x + shape[o][2 * i];
			tmpY = y + shape[o][2 * i + 1];
			if (tmpX < 1 || tmpX > MAPWIDTH ||
				tmpY < 1 || tmpY > MAPHEIGHT ||
				Sample::gridInfo[color][tmpY][tmpX] != 0)
				return false;
		}
		return true;
	}

	// 判断是否落地
	bool Tetris::onGround()
	{
		if (isValid() && !isValid(-1, block.y - 1))
			return true;
		return false;
	}

	// 将方块放置在场地上
	bool Tetris::place()
	{
		if (!onGround())
			return false;

		int i, tmpX, tmpY;
		for (i = 0; i < 4; i++)
		{
			tmpX = block.x + shape[block.o][2 * i];
			tmpY = block.y + shape[block.o][2 * i + 1];
			Sample::gridInfo[color][tmpY][tmpX] = 2;
		}
		return true;
	}

	// 检查能否逆时针旋转自己到o
	bool Tetris::rotation(int o)
	{
		if (o < 0 || o > 3)
			return false;

		if (block.o == o)
			return true;

		int fromO = block.o;
		while (true)
		{
			if (!isValid(-1, -1, fromO))
				return false;

			if (fromO == o)
				break;

			fromO = (fromO + 1) % 4;
		}
		return true;
	}

	// 检查能否从场地顶端直接落到当前位置
	inline bool checkDirectDropTo(int color, int blockType, int x, int y, int o)
	{
		auto &def = blockShape[blockType][o];
		for (; y <= MAPHEIGHT; y++)
			for (int i = 0; i < 4; i++)
			{
				int _x = def[i * 2] + x, _y = def[i * 2 + 1] + y;
				if (_y > MAPHEIGHT)
					continue;
				if (_y < 1 || _x < 1 || _x > MAPWIDTH || Sample::gridInfo[color][_y][_x])
					return false;
			}
		return true;
	}
	// 消去行
	void eliminate(int color)
	{
		int &count = Sample::transCount[color] = 0;
		int i, j, emptyFlag, fullFlag;
		Sample::maxHeight[color] = MAPHEIGHT;
		for (i = 1; i <= MAPHEIGHT; i++)
		{
			emptyFlag = 1;
			fullFlag = 1;
			for (j = 1; j <= MAPWIDTH; j++)
			{
				if (Sample::gridInfo[color][i][j] == 0)
					fullFlag = 0;
				else
					emptyFlag = 0;
			}
			if (fullFlag)
			{
				for (j = 1; j <= MAPWIDTH; j++)
				{
					// 注意这里只转移以前的块，不包括最后一次落下的块（“撤销最后一步”）
					Sample::trans[color][count][j] = Sample::gridInfo[color][i][j] == 1 ? 1 : 0;
					Sample::gridInfo[color][i][j] = 0;
				}
				count++;
			}
			else if (emptyFlag)
			{
				Sample::maxHeight[color] = i - 1;
				break;
			}
			else
				for (j = 1; j <= MAPWIDTH; j++)
				{
					Sample::gridInfo[color][i - count][j] =
						Sample::gridInfo[color][i][j] > 0 ? 1 : Sample::gridInfo[color][i][j];
					if (count)
						Sample::gridInfo[color][i][j] = 0;
				}
		}
		Sample::maxHeight[color] -= count;
		Sample::score[color] += elimBonus[count];
	}

	// 转移双方消去的行，返回-1表示继续，否则返回输者
	int transfer()
	{
		int color1 = 0, color2 = 1;
		if (Sample::transCount[color1] == 0 && Sample::transCount[color2] == 0)
			return -1;
		if (Sample::transCount[color1] == 0 || Sample::transCount[color2] == 0)
		{
			if (Sample::transCount[color1] == 0 && Sample::transCount[color2] > 0)
				swap(color1, color2);
			int h2;
			Sample::maxHeight[color2] = h2 = Sample::maxHeight[color2] + Sample::transCount[color1];
			if (h2 > MAPHEIGHT)
				return color2;
			int i, j;

			for (i = h2; i > Sample::transCount[color1]; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					Sample::gridInfo[color2][i][j] = Sample::gridInfo[color2][i - Sample::transCount[color1]][j];

			for (i = Sample::transCount[color1]; i > 0; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					Sample::gridInfo[color2][i][j] = Sample::trans[color1][i - 1][j];
			return -1;
		}
		else
		{
			int h1, h2;
			Sample::maxHeight[color1] = h1 = Sample::maxHeight[color1] + Sample::transCount[color2];//从color1处移动count1去color2
			Sample::maxHeight[color2] = h2 = Sample::maxHeight[color2] + Sample::transCount[color1];

			if (h1 > MAPHEIGHT) {
				if (h2 > MAPHEIGHT)
					return (score[color1] < score[color2] ? color1 : color2);
				return color1;
			}
			if (h2 > MAPHEIGHT) return color2;

			int i, j;
			for (i = h2; i > Sample::transCount[color1]; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					Sample::gridInfo[color2][i][j] = Sample::gridInfo[color2][i - Sample::transCount[color1]][j];

			for (i = Sample::transCount[color1]; i > 0; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					Sample::gridInfo[color2][i][j] = Sample::trans[color1][i - 1][j];

			for (i = h1; i > Sample::transCount[color2]; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					Sample::gridInfo[color1][i][j] = Sample::gridInfo[color1][i - Sample::transCount[color2]][j];

			for (i = Sample::transCount[color2]; i > 0; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					Sample::gridInfo[color1][i][j] = Sample::trans[color2][i - 1][j];

			return -1;
		}
	}

	// 颜色方还能否继续游戏
	inline bool canPut(int color, int blockType)
	{
		Tetris t(blockType, color);
		for (int y = MAPHEIGHT; y >= 1; y--)
			for (int x = 1; x <= MAPWIDTH; x++)
				for (int o = 0; o < 4; o++)
				{
					t.set(x, y, o);
					if (t.isValid() && checkDirectDropTo(color, blockType, x, y, o))
						return true;
				}
		return false;
	}
    
    // 能不能以o的姿势放到(x, y)这个位置且刚好挨地
    bool canPutThere(int color, int blockType, int x, int y, int o)
    {
        Tetris t(blockType, color);
        if (y > 1) {
            t.set(x, y-1, o);
            if (t.isValid()) {
                // 都没挨地！
                return false;
            }
        }
        
        int can[MAPWIDTH+1] = {0};
        // 枚举出最顶上那行所有可能的姿态和位置
        for (int i = 1; i <= MAPWIDTH; ++i) {
            for (int k = 0; k < 4; ++k) {
                t.set(i, MAPHEIGHT-1, k);
                if (t.isValid()) {
                    can[i] |= (1 << k);
                }
            }
        }
        // 往下降一格，每个方块原地转转看看行不行，再往左看看行不行，再原地转转看看行不行
        for (int j = MAPHEIGHT-1; j > y; --j) {
            for (int i = 1; i <= MAPWIDTH; ++i) {
                // 原地转；can[i] == 15就是哪个方向都行，就不用转了
                for (int k = 0; k < 4 && can[i] != 15; ++k) {
                    if (!(can[i] & (1 << k)))
                        continue;
                    int z = (k + 1) & 3;
                    while (can[i] != 15) {
                        t.set(i, j, z);
                        if (!t.isValid())
                            break;
                        can[i] |= (1 << z);
                        z = (++z) & 3;
                    }
                }
                // 往左
                for (int k = 0; k < 4; ++k) {
                    int p = i;
                    while (--p && !(can[p] & (1 << k))) {
                        t.set(p, j, k);
                        if (t.isValid()) {
                            can[p] |= (1 << k);
                            // 再转转
                            int z = (k + 1) & 3;
                            while (can[p] != 15) {
                                t.set(p, j, z);
                                if (!t.isValid())
                                    break;
                                can[p] |= (1 << z);
                                z = (++z) & 3;
                            }
                        }
                    }
                }
            }
            // 掉不下去的就不管了
            for (int i = 1; i <= MAPWIDTH; ++i) {
                for (int k = 0; k < 4; ++k) {
                    if (!(can[i] & (1 << k)))
                        continue;
                    t.set(i, j-1, k);
                    if (!t.isValid()) {
                        can[i] &= ~(1 << k);
                    }
                }
            }
        }
        return can[x] & (1 << o);
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
    
    int evaluate(const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2],
                 const int nextBlockType, int role)
    {
        // 每空一行100
        int ret = (MAPHEIGHT - maxHeight[role]) * 100;
        int blanks[MAPHEIGHT+1]={0};
        Tetris t(nextBlockType, role);
        
        // 可以消行的奖励多一些（每行150）
        for (int y = maxHeight[role]; y >= 1; --y) {
            blanks[y] = count_if(gridInfo[role][y]+1, gridInfo[role][y]+MAPWIDTH+1,
                                 [](int a) -> bool { return a == 0; });
        }
        
        for (int y = maxHeight[role]+2; y >= 1; --y) {
            if (blanks[y] > 3 && !(blanks[y] == 4 && nextBlockType == 5))
                continue;
            for (int x = 1; x <= MAPWIDTH; ++x) {
                for (int o = 0; o < 4; ++o) {
                    if (canPutThere(role, nextBlockType, x, y, o)) {
                        for (int i = 1; i < 8; i += 2) {
                            int index = y + blockShape[nextBlockType][o][i];
                            if (!--blanks[index]) {
                                ret += 150;
                            }
                        }
                    }
                }
            }
        }
        
        return ret;
    }

	//样例决策
	int sampleStrategy(
		const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2], int(&typeCount)[2][7],
		const int nextBlockType, int depth, int alpha, int beta, int role)
	{
		memcpy(Sample::gridInfo, gridInfo, sizeof(gridInfo));
		int myColor = role;
		/* 贪心决策												*/
		/* 从下往上以各种姿态找到第一个位置，要求能够直着落下		*/
		Sample::Tetris block(nextBlockType, myColor);
		for (int y = 1; y <= MAPHEIGHT; y++)
			for (int x = 1; x <= MAPWIDTH; x++)
				for (int o = 0; o < 4; o++)
				{
					if (block.set(x, y, o).isValid() &&
						Sample::checkDirectDropTo(myColor, block.blockType, x, y, o))
					{
						result = { x,y,o };
						goto determined;
					}
				}

	determined:
		// 再看看给对方什么好

		int maxCount = 0, minCount = 99;
		for (int i = 0; i < 7; i++)
		{
			if (typeCount[1 - myColor][i] > maxCount)
				maxCount = typeCount[1 - myColor][i];
			if (typeCount[1 - myColor][i] < minCount)
				minCount = typeCount[1 - myColor][i];
		}
		if (maxCount - minCount == 2)
		{
			// 危险，找一个不是最大的块给对方吧
			for (blockForEnemy = 0; blockForEnemy < 7; blockForEnemy++)
				if (typeCount[1 - myColor][blockForEnemy] != maxCount)
					break;
		}
		else
		{
			blockForEnemy = rand() % 7;
		}


		return 0;
	}

}

//both required in botzone.org and localTest
int programInit()
{
	istream::sync_with_stdio(false);		// 加速输入
	srand((unsigned)time(nullptr));
#ifdef __APPLE__
	freopen("/Users/whitephosphorus/Desktop/in.txt", "r", stdin);
#endif

#ifdef _BOTZONE_ONLINE
	MODE = 0;
#else
	cout << "Local: Please enter test mode: ";
	cin >> MODE;
	if (cin.fail())
		exit(0);
#endif // _BOTZONE_ONLINE

	return 0;
}

//read between { and } return false when failure
bool getJsonStr(istream& in, Json::Value& json)
{
	string str;
	Json::Reader reader;

	int stk = 1;
	char beginChar;
	in >> beginChar;
	if (beginChar != '{')
		return false;
	str = "{";
	int ch;
	while ((ch = cin.get()) != EOF)
	{
		str.push_back(char(ch));
		if (ch == '{')stk++;
		if (ch == '}')stk--;
		if (stk == 0)
			break;
	}
	if (stk != 0)
		return false;


	return reader.parse(str, json);
}

// 围一圈护城河
void stateInit(int(&grid)[2][MAPHEIGHT + 2][MAPWIDTH + 2])
{
	int i;
	for (i = 0; i < MAPHEIGHT + 2; i++)
	{
		grid[1][i][0] = grid[1][i][MAPWIDTH + 1] = -2;
		grid[0][i][0] = grid[0][i][MAPWIDTH + 1] = -2;
	}
	for (i = 0; i < MAPWIDTH + 2; i++)
	{
		grid[1][0][i] = grid[1][MAPHEIGHT + 1][i] = -2;
		grid[0][0][i] = grid[0][MAPHEIGHT + 1][i] = -2;
	}
}

int recoverState(int(&grid)[2][MAPHEIGHT + 2][MAPWIDTH + 2],
	int(&nextType)[2], int(&typeCount)[2][7], int &myColor)
{
	memcpy(Sample::gridInfo, grid, sizeof(grid));
	//读入JSON
	Json::Value input;
	getJsonStr(cin, input);
#ifndef _BOTZONE_ONLINE
	interpretSeverLog(input);
#endif // !_BOTZONE_ONLINE

	int curTurnID, tmpBlockType;


	/* 先读入第一回合，得到自己的颜色	*/
	/* 双方的第一块肯定是一样的		*/
	curTurnID = input["responses"].size() + 1;
	auto &first = input["requests"][(Json::UInt) 0];

	tmpBlockType = first["block"].asInt();
	myColor = first["color"].asInt();

	nextType[0] = tmpBlockType;
	nextType[1] = tmpBlockType;
	typeCount[0][tmpBlockType]++;
	typeCount[1][tmpBlockType]++;


	/* 然后分析以前每回合的输入输出，并恢复状态			*/
	/* 循环中，color 表示当前这一行是 color 的行为		*/

	for (int i = 1; i < curTurnID; i++)
	{
		/* 根据这些输入输出逐渐恢复状态到当前回合					*/
		int currType[2] = { nextType[0], nextType[1] };
		Block block;

		/* 先读自己的输出，也就是自己的行为						*/
		/* 然后模拟放置块									 		*/
		auto &myOutput = input["responses"][i - 1];
		tmpBlockType = myOutput["block"].asInt();
		block = myOutput;

		// 我当时把上一块落到了 x y o！
		Sample::Tetris myBlock(currType[myColor], myColor);
		myBlock.set(block).place();

		// 我给对方什么块来着？
		typeCount[1 - myColor][tmpBlockType]++;
		nextType[1 - myColor] = tmpBlockType;

		/* 然后读对方的输入，也就是对方的行为   */
		/* 裁判给自己的输入只有对方的最后一步   */
		auto &myInput = input["requests"][i];
		tmpBlockType = myInput["block"].asInt();
		block = myInput;

		// 对方当时把上一块落到了 x y o！
		Sample::Tetris enemyBlock(currType[1 - myColor], 1 - myColor);
		enemyBlock.set(block).place();

		// 对方给我什么块来着？
		typeCount[myColor][tmpBlockType]++;
		nextType[myColor] = tmpBlockType;

		// 检查消去
		Sample::eliminate(0);
		Sample::eliminate(1);

		// 进行转移
		Sample::transfer();
	}
	memcpy(grid, Sample::gridInfo, sizeof(grid));
	return 0;
}

int negativeMaxSearch(
	const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2], int(&typeCount)[2][7],
	const int nextBlockType, int depth, int alpha, int beta, int role)
{
	return 0;
}

void outputResult(const int &blockForEnemy, const Block &result)
{
	Json::Value output;
	Json::FastWriter writer;

	output["response"] = result;
	output["response"]["block"] = blockForEnemy;


	cout << writer.write(output);
}

int main()
{
	// 0为红，1为蓝
	int myColor;
	int nextType[2];
	// 给对应玩家的各类块的数目总计
	int typeCount[2][7] = { 0 };
	// 先y后x，记录地图状态，0为空，1为以前放置，2为刚刚放置，负数为越界
	int curGrid[2][MAPHEIGHT + 2][MAPWIDTH + 2] = { 0 };

	programInit();

	stateInit(curGrid);


	if (MODE == 0 || MODE == 1)
	{
		recoverState(curGrid, nextType, typeCount, myColor);

#ifndef _BOTZONE_ONLINE
		Sample::printField(curGrid);
#endif // _BOTZONE_ONLINE

		Sample::sampleStrategy(curGrid, typeCount, nextType[myColor], 1, -INF, INF, 0);

		outputResult(blockForEnemy, result);
	}
#ifndef _BOTZONE_ONLINE
	else if (MODE == 2)
	{
		gameEngineWork();
	}
#endif // _BOTZONE_ONLINE
	else
		exit(0);

	return 0;
}
