/****************************************************/
/** 程序： 五子棋AI示例     *************************/                                                                         
/** 作者： jw@pku           *************************/
/** 日期： 2010-12-9        *************************/
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <ctime>


/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    全局参数（开始）                              ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

#define OPENDEBUG 0                 // 调试开关
#define INFINITY 1000000000         // 最大整数

#define BOARDSIZE 15                // 棋盘大小
#define DEPTH 4                     // 尝试深度
#define EXTERNWINDOW 3              // 可落子的格子与最近棋子的距离<3

#define HASHSIZE (1 << 20)          // 棋盘哈希表大小 1024*1024
#define HASHMOD  1048573            // 棋盘哈希表模值

// 局面估值
#define VALUE_WIN 10000             // 五子
#define VALUE_WE_DOUBLE_THREE 2000  // 我方双活三
#define VALUE_WE_THREE 200          // 我方活三
#define VALUE_THEY_DOUBLE_THREE 500 // 对方双活三
#define VALUE_THEY_THREE 100        // 对方活三
#define VALUE_SLEEP_THREE 10        // 眠三
#define VALUE_TWO 4                 // 活二
#define VALUE_SLEEP_TWO 1           // 眠二


int max(int a, int b) {
	return (a > b) ? a : b;
}

/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    全局参数（结束）                              ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/




/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    ChessMove结构（开始）                         ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

// 一步棋的结构
struct ChessMove {
	int x, y;   // 落子位置
	int color;  // 哪一方走
};

/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    ChessMove结构（结束）                         ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/


/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    ChessBoard结构（开始）                         ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

// 棋盘的结构（局面）
struct ChessBoard {
	char data[BOARDSIZE][BOARDSIZE];    // 数据
	ChessMove last_move;                // 上一步的走法
	int step;                           // 总共下的步数
};

// 对棋盘排序所需的结构
struct BoardScore {
	int index;                          // 保存了棋盘索引
	int score;                          // 保存了该棋盘的得分
};

// 两个棋盘的比较函数
int BoardScoreCmp(const void *a, const void *b) {
	if (((BoardScore*)a)->score != ((BoardScore*)b)->score) 
	{
		if (((BoardScore*)a)->score > ((BoardScore*)b)->score) 
		{
			return -1;
		} else {
			return 1;
		}
	} else {
		if (((BoardScore*)a)->index < ((BoardScore*)b)->index) {
			return -1; //分值相等，index优先
		} else if (((BoardScore*)a)->index > ((BoardScore*)b)->index) {
			return 1;
		} else {
			return 0;
		}
	}
}

/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    ChessBoard结构（结束）                        ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    HashChessBoard哈希表（开始）                  ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

// 棋盘哈希表
struct HashChessBoard 
{
	struct HashNode {   	// 一个哈希节点
		__int64 check_code;                   // 校验码
		int score;                            // 局面的得分
	};
	HashNode hash[HASHSIZE];                 //2^20
	__int64 zob[2][BOARDSIZE][BOARDSIZE];     // zobrist哈希
	__int64 check[2][BOARDSIZE][BOARDSIZE];   // 校验数组
};

// 初始化函数
int HashChessBoard_Init(HashChessBoard *hash_chess_board);

// 插入一个hash节点，包含一个棋局和向下看的深度，记录得分。
int HashChessBoard_Insert(HashChessBoard *hash_chess_board, const ChessBoard &chess_board, const int depth, const int score);

// 查找一个hash节点，输入一个棋局和向下看的深度，输出得分。
int HashChessBoard_Find(HashChessBoard *hash_chess_board, const ChessBoard &chess_board, const int depth, int *score);

// 输入一个棋局和向下看的深度，得到哈希值。
int HashChessBoard_GetKey(HashChessBoard *hash_chess_board, const ChessBoard &chess_board, const int depth, __int64 *check_code);

// 得到一个64位的随机数
__int64 HashChessBoard_Rand64();


int HashChessBoard_Init(HashChessBoard *hash_chess_board) {
	memset(hash_chess_board->hash, -1, sizeof(hash_chess_board->hash));

	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < BOARDSIZE; ++j) {
			for (int k = 0; k < BOARDSIZE; ++k) {
				hash_chess_board->zob[i][j][k] = HashChessBoard_Rand64();  //填随机数
			}
		}
	}

	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < BOARDSIZE; ++j) {
			for (int k = 0; k < BOARDSIZE; ++k) {
				hash_chess_board->check[i][j][k] = HashChessBoard_Rand64();
			}
		}
	}
	return 0;
}

