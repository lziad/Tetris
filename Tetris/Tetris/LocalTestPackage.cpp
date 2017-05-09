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

/* 旋转中心的x轴坐标	   */
/* 旋转中心的y轴坐标	   */
/* 标记方块的朝向 0~3	   */
struct Block
{
    int x, y, o;
    
    Block() {}
    Block(const int &x, const int &y, const int &o)
    :x(x), y(y), o(o) {}
    
    Block(const Json::Value& jv)
    {
        x = jv["x"].asInt();
        y = jv["y"].asInt();
        o = jv["o"].asInt();
    }
    
    operator Json::Value()const
    {
        Json::Value jv;
        jv["x"] = x;
        jv["y"] = y;
        jv["o"] = o;
        return jv;
    }
};

const int blockShape[7][4][8] = {
    { { 0,0,1,0,-1,0,-1,-1 },{ 0,0,0,1,0,-1,1,-1 },{ 0,0,-1,0,1,0,1,1 },{ 0,0,0,-1,0,1,-1,1 } },
    { { 0,0,-1,0,1,0,1,-1 },{ 0,0,0,-1,0,1,1,1 },{ 0,0,1,0,-1,0,-1,1 },{ 0,0,0,1,0,-1,-1,-1 } },
    { { 0,0,1,0,0,-1,-1,-1 },{ 0,0,0,1,1,0,1,-1 },{ 0,0,-1,0,0,1,1,1 },{ 0,0,0,-1,-1,0,-1,1 } },
    { { 0,0,-1,0,0,-1,1,-1 },{ 0,0,0,-1,1,0,1,1 },{ 0,0,1,0,0,1,-1,1 },{ 0,0,0,1,-1,0,-1,-1 } },
    { { 0,0,-1,0,0,1,1,0 },{ 0,0,0,-1,-1,0,0,1 },{ 0,0,1,0,0,-1,-1,0 },{ 0,0,0,1,1,0,0,-1 } },
    { { 0,0,0,-1,0,1,0,2 },{ 0,0,1,0,-1,0,-2,0 },{ 0,0,0,1,0,-1,0,-2 },{ 0,0,-1,0,1,0,2,0 } },
    { { 0,0,0,1,-1,0,-1,1 },{ 0,0,-1,0,0,-1,-1,-1 },{ 0,0,0,-1,1,-0,1,-1 },{ 0,0,1,0,0,1,1,1 } }
};

namespace AI {
    /* 先y后x，记录地图状态，0为空，1为以前放置，2为刚刚放置，负数为越界	  */
    /* （2用于在清行后将最后一步撤销再送给对方）						  */
    int gridInfo[2][MAPHEIGHT + 2][MAPWIDTH + 2] = { 0 };
    
    // 代表分别向对方转移的行
    int trans[2][4][MAPWIDTH + 2] = { 0 };
    // 转移行数
    int transCount[2] = { 0 };
    // 运行eliminate后的当前高度
    int maxHeight[2] = { 0 };
    
    // 防止全局变量冲突
    int blockForEnemy;
    
    Block result;
    
    class Tetris
    {
    public:
        const int blockType;   // 标记方块类型的序号 0~6
        Block block;
        const int(*shape)[8]; // 当前类型方块的形状定义
        
        int color;
        
        Tetris(int t, int color) : blockType(t), shape(blockShape[t]), color(color) {}
        
        
        inline Tetris &set(int x = -1, int y = -1, int o = -1)
        {
            
            block.x = x == -1 ? block.x : x;
            block.y = y == -1 ? block.y : y;
            block.o = o == -1 ? block.o : o;
            return *this;
        }
        
        inline Tetris &set(const Block& _block = { -1,-1,-1 })
        {
            
            block.x = _block.x == -1 ? block.x : _block.x;
            block.y = _block.y == -1 ? block.y : _block.y;
            block.o = _block.o == -1 ? block.o : _block.o;
            return *this;
        }
        
