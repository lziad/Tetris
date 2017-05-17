




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

bool operator==(const State &a, const State &b)
{
	if (a.nextType != b.nextType)return false;
	if (memcmp(a.typeCount, b.typeCount, sizeof(a.typeCount)))return false;
	if (memcmp(a.grids, b.grids, sizeof(a.grids)))return false;
	return true;
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

	// 颜色方还能否继续游戏 (orignal canput)
	inline bool canContinue(int color, int blockType)
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

#ifndef DefaultMode
	cout << "Local: Please enter test mode: ";
	cin >> MODE;
	if (cin.fail())
		exit(0);
#else
	MODE = DefaultMode;
#endif // !DefaultMode


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
void stateInit(int(&grids)[2][MAPHEIGHT + 2][MAPWIDTH + 2])
{
	int i;
	for (i = 0; i < MAPHEIGHT + 2; i++)
	{
		grids[1][i][0] = grids[1][i][MAPWIDTH + 1] = -2;
		grids[0][i][0] = grids[0][i][MAPWIDTH + 1] = -2;
	}
	for (i = 0; i < MAPWIDTH + 2; i++)
	{
		grids[1][0][i] = grids[1][MAPHEIGHT + 1][i] = -2;
		grids[0][0][i] = grids[0][MAPHEIGHT + 1][i] = -2;
	}
}

int recoverState(int(&grids)[2][MAPHEIGHT + 2][MAPWIDTH + 2],
	int(&nextType)[2], int(&typeCount)[2][7], int &myColor)
{
	memcpy(Sample::gridInfo, grids, sizeof(grids));
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
	memcpy(grids, Sample::gridInfo, sizeof(grids));
	return 0;
}

void AI::GenerateStrategy(const State(&states)[2])
{

}

//role: 0, in my field; 1, in op's field
int AI::negativeMaxSearch(const State &curState, int depth, int alpha, int beta, int role)
{
	int score;

	// check whether current state has been calculated
	if (mp.find(curState) != mp.end())
		return mp.find(curState)->second;

	//!!
	// 搜索深度达到 DEPTH + 1 ，已经胜利(改成不能放），则返回
	score = evaluate(curState, role);
	//!!! == must be changed
	if (depth == DEPTH + 1 || depth > 1 && abs(score) == LostValue) {
		// 不同深度是否需要不同结果？
		mp.insert({ curState, score });
		return score;
	}

	int totInfo = 0;
	StateInfo info[180];
	int index[180];
	// 产生所有的走法
	GenerateAllPossibleMove(curState, info, totInfo, role);
	for (int i = 0; i < totInfo; i++)index[i] = i;

	//x1

	sort(index, index + totInfo, IndexCmp(info));

	int new_alpha = alpha;
	Block bestChoice{ 0,0,0 };

	score = -INF;

	for (int i = 0; i < totInfo; ++i) {

		StateInfo &cur = info[index[i]];

		// 递归
		int result = -negativeMaxSearch(cur.state, depth + 1, -beta, -new_alpha, 1 - role);

		if (result > score) {
			bestChoice = cur.choice;
			score = result;
		}

		// beta剪枝
		if (score >= beta) {
			break;
		}

		// 更新最高分
		if (score > new_alpha) {
			new_alpha = score;
		}
	}

	//x2

	if (depth == 1) {
		AI::bestChoice = bestChoice;
	}

	// hash
	mp.insert({ curState,score });

	return score;
}

void AI::GenerateAllPossibleMove(const State &curState, StateInfo *info, int &totInfo, int role)
{
	totInfo = 0;

	if (role == 1)
	{
		int containZero = 0;
		for (int i = 0; i < 7; i++)
		{
			if (curState.typeCount[i] == 0)
			{
				containZero = 1;
				break;
			}
		}

		for (int i = 0; i < 7; i++)
		{
			if (curState.typeCount[i] < 2 || curState.typeCount[i] == 2 && !containZero)
			{
				info[totInfo].state = curState;
				info[totInfo].state.nextType = i;
				//abnormal use of o
				info[totInfo].choice.o = i;
				totInfo++;
			}
		}

		return;
	}


	int type = curState.nextType;
	for (int x = 0; x < MAPWIDTH; ++x) {
		for (int y = MAPWIDTH-1; y >= 0; --y) {
			int can[MAPWIDTH + 1] = { 0 };
			Sample::Tetris t(type, 0);

			// 枚举出最顶上那行所有可能的姿态和位置
			for (int i = 0; i < MAPWIDTH; ++i) {
				for (int k = 0; k < 4; ++k) {
					if (t.isValid(curState, i, MAPHEIGHT - 1, k)) {
						can[i] |= (1 << k);
					}
				}
			}

			// 往下降一格，每个方块原地转转看看行不行，再往左看看行不行，再原地转转看看行不行
			for (int j = MAPHEIGHT - 2; j > y; --j) {
				for (int i = 0; i < MAPWIDTH; ++i) {
					// 原地转；can[i] == 15就是哪个方向都行，就不用转了
					for (int k = 0; k < 4 && can[i] != 15; ++k) {
						if (!(can[i] & (1 << k)))
							continue;
						int z = (k + 1) & 3;
						while (can[i] != 15) {
							if (!t.isValid(curState, i, j, z))
								break;
							can[i] |= (1 << z);
							z = (++z) & 3;
						}
					}
					// 往左
					for (int k = 0; k < 4; ++k) {
						int p = i;
						while (--p && !(can[p] & (1 << k))) {
							if (t.isValid(curState, p, j, k)) {
								can[p] |= (1 << k);
								// 再转转
								int z = (k + 1) & 3;
								while (can[p] != 15) {
									if (!t.isValid(curState, p, j, z))
										break;
									can[p] |= (1 << z);
									z = (++z) & 3;
								}
							}
						}
					}
				}
				// 掉不下去的就不管了
				for (int i = 0; i < MAPWIDTH; ++i) {
					for (int k = 0; k < 4; ++k) {
						if (!(can[i] & (1 << k)))
							continue;
						if (!t.isValid(curState, i, j - 1, k)) {
							can[i] &= ~(1 << k);
						}
					}
				}
			}

			for (int o = 0; o < 4; ++o) {
				if (y > 0 && t.isValid(curState, x, y - 1, o)) continue;
				if (!t.isValid(curState, x, y, o)) continue;
                if (can[x] & (1 << o)) {
                    info[totInfo] = AI::StateInfo();
					info[totInfo].state = curState;
					// set
					int i, tmpX, tmpY;
					for (i = 0; i < 4; i++) {
						tmpX = x + blockShape[type][o][2 * i];
						tmpY = y + blockShape[type][o][2 * i + 1];
						info[totInfo].state.grids[tmpY][tmpX] = true;
					}

					info[totInfo].choice = Block(x, y, o);
					info[totInfo].id = totInfo;

					// cal score

					int height = 0;
					int fullLines = 0;

					for (int y = 1; y <= MAPWIDTH; ++y) {
						bool isFull = true;
						for (int x = 1; x <= MAPWIDTH; ++x) {
							if (info[totInfo].state.grids[y][x]) {
								height = y;
							} else {
								isFull = false;
							}
						}
						if (isFull) ++fullLines;
					}

					info[totInfo].score = (MAPHEIGHT - height) * 100 + fullLines * 150;
                    ++(info[totInfo].state.typeCount[type]);

                    ++totInfo;
				}
			}

		}
	}
}

bool AI::IndexCmp::operator ()(const int a, const int b)const
{
	return info[a].score - info[b].score;
}

void outputResult(const int &blockForEnemy, const Block &result)
{
	Json::Value output;
	Json::FastWriter writer;

	output["response"] = result;
	output["response"]["block"] = blockForEnemy;


	cout << writer.write(output);
}

// 能不能以o的姿势放到(x, y)这个位置且刚好挨地  (orignal canPutThere)
bool canReach(int color, int blockType, int x, int y, int o)
{
	Sample::Tetris t(blockType, color);
	if (y > 1) {
		t.set(x, y - 1, o);
		if (t.isValid()) {
			// 都没挨地！
			return false;
		}
	}

	int can[MAPWIDTH + 1] = { 0 };
	// 枚举出最顶上那行所有可能的姿态和位置
	for (int i = 1; i <= MAPWIDTH; ++i) {
		for (int k = 0; k < 4; ++k) {
			t.set(i, MAPHEIGHT - 1, k);
			if (t.isValid()) {
				can[i] |= (1 << k);
			}
		}
	}
	// 往下降一格，每个方块原地转转看看行不行，再往左看看行不行，再原地转转看看行不行
	for (int j = MAPHEIGHT - 1; j > y; --j) {
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
				t.set(i, j - 1, k);
				if (!t.isValid()) {
					can[i] &= ~(1 << k);
				}
			}
		}
	}
	return bool(can[x] & (1 << o));
}