int HashChessBoard_Insert(HashChessBoard *hash_chess_board, const ChessBoard &chess_board, const int depth, const int score)
{
	__int64 check_code;
	// 先得到哈希值
	int key = HashChessBoard_GetKey(hash_chess_board, chess_board, depth, &check_code);//返回key和check_code

	// 插入节点
	hash_chess_board->hash[key].check_code = check_code;
	hash_chess_board->hash[key].score = score;

	return 0;
}

int HashChessBoard_Find(HashChessBoard *hash_chess_board, const ChessBoard &chess_board, const int depth, int *score) {
	__int64 check_code;
	// 先得到哈希值。
	int key = HashChessBoard_GetKey(hash_chess_board, chess_board, depth, &check_code);

	// 如果校验码相等，认为存在；否则，认为不存在。
	if (check_code == hash_chess_board->hash[key].check_code)
	{
		*score = hash_chess_board->hash[key].score;
		return 0;
	} else {
		return -1;
	}
}


// 利用zobrist哈希技术
int HashChessBoard_GetKey(HashChessBoard *hash_chess_board, const ChessBoard &chess_board, const int depth, __int64 *check_code) 
{
	__int64 key = depth;
	*check_code = 0;

	// 分别得到哈希值和校验码。
	int color = 3 - chess_board.last_move.color;  //取值 0,1,2
	for (int i = 0; i < BOARDSIZE; ++i) {
		for (int j = 0; j < BOARDSIZE; ++j) 
		{
			if (chess_board.data[i][j] != 0) //有子
			{
				key ^= hash_chess_board->zob[chess_board.data[i][j] - 1][i][j]; //取出对应位置、颜色的随机数的值 异或运算
				(*check_code) ^= hash_chess_board->check[chess_board.data[i][j] - 1][i][j];//取check code
			}
		}
	}
	return (int)((key % HASHMOD + HASHMOD) % HASHMOD);
}


__int64 HashChessBoard_Rand64() {
	return rand() ^ ((__int64)rand() << 15) ^ ((__int64)rand() << 30) ^ ((__int64)rand() << 45) ^ ((__int64)rand() << 60); 
}



/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    HashChessBoard哈希表（结束）                  ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/




/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    Evaluate类（开始）                            ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

int iterator_num = 0;

struct Evaluate {
	int count_live[3][6];                                                 // 统计活子总数
	int count_sleep[3][6];                                                // 统计眠子总数
};

// 估计一个棋盘的得分。
int Evaluate_GetEvaluate(Evaluate *evaluate, const ChessBoard &chess_board, const int color);

// 分解一个棋盘并分析。
int Evaluate_DecBoard(Evaluate *evaluate, const ChessBoard &chess_board);

// 统计一个棋盘的估分。
int Evaluate_CountValue(Evaluate *evaluate, const ChessBoard &chess_board, const int color);

// 分析单独的一行。
int Evaluate_AnalysisLine(Evaluate *evaluate, const int num, const char line[BOARDSIZE]);


int Evaluate_GetEvaluate(Evaluate *evaluate, const ChessBoard &chess_board, const int color) {
	
	++iterator_num; // 估计计算量用

	memset(evaluate->count_live, 0, sizeof(evaluate->count_live));
	memset(evaluate->count_sleep, 0, sizeof(evaluate->count_sleep));

	// 先分解棋盘，再计算得分
	Evaluate_DecBoard(evaluate, chess_board);
	return Evaluate_CountValue(evaluate, chess_board, color);
}


