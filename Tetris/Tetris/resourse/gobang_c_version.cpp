/****************************************************/
/** ���� ������AIʾ��     *************************/                                                                         
/** ���ߣ� jw@pku           *************************/
/** ���ڣ� 2010-12-9        *************************/
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <ctime>


/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    ȫ�ֲ�������ʼ��                              ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

#define OPENDEBUG 0                 // ���Կ���
#define INFINITY 1000000000         // �������

#define BOARDSIZE 15                // ���̴�С
#define DEPTH 4                     // �������
#define EXTERNWINDOW 3              // �����ӵĸ�����������ӵľ���<3

#define HASHSIZE (1 << 20)          // ���̹�ϣ���С 1024*1024
#define HASHMOD  1048573            // ���̹�ϣ��ģֵ

// �����ֵ
#define VALUE_WIN 10000             // ����
#define VALUE_WE_DOUBLE_THREE 2000  // �ҷ�˫����
#define VALUE_WE_THREE 200          // �ҷ�����
#define VALUE_THEY_DOUBLE_THREE 500 // �Է�˫����
#define VALUE_THEY_THREE 100        // �Է�����
#define VALUE_SLEEP_THREE 10        // ����
#define VALUE_TWO 4                 // ���
#define VALUE_SLEEP_TWO 1           // �߶�


int max(int a, int b) {
	return (a > b) ? a : b;
}

/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    ȫ�ֲ�����������                              ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/




/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    ChessMove�ṹ����ʼ��                         ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

// һ����Ľṹ
struct ChessMove {
	int x, y;   // ����λ��
	int color;  // ��һ����
};

/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    ChessMove�ṹ��������                         ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/


/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    ChessBoard�ṹ����ʼ��                         ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

// ���̵Ľṹ�����棩
struct ChessBoard {
	char data[BOARDSIZE][BOARDSIZE];    // ����
	ChessMove last_move;                // ��һ�����߷�
	int step;                           // �ܹ��µĲ���
};

// ��������������Ľṹ
struct BoardScore {
	int index;                          // ��������������
	int score;                          // �����˸����̵ĵ÷�
};

// �������̵ıȽϺ���
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
			return -1; //��ֵ��ȣ�index����
		} else if (((BoardScore*)a)->index > ((BoardScore*)b)->index) {
			return 1;
		} else {
			return 0;
		}
	}
}

/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    ChessBoard�ṹ��������                        ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    HashChessBoard��ϣ����ʼ��                  ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

// ���̹�ϣ��
struct HashChessBoard 
{
	struct HashNode {   	// һ����ϣ�ڵ�
		__int64 check_code;                   // У����
		int score;                            // ����ĵ÷�
	};
	HashNode hash[HASHSIZE];                 //2^20
	__int64 zob[2][BOARDSIZE][BOARDSIZE];     // zobrist��ϣ
	__int64 check[2][BOARDSIZE][BOARDSIZE];   // У������
};

// ��ʼ������
int HashChessBoard_Init(HashChessBoard *hash_chess_board);

// ����һ��hash�ڵ㣬����һ����ֺ����¿�����ȣ���¼�÷֡�
int HashChessBoard_Insert(HashChessBoard *hash_chess_board, const ChessBoard &chess_board, const int depth, const int score);

// ����һ��hash�ڵ㣬����һ����ֺ����¿�����ȣ�����÷֡�
int HashChessBoard_Find(HashChessBoard *hash_chess_board, const ChessBoard &chess_board, const int depth, int *score);

// ����һ����ֺ����¿�����ȣ��õ���ϣֵ��
int HashChessBoard_GetKey(HashChessBoard *hash_chess_board, const ChessBoard &chess_board, const int depth, __int64 *check_code);

// �õ�һ��64λ�������
__int64 HashChessBoard_Rand64();