//!! adjusted gridInfo size
int evaluate(const State &state, int role)
{

	// 每空一行100
	int ret = (MAPHEIGHT - Sample::maxHeight[role]) * 100;
	int blanks[MAPHEIGHT] = { 0 };
	Sample::Tetris t(state.nextType, role);

	// 可以消行的奖励多一些（每行150）
	for (int y = Sample::maxHeight[role]; y >= 1; --y) {
		blanks[y] = count_if(state.grids[y], state.grids[y] + MAPWIDTH,
			[](bool a) -> bool { return a == 0; });
	}

	for (int y = Sample::maxHeight[role] + 2; y >= 1; --y) {
		if (blanks[y] > 3 && !(blanks[y] == 4 && state.nextType == 5))
			continue;
		for (int x = 1; x <= MAPWIDTH; ++x) {
			for (int o = 0; o < 4; ++o) {
				if (canReach(role, state.nextType, x, y, o)) {
					for (int i = 1; i < 8; i += 2) {
						int index = y + blockShape[state.nextType][o][i];
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

State::State(const int(&_grid)[MAPHEIGHT][MAPWIDTH],
	const int(&_typeCount)[7], const int nextType)
	:nextType(nextType)
{
	for (int i = 0; i < 20; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			grids[i][j] = (bool)_grid[i][j];
		}
	}
	memcpy(typeCount, _typeCount, sizeof(typeCount));
	auto min = *min_element(typeCount, typeCount + 7);
	for (int i = 0; i < 7; i++)
		typeCount[i] -= min;
}

State::operator Int256()const
{
	Int256 ret;
	//memset(ret.data, 0, sizeof(ret.data));
	for (int d = 0; d < 3; d++)
	{
		for (int i = 0; i < 63; i++)
		{
			if (grids[(d * 64 + i) / 10][(d * 64 + i) % 10])
				ret.data[d] += 1ull << i;
		}
	}
	for (int i = 2; i < 9; i++)
		if (grids[19][i])
			ret.data[3] += 1ull << i;
	for (int i = 0; i < 7; i++)
	{
		ret.data[3] += typeCount[i] << (i * 2 + 10);
	}
	ret.data[3] += nextType << 30;
	return ret;
}

void State::init()
{
	for (int i = 0; i < MAPHEIGHT; i++)
		for (int j = 0; j < MAPWIDTH; j++)
			grids[i][j] = 0;
	for (int i = 0; i < 7; i++)
		typeCount[i] = 0;
	nextType = 0;	//模拟 玄学系统
}

namespace std
{
	unsigned hash<State>::operator()(const State &state)const
	{
		Int256 i = state;
		return hash<ULL>()(i.data[0]) ^
			hash<ULL>()(i.data[1]) ^
			hash<ULL>()(i.data[2]) ^
			hash<ULL>()(i.data[3]);
	}
}

// 去掉了没用的参数检查
inline bool Sample::Tetris::isValid(const State &curState, int x, int y, int o)
{
	int i, tmpX, tmpY;
	for (i = 0; i < 4; i++)
	{
		tmpX = x + shape[o][2 * i];
		tmpY = y + shape[o][2 * i + 1];
		if (tmpX < 0 || tmpX >= MAPWIDTH ||
			tmpY < 0 || tmpY >= MAPHEIGHT ||
			curState.grids[tmpY][tmpX] != 0)
			return false;
	}
	return true;
}

int main()
{
	// 0为红，1为蓝
	int myColor;

	State curState[2];

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

		for (int i = 0; i < 2; i++)
		{
			curState[i].nextType = nextType[i];
			memcpy(curState[i].typeCount, typeCount[i], sizeof(typeCount[i]));
			for (int r = 0; r < MAPHEIGHT; r++)
				for (int c = 0; c < MAPWIDTH; c++)
					curState[i].grids[r][c] = curGrid[i][r + 1][c + 1];

		}

#ifndef _BOTZONE_ONLINE
		printField(curGrid);
#endif // _BOTZONE_ONLINE

		auto ai = new AI();

		ai->negativeMaxSearch(curState[0], 4, -INF, INF, 0);
		ai->negativeMaxSearch(curState[1], 4, -INF, INF, 1);

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