int Evaluate_DecBoard(Evaluate *evaluate, const ChessBoard &chess_board) {
	char line[BOARDSIZE];
	// 分析行
	for (int i = 0; i < BOARDSIZE; ++i) {		
		for (int j = 0; j < BOARDSIZE; ++j) {
			line[j] = chess_board.data[i][j];
		}
		Evaluate_AnalysisLine(evaluate, BOARDSIZE, line);
	}

	// 分析列
	for (int i = 0; i < BOARDSIZE; ++i) {
		for (int j = 0; j < BOARDSIZE; ++j) {
			line[j] = chess_board.data[j][i];
		}
		Evaluate_AnalysisLine(evaluate, BOARDSIZE, line);
	}

	// 分析正对角线
	// 长度至少为5才行
	for (int i = 5; i <= BOARDSIZE; ++i) {
		for (int j = 0; j < i; ++j) {
			line[j] = chess_board.data[j][BOARDSIZE + j - i];
		}
		Evaluate_AnalysisLine(evaluate, i, line);
	}
	for (int i = 5; i < BOARDSIZE; ++i) {
		for (int j = 0; j < i; ++j) {
			line[j] = chess_board.data[BOARDSIZE + j - i][j];
		}
		Evaluate_AnalysisLine(evaluate, i, line);
	}

	// 分析反对角线
	for (int i = 5; i <= BOARDSIZE; ++i) {
		for (int j = 0; j < i; ++j) {
			line[j] = chess_board.data[i - j - 1][j];
		}
		Evaluate_AnalysisLine(evaluate, i, line);
	}

	for (int i = 5; i < BOARDSIZE; ++i) {
		for (int j = 0; j < i; ++j) {
			line[j] = chess_board.data[BOARDSIZE - j - 1][BOARDSIZE - i + j];
		}
		Evaluate_AnalysisLine(evaluate, i, line);
	}

	return 0;
}

int Evaluate_CountValue(Evaluate *evaluate, const ChessBoard &chess_board, const int color) {

	// 已经分出胜负的所有情况
	// 我方已有连5，我方胜。
	if (evaluate->count_live[color][5] + evaluate->count_sleep[color][5] > 0)
			return VALUE_WIN + 50;

	// 对方已有连5，我方胜。
	if (evaluate->count_live[3 - color][5] + evaluate->count_sleep[3 - color][5] > 0)
			return -VALUE_WIN - 50;

	// 两个眠四等于一个活四
	if (evaluate->count_sleep[color][4] >= 2)
		evaluate->count_live[color][4] = 1;

	if (evaluate->count_sleep[3 - color][4] >= 2)
		evaluate->count_live[3 - color][4] = 1;

	// 我方有连四，我方胜。
	if (evaluate->count_live[color][4] + evaluate->count_sleep[color][4] > 0)
		return VALUE_WIN + 40;

	// 我方没有连四，且对方有活四，对方胜。
	if (evaluate->count_live[3 - color][4] > 0)
		return -VALUE_WIN - 30;

	// 对方有眠四和活三，对方胜。
	if (evaluate->count_live[3 - color][3] > 0 && evaluate->count_sleep[3 - color][4] > 0)
		return -VALUE_WIN - 20;

	// 我方有活三切对方没有眠四和活四，我方胜。
	if (evaluate->count_live[color][3] > 0 &&
		evaluate->count_sleep[3 - color][4] + evaluate->count_live[3 - color][4] == 0)
		return VALUE_WIN + 10;

	// 对方有大于1个活三，我方没有连四和连三，对方胜。
	if (evaluate->count_live[3 - color][3] > 1 &&
		evaluate->count_live[color][4] + evaluate->count_sleep[color][4] + evaluate->count_live[color][3] + evaluate->count_sleep[color][3] == 0)
		return -VALUE_WIN;


    // 计算连子情况
	int value = 0;
	

	// 一个眠四等于一个活三
	if (evaluate->count_sleep[color][4] > 0)
		++evaluate->count_live[color][3];

	if (evaluate->count_sleep[3 - color][4] > 0)
		++evaluate->count_live[3 - color][3];

	// 计算活三价值
	if (evaluate->count_live[color][3] >= 2) {
		value += VALUE_WE_DOUBLE_THREE;
	} else {
		value += evaluate->count_live[color][3] * VALUE_WE_THREE;
	}

	if (evaluate->count_live[3 - color][3] >= 2) {
		value -= VALUE_THEY_DOUBLE_THREE;
	} else {
		value -= evaluate->count_live[3 - color][3] * VALUE_THEY_THREE;
	}

	// 计算眠三价值
	value += (evaluate->count_sleep[color][3] - evaluate->count_sleep[3 - color][3]) * VALUE_SLEEP_THREE;
	// 计算活二价值
	value += (evaluate->count_live[color][2] - evaluate->count_live[3 - color][2]) * VALUE_TWO;
	// 计算眠二价值
	value += (evaluate->count_sleep[color][2] - evaluate->count_sleep[3 - color][2]) * VALUE_SLEEP_TWO;

	// 计算棋盘分布情况
	for (int i = 0; i < BOARDSIZE; ++i) {
		for (int j = 0; j < BOARDSIZE; ++j) {
			if (chess_board.data[i][j] != 0) {
				int mul = (chess_board.data[i][j] == color) ? 1 : -1;
				value += (BOARDSIZE / 2 - max(abs(i - BOARDSIZE / 2), abs(j - BOARDSIZE / 2))) * mul;
			}
		}
	}

	// 返回最终得分
	return value;
}