int HashChessBoard_Init(HashChessBoard *hash_chess_board) {
	memset(hash_chess_board->hash, -1, sizeof(hash_chess_board->hash));

	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < BOARDSIZE; ++j) {
			for (int k = 0; k < BOARDSIZE; ++k) {
				hash_chess_board->zob[i][j][k] = HashChessBoard_Rand64();  //�������
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
	// �ȵõ���ϣֵ
	int key = HashChessBoard_GetKey(hash_chess_board, chess_board, depth, &check_code);//����key��check_code

	// ����ڵ�
	hash_chess_board->hash[key].check_code = check_code;
	hash_chess_board->hash[key].score = score;

	return 0;
}

int HashChessBoard_Find(HashChessBoard *hash_chess_board, const ChessBoard &chess_board, const int depth, int *score) {
	__int64 check_code;
	// �ȵõ���ϣֵ��
	int key = HashChessBoard_GetKey(hash_chess_board, chess_board, depth, &check_code);

	// ���У������ȣ���Ϊ���ڣ�������Ϊ�����ڡ�
	if (check_code == hash_chess_board->hash[key].check_code)
	{
		*score = hash_chess_board->hash[key].score;
		return 0;
	} else {
		return -1;
	}
}


// ����zobrist��ϣ����
int HashChessBoard_GetKey(HashChessBoard *hash_chess_board, const ChessBoard &chess_board, const int depth, __int64 *check_code) 
{
	__int64 key = depth;
	*check_code = 0;

	// �ֱ�õ���ϣֵ��У���롣
	int color = 3 - chess_board.last_move.color;  //ȡֵ 0,1,2
	for (int i = 0; i < BOARDSIZE; ++i) {
		for (int j = 0; j < BOARDSIZE; ++j) 
		{
			if (chess_board.data[i][j] != 0) //����
			{
				key ^= hash_chess_board->zob[chess_board.data[i][j] - 1][i][j]; //ȡ����Ӧλ�á���ɫ���������ֵ �������
				(*check_code) ^= hash_chess_board->check[chess_board.data[i][j] - 1][i][j];//ȡcheck code
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
/*****************                    HashChessBoard��ϣ��������                  ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/




/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    Evaluate�ࣨ��ʼ��                            ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

int iterator_num = 0;

struct Evaluate {
	int count_live[3][6];                                                 // ͳ�ƻ�������
	int count_sleep[3][6];                                                // ͳ����������
};

// ����һ�����̵ĵ÷֡�
int Evaluate_GetEvaluate(Evaluate *evaluate, const ChessBoard &chess_board, const int color);

// �ֽ�һ�����̲�������
int Evaluate_DecBoard(Evaluate *evaluate, const ChessBoard &chess_board);

// ͳ��һ�����̵Ĺ��֡�
int Evaluate_CountValue(Evaluate *evaluate, const ChessBoard &chess_board, const int color);

// ����������һ�С�
int Evaluate_AnalysisLine(Evaluate *evaluate, const int num, const char line[BOARDSIZE]);


int Evaluate_GetEvaluate(Evaluate *evaluate, const ChessBoard &chess_board, const int color) {
	
	++iterator_num; // ���Ƽ�������

	memset(evaluate->count_live, 0, sizeof(evaluate->count_live));
	memset(evaluate->count_sleep, 0, sizeof(evaluate->count_sleep));

	// �ȷֽ����̣��ټ���÷�
	Evaluate_DecBoard(evaluate, chess_board);
	return Evaluate_CountValue(evaluate, chess_board, color);
}


int Evaluate_DecBoard(Evaluate *evaluate, const ChessBoard &chess_board) {
	char line[BOARDSIZE];
	// ������
	for (int i = 0; i < BOARDSIZE; ++i) {		
		for (int j = 0; j < BOARDSIZE; ++j) {
			line[j] = chess_board.data[i][j];
		}
		Evaluate_AnalysisLine(evaluate, BOARDSIZE, line);
	}

	// ������
	for (int i = 0; i < BOARDSIZE; ++i) {
		for (int j = 0; j < BOARDSIZE; ++j) {
			line[j] = chess_board.data[j][i];
		}
		Evaluate_AnalysisLine(evaluate, BOARDSIZE, line);
	}

	// �������Խ���
	// ��������Ϊ5����
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

	// �������Խ���
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

	// �Ѿ��ֳ�ʤ�����������
	// �ҷ�������5���ҷ�ʤ��
	if (evaluate->count_live[color][5] + evaluate->count_sleep[color][5] > 0)
			return VALUE_WIN + 50;

	// �Է�������5���ҷ�ʤ��
	if (evaluate->count_live[3 - color][5] + evaluate->count_sleep[3 - color][5] > 0)
			return -VALUE_WIN - 50;

	// �������ĵ���һ������
	if (evaluate->count_sleep[color][4] >= 2)
		evaluate->count_live[color][4] = 1;

	if (evaluate->count_sleep[3 - color][4] >= 2)
		evaluate->count_live[3 - color][4] = 1;

	// �ҷ������ģ��ҷ�ʤ��
	if (evaluate->count_live[color][4] + evaluate->count_sleep[color][4] > 0)
		return VALUE_WIN + 40;

	// �ҷ�û�����ģ��ҶԷ��л��ģ��Է�ʤ��
	if (evaluate->count_live[3 - color][4] > 0)
		return -VALUE_WIN - 30;

	// �Է������ĺͻ������Է�ʤ��
	if (evaluate->count_live[3 - color][3] > 0 && evaluate->count_sleep[3 - color][4] > 0)
		return -VALUE_WIN - 20;

	// �ҷ��л����жԷ�û�����ĺͻ��ģ��ҷ�ʤ��
	if (evaluate->count_live[color][3] > 0 &&
		evaluate->count_sleep[3 - color][4] + evaluate->count_live[3 - color][4] == 0)
		return VALUE_WIN + 10;

	// �Է��д���1���������ҷ�û�����ĺ��������Է�ʤ��
	if (evaluate->count_live[3 - color][3] > 1 &&
		evaluate->count_live[color][4] + evaluate->count_sleep[color][4] + evaluate->count_live[color][3] + evaluate->count_sleep[color][3] == 0)
		return -VALUE_WIN;


    // �����������
	int value = 0;
	

	// һ�����ĵ���һ������
	if (evaluate->count_sleep[color][4] > 0)
		++evaluate->count_live[color][3];

	if (evaluate->count_sleep[3 - color][4] > 0)
		++evaluate->count_live[3 - color][3];

	// ���������ֵ
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

	// ����������ֵ
	value += (evaluate->count_sleep[color][3] - evaluate->count_sleep[3 - color][3]) * VALUE_SLEEP_THREE;
	// ��������ֵ
	value += (evaluate->count_live[color][2] - evaluate->count_live[3 - color][2]) * VALUE_TWO;
	// �����߶���ֵ
	value += (evaluate->count_sleep[color][2] - evaluate->count_sleep[3 - color][2]) * VALUE_SLEEP_TWO;

	// �������̷ֲ����
	for (int i = 0; i < BOARDSIZE; ++i) {
		for (int j = 0; j < BOARDSIZE; ++j) {
			if (chess_board.data[i][j] != 0) {
				int mul = (chess_board.data[i][j] == color) ? 1 : -1;
				value += (BOARDSIZE / 2 - max(abs(i - BOARDSIZE / 2), abs(j - BOARDSIZE / 2))) * mul;
			}
		}
	}

	// �������յ÷�
	return value;
}


int Evaluate_AnalysisLine(Evaluate *evaluate, const int num, const char line[BOARDSIZE]) {
	for (int i = 0; i < num; ++i) {
		if (line[i] == 0)
			continue;

		int j = i;
		for (; j + 1 < num && line[j + 1] == line[i]; ++j) {
		}

		// i -> j ������һ����ɫ������
		int len = (j - i + 1);
		bool sleep = false;

		// �ж��Ƿ�������
		if (i == 0 || j == num - 1 ||
			line[i - 1] == 3 - line[i] || line[j + 1] == 3 - line[i]) {
			sleep = true;
		}

		// �������ߵĿո���
		int blank_num = 0;
		for (int k = i - 1; k >= 0 && line[k] == 0; --k) {
			++blank_num;
		}
		for (int k = j + 1; k < num && line[k] == 0; ++k) {
			++blank_num;
		}

		// ����ܹ�����չ����С��5��������ͳ��
		if (len + blank_num >= 5) {

			// ��չ���ȸպ�Ϊ5����Ϊ������
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
/*****************                    Evaluate�ࣨ������                            ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/



/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    MoveGenerator�ࣨ��ʼ��                       ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

// �з�������
int MoveGenerator_CreatePossibleMove(const ChessBoard &chess_board, ChessBoard *board_list, int *list_num) {

	ChessBoard new_board;

	*list_num = 0;

	// ������ڵ�0����ֱ����(7, 7)�Ϸ�һ���ӣ������ء�
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

	// ���һ���ո����������ٶپ��� < EXTERNWINDOW �����ӣ�����Ϊ�ÿո������
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

						// ������
						if (chess_board.data[i + k][j + l] != 0) {
							flag = true;
							break;
						}
					}
					if (flag)
						break;
				}

				// �� (i, j)�Ϸ��ӡ�
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
/*****************                    MoveGenerator�ࣨ������                       ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/




/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    SearchMoveEngine�ࣨ��ʼ��                    ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

// һ��SearchMoveEngine��
struct SearchMoveEngine {
	int color;						// ��ǰ�ֵ�˭����
	ChessBoard *chess_board;		// ��ǰ������
	ChessMove *choose_chess_move;	// ����һ���·�

	Evaluate *evaluate;							// ��ֵ�ṹ
	HashChessBoard *hash_chess_board;			// ���̹�ϣ��
	int history_score[BOARDSIZE][BOARDSIZE];	// ��ʷ�����÷ֱ�
};

// ��ʼ��һ��SearchMoveEngine��
int SearchMoveEngine_Init(SearchMoveEngine *search_move_engine, const int color);

// �������ڴ�
int SearchMoveEngine_Clear(SearchMoveEngine *search_move_engine);

// ��AIѡһ���߷�
int SearchMoveEngine_MoveByAI(SearchMoveEngine *search_move_engine);

// ֱ�Ӵ�����ֵ��߷�
int SearchMoveEngine_MoveByRival(SearchMoveEngine *search_move_engine, const ChessMove &chess_move);

// �ڵ�ǰ����£�����һ������
int SearchMoveEngine_MoveOneStep(SearchMoveEngine *search_move_engine, const ChessMove &chess_move);

// ���и�����ֵ�ݹ�������
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
	// ���ø�����ֵ�ݹ��������õ��ⷨ��
	SearchMoveEngine_NegamaxSearch(search_move_engine, *search_move_engine->chess_board, 1, - INFINITY, INFINITY);
	// �������庯������һ����
	SearchMoveEngine_MoveOneStep(search_move_engine, *search_move_engine->choose_chess_move);
	printf("%d %d\n", search_move_engine->chess_board->last_move.x, search_move_engine->chess_board->last_move.y);
	if (OPENDEBUG) {
		printf("iterator num %d\n", iterator_num);
	}
	return 0;
}

int SearchMoveEngine_MoveByRival(SearchMoveEngine *search_move_engine, const ChessMove &chess_move) 
{
	// ֱ�ӵ������庯������һ����
	return SearchMoveEngine_MoveOneStep(search_move_engine, chess_move);
}

int SearchMoveEngine_MoveOneStep(SearchMoveEngine *search_move_engine, const ChessMove &chess_move) 
{
	search_move_engine->chess_board->data[chess_move.x][chess_move.y] = chess_move.color;
	search_move_engine->chess_board->last_move = chess_move;
	++search_move_engine->chess_board->step;
	return 0;
}

// ������ֵ����
int SearchMoveEngine_NegamaxSearch(SearchMoveEngine *search_move_engine, const ChessBoard &chess_board, const int depth, const int alpha, const int beta) 
{
	int score;

	// ���ù�ϣ���ų��ظ��������
	if (HashChessBoard_Find(search_move_engine->hash_chess_board, chess_board, depth, &score) == 0)
		return score;

	// �Ե�ǰ��ֹ�ֵ�������ȴﵽ DEPTH + 1 �����Ѿ�ʤ�����򷵻�
	score = Evaluate_GetEvaluate(search_move_engine->evaluate, chess_board, 3 - chess_board.last_move.color);
	if (depth == DEPTH + 1 || depth > 1 && abs(score) > VALUE_WIN) {
		HashChessBoard_Insert(search_move_engine->hash_chess_board, chess_board, depth, score);
		return score;
	}

	// �������е��߷�
	int list_num;
	ChessBoard board_list[BOARDSIZE * BOARDSIZE];
	MoveGenerator_CreatePossibleMove(chess_board, board_list, &list_num);

	// ����ÿһ������ʷ�����÷ֲ�����
	BoardScore board_score_list[BOARDSIZE * BOARDSIZE];
	for (int i = 0; i < list_num; ++i) 
	{
		board_score_list[i].index = i;
		board_score_list[i].score =
			search_move_engine->history_score[board_list[i].last_move.x][board_list[i].last_move.y];
	}
	qsort(board_score_list, list_num, sizeof(BoardScore), BoardScoreCmp);

	// ��ʼ NegamaxSearch ����
	int new_alpha = alpha;
	ChessMove best_move;
	best_move.x = best_move.y = -1;

	score = -INFINITY;

	for (int i = 0; i < list_num; ++i) {
		int index = board_score_list[i].index;

		// �ݹ���ø�����ֵ������
		int result = -SearchMoveEngine_NegamaxSearch(search_move_engine, board_list[index], depth + 1, - beta, - new_alpha);

		if (result > score) {
			// ��¼����߷�
			best_move = board_list[index].last_move;
			score = result;
		}

		// beta��֦ (��ʵ���������൱��alpha��֦)
		if (score >= beta) {
			break;
		}

		// ��¼��߷֣��������Ų���
		if (score > new_alpha) {
			new_alpha = score;
		}
	}

	// ������ʷ�÷�����
	if (best_move.x != -1) {
		search_move_engine->history_score[best_move.x][best_move.y] += (1 << (DEPTH - depth));
	}

	// ����ڵ�һ�㣬��Ҫ����������߷�
	if (depth == 1) {
		*search_move_engine->choose_chess_move = best_move;
	}

	// ����ǰ��ֵķ��������ϣ��
	HashChessBoard_Insert(search_move_engine->hash_chess_board, chess_board, depth, score);

	// �������÷�
	return score;
}

/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    SearchMoveEngine�ࣨ������                    ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/




/*****************************************************************************************************/
/*****************                                                                  ******************/
/*****************                    GobangAIEngine�ࣨ��ʼ��                      ******************/
/*****************                                                                  ******************/
/*****************************************************************************************************/

// �ܵ�AI������
struct GobangAIEngine {
	SearchMoveEngine *search_move_engine;
	int color;
};

// ��ʼ��������
int GobangAIEngine_Init(GobangAIEngine *ai);

// �ͷ�����ڴ档
int GobangAIEngine_Clear(GobangAIEngine *ai);

// ���幤���߼���
int GobangAIEngine_Work(GobangAIEngine *ai);

int GobangAIEngine_Init(GobangAIEngine *ai) {
	ai->search_move_engine = NULL;

	char command[10];
	scanf("%s", command);

	if (strcmp(command, "[START]") != 0)
		return -1;

	// ������ɫ
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
		
		// �������µ���
		ChessMove chess_move;
		scanf("%d%d", &chess_move.x, &chess_move.y);
		chess_move.color = 3 - ai->search_move_engine->color;

		if (chess_move.x != -1) {
			// ���ݶ����µ�һ����ı��Լ������
			SearchMoveEngine_MoveByRival(ai->search_move_engine, chess_move);
		}

		int time1 = clock();
		// �����Լ����������棬��һ����
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
/*****************                    GobangAIEngine�ࣨ������                      ******************/
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