        // 判断当前位置是否合法
        inline bool isValid(int x = -1, int y = -1, int o = -1)
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
                    AI::gridInfo[color][tmpY][tmpX] != 0)
                    return false;
            }
            return true;
        }
        
        // 判断是否落地
        inline bool onGround()
        {
            if (isValid() && !isValid(-1, block.y - 1))
                return true;
            return false;
        }
        
        // 将方块放置在场地上
        inline bool place()
        {
            if (!onGround())
                return false;
            
            int i, tmpX, tmpY;
            for (i = 0; i < 4; i++)
            {
                tmpX = block.x + shape[block.o][2 * i];
                tmpY = block.y + shape[block.o][2 * i + 1];
                AI::gridInfo[color][tmpY][tmpX] = 2;
            }
            return true;
        }
        
        // 检查能否逆时针旋转自己到o
        inline bool rotation(int o)
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
    };
    
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
                if (_y < 1 || _x < 1 || _x > MAPWIDTH || AI::gridInfo[color][_y][_x])
                    return false;
            }
        return true;
    }
    
    
    int sampleStrategy(
        const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2], int(&typeCount)[2][7],
        const int nextBlockType, int depth, int alpha, int beta, int role)
    {
        memcpy(AI::gridInfo, gridInfo, sizeof(gridInfo));
        int myColor = role;
        /* 贪心决策												*/
        /* 从下往上以各种姿态找到第一个位置，要求能够直着落下		*/
        AI::Tetris block(nextBlockType, myColor);
        for (int y = 1; y <= MAPHEIGHT; y++)
            for (int x = 1; x <= MAPWIDTH; x++)
                for (int o = 0; o < 4; o++)
                {
                    if (block.set(x, y, o).isValid() &&
                        AI::checkDirectDropTo(myColor, block.blockType, x, y, o))
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
    
    // 消去行
    void eliminate(int color)
    {
        int &count = AI::transCount[color] = 0;
        int i, j, emptyFlag, fullFlag;
        AI::maxHeight[color] = MAPHEIGHT;
        for (i = 1; i <= MAPHEIGHT; i++)
        {
            emptyFlag = 1;
            fullFlag = 1;
            for (j = 1; j <= MAPWIDTH; j++)
            {
                if (AI::gridInfo[color][i][j] == 0)
                    fullFlag = 0;
                else
                    emptyFlag = 0;
            }
            if (fullFlag)
            {
                for (j = 1; j <= MAPWIDTH; j++)
                {
                    // 注意这里只转移以前的块，不包括最后一次落下的块（“撤销最后一步”）
                    AI::trans[color][count][j] = AI::gridInfo[color][i][j] == 1 ? 1 : 0;
                    AI::gridInfo[color][i][j] = 0;
                }
                count++;
            }
            else if (emptyFlag)
            {
                AI::maxHeight[color] = i - 1;
                break;
            }
            else
                for (j = 1; j <= MAPWIDTH; j++)
                {
                    AI::gridInfo[color][i - count][j] =
                    AI::gridInfo[color][i][j] > 0 ? 1 : AI::gridInfo[color][i][j];
                    if (count)
                        AI::gridInfo[color][i][j] = 0;
                }
        }
        AI::maxHeight[color] -= count;
    }
    
    // 转移双方消去的行，返回-1表示继续，否则返回输者
    
    int transfer()
    {
        int color1 = 0, color2 = 1;
        if (AI::transCount[color1] == 0 && AI::transCount[color2] == 0)
            return -1;
        if (AI::transCount[color1] == 0 || AI::transCount[color2] == 0)
        {
            if (AI::transCount[color1] == 0 && AI::transCount[color2] > 0)
                swap(color1, color2);
            int h2;
            AI::maxHeight[color2] = h2 = AI::maxHeight[color2] + AI::transCount[color1];
            if (h2 > MAPHEIGHT)
                return color2;
            int i, j;
            
            for (i = h2; i > AI::transCount[color1]; i--)
                for (j = 1; j <= MAPWIDTH; j++)
                    AI::gridInfo[color2][i][j] = AI::gridInfo[color2][i - AI::transCount[color1]][j];
            
            for (i = AI::transCount[color1]; i > 0; i--)
                for (j = 1; j <= MAPWIDTH; j++)
                    AI::gridInfo[color2][i][j] = AI::trans[color1][i - 1][j];
            return -1;
        }
        else
        {
            int h1, h2;
            AI::maxHeight[color1] = h1 = AI::maxHeight[color1] + AI::transCount[color2];//从color1处移动count1去color2
            AI::maxHeight[color2] = h2 = AI::maxHeight[color2] + AI::transCount[color1];
            
            // TODO: 同时满出来要看双方分数
            if (h1 > MAPHEIGHT) return color1;
            if (h2 > MAPHEIGHT) return color2;
            
            int i, j;
            for (i = h2; i > AI::transCount[color1]; i--)
                for (j = 1; j <= MAPWIDTH; j++)
                    AI::gridInfo[color2][i][j] = AI::gridInfo[color2][i - AI::transCount[color1]][j];
            
            for (i = AI::transCount[color1]; i > 0; i--)
                for (j = 1; j <= MAPWIDTH; j++)
                    AI::gridInfo[color2][i][j] = AI::trans[color1][i - 1][j];
            
            for (i = h1; i > AI::transCount[color2]; i--)
                for (j = 1; j <= MAPWIDTH; j++)
                    AI::gridInfo[color1][i][j] = AI::gridInfo[color1][i - AI::transCount[color2]][j];
            
            for (i = AI::transCount[color2]; i > 0; i--)
                for (j = 1; j <= MAPWIDTH; j++)
                    AI::gridInfo[color1][i][j] = AI::trans[color2][i - 1][j];
            
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
    
    void printField(const int(&gridInfo)[2][MAPHEIGHT + 2][MAPWIDTH + 2])
    {
        static const char *i2s[] = { "~~","~~","  ","[]","##" };
        
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
    }
}

int gameEngineWork()
{
    // init
    int nextType[2] = { 0 };
    int typeCount[2][7] = { 0 };
    int loser = -1;
    stateInit(AI::gridInfo);
	AI::printField(AI::gridInfo);
    
    // Game start
    while (true) {
        // 决策、放方块，检测是否有人挂了
        AI::sampleStrategy(AI::gridInfo, typeCount, nextType[0], 1, -INF, INF, 0);
        int tmp = AI::blockForEnemy;
        AI::Tetris t0(nextType[0], 0);
        t0.set(AI::result);
        if (!t0.place()) {
            cout << "Player 0 lose!" << endl;
            break;
        }
        //AI::printField(AI::gridInfo);
        AI::sampleStrategy(AI::gridInfo, typeCount, nextType[1], 1, -INF, INF, 1);
        AI::Tetris t1(nextType[1], 1);
        t1.set(AI::result);
        if (!t1.place()) {
            cout << "Player 1 lose!" << endl;
            break;
        }
        
        nextType[0] = AI::blockForEnemy;
        nextType[1] = tmp;
        
        // 检查消去
        AI::eliminate(0);
        AI::eliminate(1);
        
        // 进行转移
        loser = AI::transfer();
        if (loser != -1) {
            cout << "Player " << loser << " lose!" << endl;
            break;
        }
        
        // 每回合输出一次
        AI::printField(AI::gridInfo);
    }
    
    // 终局图
    AI::printField(AI::gridInfo);
    
	return 0;
}