int Evaluate_AnalysisLine(Evaluate *evaluate, const int num, const char line[BOARDSIZE]) {
	for (int i = 0; i < num; ++i) {
		if (line[i] == 0)
			continue;

		int j = i;
		for (; j + 1 < num && line[j + 1] == line[i]; ++j) {
		}

		// i -> j 连续有一种颜色的棋子
		int len = (j - i + 1);
		bool sleep = false;

		// 判断是否是眠子
		if (i == 0 || j == num - 1 ||
			line[i - 1] == 3 - line[i] || line[j + 1] == 3 - line[i]) {
			sleep = true;
		}

		// 左右两边的空格数
		int blank_num = 0;
		for (int k = i - 1; k >= 0 && line[k] == 0; --k) {
			++blank_num;
		}
		for (int k = j + 1; k < num && line[k] == 0; ++k) {
			++blank_num;
		}

		// 如果总共可扩展长度小于5，不计入统计
		if (len + blank_num >= 5) {

			// 扩展长度刚好为5，认为是眠子
			if (len + blank_num == 5)
				sleep = true;

			if (sleep) {
				++evaluate->count_sleep[line[i]][len];
			} else {
				++evaluate->count_live[line[i]][len];
			}
		}

		i = j;
	}

	return 0;
}



/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    Evaluate类（结束）                            ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/



/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    MoveGenerator类（开始）                       ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

// 招法产生器
int MoveGenerator_CreatePossibleMove(const ChessBoard &chess_board, ChessBoard *board_list, int *list_num) {

	ChessBoard new_board;

	*list_num = 0;

	// 如果处于第0步，直接在(7, 7)上放一个子，并返回。
	if (chess_board.step == 0) {
		memset(&new_board.data, 0, sizeof(new_board.data));
		new_board.data[BOARDSIZE / 2][BOARDSIZE / 2] = 3 - chess_board.last_move.color;

		new_board.last_move.x = BOARDSIZE / 2;
		new_board.last_move.y = BOARDSIZE / 2;
		new_board.last_move.color = 3 - chess_board.last_move.color;
		
		new_board.step = 1;

		board_list[(*list_num)++] = new_board;

		return 0;
	}

	// 如果一个空格左右曼哈顿顿距离 < EXTERNWINDOW 有棋子，则认为该空格可落子
	for (int i = 0; i < BOARDSIZE; ++i) {
		for (int j = 0; j <= BOARDSIZE; ++j) {
			if (chess_board.data[i][j] == 0) {
				bool flag = false;
				for (int k = -EXTERNWINDOW; k <= EXTERNWINDOW; ++k) {
					if (i + k < 0 || i + k >= BOARDSIZE)
						continue;

					for (int l = -(EXTERNWINDOW-abs(k)); l <=EXTERNWINDOW-abs(k); ++l) {

						if (j + l < 0 || j + l >= BOARDSIZE)
							continue;

						// 可落子
						if (chess_board.data[i + k][j + l] != 0) {
							flag = true;
							break;
						}
					}
					if (flag)
						break;
				}

				// 在 (i, j)上放子。
				if (flag) {
					memcpy(&new_board.data, chess_board.data, sizeof(new_board.data));
					new_board.data[i][j] = 3 - chess_board.last_move.color;
					new_board.last_move.x = i;
					new_board.last_move.y = j;
					new_board.last_move.color = 3 - chess_board.last_move.color;
					new_board.step = chess_board.step + 1;
					board_list[(*list_num)++] = new_board;
				}
			}
		}
	}

	return 0;
}

/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    MoveGenerator类（结束）                       ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/




/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    SearchMoveEngine类（开始）                    ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

// 一个SearchMoveEngine类
struct SearchMoveEngine {
	int color;						// 当前轮到谁下棋
	ChessBoard *chess_board;		// 当前的棋盘
	ChessMove *choose_chess_move;	// 给出一招下法

	Evaluate *evaluate;							// 估值结构
	HashChessBoard *hash_chess_board;			// 棋盘哈希表
	int history_score[BOARDSIZE][BOARDSIZE];	// 历史启发得分表。
};

// 初始化一个SearchMoveEngine类
int SearchMoveEngine_Init(SearchMoveEngine *search_move_engine, const int color);

// 清除相关内存
int SearchMoveEngine_Clear(SearchMoveEngine *search_move_engine);

// 让AI选一步走法
int SearchMoveEngine_MoveByAI(SearchMoveEngine *search_move_engine);

// 直接传入对手的走法
int SearchMoveEngine_MoveByRival(SearchMoveEngine *search_move_engine, const ChessMove &chess_move);

// 在当前棋局下，进行一步下棋
int SearchMoveEngine_MoveOneStep(SearchMoveEngine *search_move_engine, const ChessMove &chess_move);

// 进行负极大值递归搜索。
int SearchMoveEngine_NegamaxSearch(SearchMoveEngine *search_move_engine, const ChessBoard &chess_board, const int depth, const int alpha, const int beta);


int SearchMoveEngine_Init(SearchMoveEngine *search_move_engine, const int color) {
	search_move_engine->evaluate = NULL;
	search_move_engine->hash_chess_board = NULL;

	search_move_engine->chess_board = NULL;
	search_move_engine->choose_chess_move = NULL;

	search_move_engine->chess_board = new ChessBoard;
	memset(&(search_move_engine->chess_board->data), 0, sizeof(search_move_engine->chess_board->data));
	search_move_engine->chess_board->last_move.x = search_move_engine->chess_board->last_move.y = -1;
	search_move_engine->chess_board->last_move.color = 3 - color;
	search_move_engine->chess_board->step = 0;

	search_move_engine->color = color;
	search_move_engine->choose_chess_move = new ChessMove;

	search_move_engine->evaluate = new Evaluate;
	search_move_engine->hash_chess_board = new HashChessBoard;
	HashChessBoard_Init(search_move_engine->hash_chess_board);

	memset(search_move_engine->history_score, 0, sizeof(search_move_engine->history_score));
	return 0;
}

int SearchMoveEngine_Clear(SearchMoveEngine *search_move_engine) {
	if (search_move_engine->evaluate) {
		delete search_move_engine->evaluate;
	}

	if (search_move_engine->hash_chess_board) {
		delete search_move_engine->hash_chess_board;
	}

	if (search_move_engine->chess_board) {
		delete search_move_engine->chess_board;
	}

	if (search_move_engine->choose_chess_move) {
		delete search_move_engine->choose_chess_move;
	}

	return 0;
}

int SearchMoveEngine_MoveByAI(SearchMoveEngine *search_move_engine) {
	// 调用负极大值递归搜索，得到解法。
	SearchMoveEngine_NegamaxSearch(search_move_engine, *search_move_engine->chess_board, 1, - INFINITY, INFINITY);
	// 调用下棋函数走这一步。
	SearchMoveEngine_MoveOneStep(search_move_engine, *search_move_engine->choose_chess_move);
	printf("%d %d\n", search_move_engine->chess_board->last_move.x, search_move_engine->chess_board->last_move.y);
	if (OPENDEBUG) {
		printf("iterator num %d\n", iterator_num);
	}
	return 0;
}

int SearchMoveEngine_MoveByRival(SearchMoveEngine *search_move_engine, const ChessMove &chess_move) 
{
	// 直接调用下棋函数走这一步。
	return SearchMoveEngine_MoveOneStep(search_move_engine, chess_move);
}

int SearchMoveEngine_MoveOneStep(SearchMoveEngine *search_move_engine, const ChessMove &chess_move) 
{
	search_move_engine->chess_board->data[chess_move.x][chess_move.y] = chess_move.color;
	search_move_engine->chess_board->last_move = chess_move;
	++search_move_engine->chess_board->step;
	return 0;
}

// 负极大值过程
int SearchMoveEngine_NegamaxSearch(SearchMoveEngine *search_move_engine, const ChessBoard &chess_board, const int depth, const int alpha, const int beta) 
{
	int score;

	// 利用哈希表，排除重复搜索棋局
	if (HashChessBoard_Find(search_move_engine->hash_chess_board, chess_board, depth, &score) == 0)
		return score;

	// 对当前棋局估值，如果深度达到 DEPTH + 1 或者已经胜利，则返回
	score = Evaluate_GetEvaluate(search_move_engine->evaluate, chess_board, 3 - chess_board.last_move.color);
	if (depth == DEPTH + 1 || depth > 1 && abs(score) > VALUE_WIN) {
		HashChessBoard_Insert(search_move_engine->hash_chess_board, chess_board, depth, score);
		return score;
	}

	// 产生所有的走法
	int list_num;
	ChessBoard board_list[BOARDSIZE * BOARDSIZE];
	MoveGenerator_CreatePossibleMove(chess_board, board_list, &list_num);

	// 计算每一步的历史启发得分并排序
	BoardScore board_score_list[BOARDSIZE * BOARDSIZE];
	for (int i = 0; i < list_num; ++i) 
	{
		board_score_list[i].index = i;
		board_score_list[i].score =
			search_move_engine->history_score[board_list[i].last_move.x][board_list[i].last_move.y];
	}
	qsort(board_score_list, list_num, sizeof(BoardScore), BoardScoreCmp);

	// 开始 NegamaxSearch 过程
	int new_alpha = alpha;
	ChessMove best_move;
	best_move.x = best_move.y = -1;

	score = -INFINITY;

	for (int i = 0; i < list_num; ++i) {
		int index = board_score_list[i].index;

		// 递归调用负极大值搜索。
		int result = -SearchMoveEngine_NegamaxSearch(search_move_engine, board_list[index], depth + 1, - beta, - new_alpha);

		if (result > score) {
			// 记录最好走法
			best_move = board_list[index].last_move;
			score = result;
		}

		// beta剪枝 (其实在奇数步相当于alpha剪枝)
		if (score >= beta) {
			break;
		}

		// 记录最高分，更新最优步数
		if (score > new_alpha) {
			new_alpha = score;
		}
	}

	// 更新历史得分数组
	if (best_move.x != -1) {
		search_move_engine->history_score[best_move.x][best_move.y] += (1 << (DEPTH - depth));
	}

	// 如果在第一层，还要保存下最好走法
	if (depth == 1) {
		*search_move_engine->choose_chess_move = best_move;
	}

	// 讲当前格局的分数加入哈希表
	HashChessBoard_Insert(search_move_engine->hash_chess_board, chess_board, depth, score);

	// 返回最大得分
	return score;
}

/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    SearchMoveEngine类（结束）                    ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/




/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    GobangAIEngine类（开始）                      ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

// 总的AI控制类
struct GobangAIEngine {
	SearchMoveEngine *search_move_engine;
	int color;
};

// 初始化函数。
int GobangAIEngine_Init(GobangAIEngine *ai);

// 释放相关内存。
int GobangAIEngine_Clear(GobangAIEngine *ai);

// 整体工作逻辑。
int GobangAIEngine_Work(GobangAIEngine *ai);

int GobangAIEngine_Init(GobangAIEngine *ai) {
	ai->search_move_engine = NULL;

	char command[10];
	scanf("%s", command);

	if (strcmp(command, "[START]") != 0)
		return -1;

	// 读入颜色
	scanf("%d", &ai->color);

	ai->search_move_engine = new SearchMoveEngine;
	SearchMoveEngine_Init(ai->search_move_engine, ai->color);
	
	return 0;
}

int GobangAIEngine_Clear(GobangAIEngine *ai) {
	if (ai->search_move_engine) {
		SearchMoveEngine_Clear(ai->search_move_engine);
		delete ai->search_move_engine;
	}
	return 0;
}

int GobangAIEngine_Work(GobangAIEngine *ai) {
	char command[10];

	while (true) {

		scanf("%s", command);
		if (strcmp(command, "[PUT]") != 0)
			return -1;
		
		// 读对手下的棋
		ChessMove chess_move;
		scanf("%d%d", &chess_move.x, &chess_move.y);
		chess_move.color = 3 - ai->search_move_engine->color;

		if (chess_move.x != -1) {
			// 根据对手下的一步棋改变自己的棋局
			SearchMoveEngine_MoveByRival(ai->search_move_engine, chess_move);
		}

		int time1 = clock();
		// 调用自己的下棋引擎，走一步棋
		SearchMoveEngine_MoveByAI(ai->search_move_engine);
		int time2 = clock();
		if (OPENDEBUG) {
			printf("time  %dms\n", time2 - time1);
		}
		fflush(stdout);
	}

	return 0;
}

/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    GobangAIEngine类（结束）                      ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/


int main() {
	GobangAIEngine *ai = new GobangAIEngine();

	GobangAIEngine_Init(ai);
	GobangAIEngine_Work(ai);
	GobangAIEngine_Clear(ai);

	delete ai;
	return 0;
}