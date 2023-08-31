#include "wordCalculator.h"

using namespace std;

list<string> DICT[26];

/* ---------------- TABLES ---------------- */
/*
DWORD scoreTable[26] = { // ��ĸA-Z������
    0, 1, 1, 1, 0, 2, 1,
    2, 0, 3, 2, 1, 1, 1,
    0, 1, 10, 1, 0, 1, 1,
    2, 2, 6, 2, 6
};
*/
// ��ĸA-Z������
map<char, int> scoreTable = {
    {'A',0} , {'B',1} , {'C',1} , {'D',1} , {'E',0} , {'F',2} , {'G',1} , {'H',2} , {'I',0} , {'J',3} ,
    {'K',2} , {'L',1} , {'M',1} , {'N',1} , {'O',0} , {'P',1} , {'Q',10} , {'R',1} , {'S',0} , {'T',1} ,
    {'U',1} , {'V',2} , {'W',2} , {'X',6} , {'Y',2} , {'Z',6} ,
};

// ������ɫ����Ķ����
float colorScoreTable[6] = {
    0, 1.5, 2.5, 1000, 4, 6
};

// adjTable ����ÿ����������ڷ��飬��һ��Ԫ��Ϊ����
DWORD adjTable[56][7] = {
    {0},
    {3, 7, 8, 9},  // 1
    {0},
    {3, 9, 10, 11},  // 3
    {0},
    {3, 11, 12, 13},  // 5
    {0},
    {3, 1, 8, 14},  // 7
    {6, 1, 7, 9, 14, 15, 16},  // 8
    {5, 1, 3, 8, 10, 16},  // 9
    {6, 3, 9, 11, 16, 17, 18},  // 10
    {5, 3, 5, 10, 12, 18},  // 11
    {6, 5, 11, 13, 18, 19, 20},  // 12
    {3, 5, 12, 20},  // 13
    {4, 7, 8, 15, 21},  // 14
    {6, 8, 14, 16, 21, 22, 23},  // 15
    {6, 8, 9, 10, 15, 17, 23},  // 16
    {6, 10, 16, 18, 23, 24, 25},  // 17
    {6, 10, 11, 12, 17, 19, 25},  // 18
    {6, 12, 18, 20, 25, 26, 27},  // 19
    {4, 12, 13, 19, 27},  // 20
    {4, 14, 15, 22, 28},  // 21
    {6, 15, 21, 23, 28, 29, 30},  // 22
    {6, 15, 16, 17, 22, 24, 30},  // 23
    {6, 17, 23, 25, 30, 31, 32},  // 24
    {6, 17, 18, 19, 24, 26, 32},  // 25
    {6, 19, 25, 27, 32, 33, 34},  // 26
    {4, 19, 20, 26, 34},  // 27
    {4, 21, 22, 29, 35},  // 28
    {6, 22, 28, 30, 35, 36, 37},  // 29
    {6, 22, 23, 24, 29, 31, 37},  // 30
    {6, 24, 30, 32, 37, 38, 39},  // 31
    {6, 24, 25, 26, 31, 33, 39},  // 32
    {6, 26, 32, 34, 39, 40, 41},  // 33
    {4, 26, 27, 33, 41},  // 34
    {4, 28, 29, 36, 42},  // 35
    {6, 29, 35, 37, 42, 43, 44},  // 36
    {6, 29, 30, 31, 36, 38, 44},  // 37
    {6, 31, 37, 39, 44, 45, 46},  // 38
    {6, 31, 32, 33, 38, 40, 46},  // 39
    {6, 33, 39, 41, 46, 47, 48},  // 40
    {4, 33, 34, 40, 48},  // 41
    {4, 35, 36, 43, 49},  // 42
    {6, 36, 42, 44, 49, 50, 51},  // 43
    {6, 36, 37, 38, 43, 45, 51},  // 44
    {6, 38, 44, 46, 51, 52, 53},  // 45
    {6, 38, 39, 40, 45, 47, 53},  // 46
    {6, 40, 46, 48, 53, 54, 55},  // 47
    {4, 40, 41, 47, 55},  // 48
    {3, 42, 43, 50},  // 49
    {3, 43, 49, 51},  // 50
    {5, 43, 44, 45, 50, 52},  // 51
    {3, 45, 51, 53},  // 52
    {5, 45, 46, 47, 52, 54},  // 53
    {3, 47, 53, 55},  // 54
    {3, 47, 48, 54},  // 55
};

/* ---------------- read dict from file ---------------- */

void getDict()
{
    ifstream f;
    CHAR buf[100];
    f.open("D:\\code_and_proj\\vsproj\\BookWormMaster\\Release\\dict.txt");
    if (!f.is_open())
    {
        printf("error in opening dict file\n");
        return;
    }
    while (!f.eof())
    {
        f.getline(buf, 100);
        //printf("%s", line);
        int index = buf[0] - 'A';
        DICT[index].push_back(buf);
    }
    f.close();
}

/* ---------------- get the longest word ---------------- */

BOOL find(list<int> l, int num)
{
    list<int>::iterator iter = l.begin();
    for (; iter != l.end(); iter++)
    {
        if (*iter == num)
        {
            return TRUE;
        }
    }
    return FALSE;
}

// ��ȡ��ƴ���ĵ�������
// table : ��ǰ�İ���
// tile : ��ǰ�����ķ���λ��
// participants : ��ǰʣ��ĺ�ѡ�����б�
// index : ��ǰ����ƫ��
// path : ���߹���·��(index�б�)
void getLink(string table, int tile, list<string> participants, DWORD index, list<int> path, list<solution> *result)
{
    int i;
    string word; //��ǰ�����ĵ���
    list<string>::iterator iterP1, iterP2; // participants iter
    list<int>::iterator iterN; // neighbors iter
    solution currentSolution; 

    // Ϊ·����ӵ�ǰ�ڵ�
    path.push_back(tile);
    // ��ȡ��ǰtile����������tile
    int adjCount = adjTable[tile][0];
    if (adjCount < 3)
    {
        printf("error in getting adjancents\n");
        exit(1);
    }
    // �г����л�û�б�ʹ�õ�tile
    list<int> neighbors;
    for (i = 1; i <= adjCount; i++)
    {
        if (!find(path, adjTable[tile][i]))
        {
            neighbors.push_back(adjTable[tile][i]);
        }
    }
    // ��ֹ��������·����
    if (neighbors.empty())
    {
        word.clear();
        for (iterP1 = participants.begin(); iterP1 != participants.end(); iterP1++)
        {
            if ((*iterP1).length() == index)
            {
                word = *iterP1;
                break;
            }
        }
        if (!word.empty())
        {
            // append path
            currentSolution.word = word;
            currentSolution.path = path;
            result->push_back(currentSolution);
        }
        // recursion end
    }
    // �п��ƶ�·������������·��
    else
    {
        for (iterN = neighbors.begin(); iterN != neighbors.end(); iterN++)
        {
            char next = table[*iterN]; // ��ǰ��������ĸ
            list<string> newParticipants;
            // ��ȡ�µĺ�ѡ�ʵ�
            for (iterP1 = participants.begin(); iterP1 != participants.end(); iterP1++)
            {
                word = *iterP1;
                if ((word.length() >= (index + 1)) && (next == word[index]))
                {
                    newParticipants.push_back(word);
                }
            }
            // ��ֹ������ƴ���������ĵ���
            if (newParticipants.empty())
            {
                word.clear();
                for (iterP2 = participants.begin(); iterP2 != participants.end(); iterP2++)
                {
                    if ((*iterP2).length() == index)
                    {
                        word = *iterP2;
                        break;
                    }
                }
                if (!word.empty())
                {
                    // append path
                    currentSolution.word = word;
                    currentSolution.path = path;
                    result->push_back(currentSolution);
                }
                // recursion end
            }
            // ����ƴ�������ĵ��ʣ���������
            else
            {
                if (next == 'Q')
                {
                    getLink(table, *iterN, newParticipants, index + 2, path, result);
                }
                else
                {
                    getLink(table, *iterN, newParticipants, index + 1, path, result);
                }
            }
        }
    }
}

solution solveGraph(bkGraph graph)
{
    // ��߷�
    double maxScore = 0;
    string maxWord = "";
    list<int> maxPath;
    // ��ʱ����
    string word;
    list<string> participants;
    list<int> path;
    list<solution> result;
    int tile = 0; // ��ǰtile���
    //
    string::iterator iterS1, iterS2;
    list<solution>::iterator iterR; // result iter
    list<int>::iterator iterP; // path iter
    // ��ǰ������Ϣ
    string tiles = graph.tiles;
    int *colors = graph.colors;

    for (iterS1 = tiles.begin(); iterS1 != tiles.end(); iterS1++)
    {
        // �����һ�ֻ������
        path.clear();
        result.clear();

        CHAR c = *iterS1;
        // tiles��0,2,4,6�Ŀ�λʹ������0���棬�˴���鵽���־ͽ�����һ��ѭ��
        if (isdigit(c)) {
            tile++;
            continue;
        }
        int i = c - 'A';
        if (i < 0 || i > 25)
        {
            printf("do not use lower case in dict file\n");
            exit(1);
        }
        participants = DICT[i]; // ��python��ͬ��C++��listֱ�Ӹ�ֵ�ǿ��еģ���������list���Ϊ������ͬ�Ķ���
        // ����ĸQ����������
        int index = 1;
        if (c == 'Q') index = 2;
        // ��ȡ����    
        getLink(tiles, tile, participants, index, path, &result);
        // ��������ҳ���ǰѭ������߼�ֵ��
        if (!result.empty())
        {
            for (iterR = result.begin(); iterR != result.end(); iterR++)
            {
                double score = 0.0; // �ܷ�
                double exscore = 0.0; // ���ӷ֣�����tile��ɫ��tileλ�õ����صķ���
                word = iterR->word;
                path = iterR->path;
                for (iterS2 = word.begin(); iterS2 != word.end(); iterS2++)
                {
                    score += scoreTable[*iterS2];
                }
                // calculate exscore
                for (iterP = path.begin(); iterP != path.end(); iterP++)
                {
                    exscore += colorScoreTable[colors[*iterP]];
                }
                // TODO strategy
                score = 0.2 * score + word.length() + exscore;
                // ������߷�
                if (score > maxScore)
                {
                    maxScore = score;
                    maxWord = word;
                    maxPath = path;
                }
            }
        }
        // �ƶ�����һ��tile
        tile++;
    }
    // ������߷�
    //cout << maxWord << endl << maxScore << endl;
    solution s;
    s.word = maxWord;
    s.path = maxPath;
    return s;
}

solution solveGraphAndBonus(bkGraph graph, string bonus)
{
    string maxWord = "";
    list<int> maxPath;
    list<int> path;
    list<solution> result;
    int tile = 0; // ��ǰtile���
    // ��ǰ������Ϣ
    string tiles = graph.tiles;
    // �������ֵ䣬ֻ��һ����Ա
    list<string> bonusDict;
    bonusDict.push_back(bonus);

    string::iterator iterS;
    list<solution>::iterator iterR;
    for (iterS = tiles.begin(); iterS != tiles.end(); iterS++)
    {
        // �����һ�ֻ������
        path.clear();
        result.clear();

        CHAR c = *iterS;
        // ����ĸQ����������
        int index = 1;
        if (c == 'Q') index = 2;
        // tiles��0,2,4,6�Ŀ�λʹ������0���棬�˴���鵽���־ͽ�����һ��ѭ�� || ��ǰ�ַ���bonus word���ַ���ͬ��ֱ����һ��
        if (isdigit(c) || c != bonus[0]) {
            tile++;
            continue;
        }
        // ��ȡ����    
        getLink(tiles, tile, bonusDict, index, path, &result);
        if (!result.empty())
        {
            for (iterR = result.begin(); iterR != result.end(); iterR++)
            {
                string word = iterR->word;
                list<int> path = iterR->path;
                solution s;
                if (word == bonus)
                {
                    s.word = word;
                    s.path = path;
                    return s;
                }
            }
        }
        // �ƶ�����һ��tile
        tile++;
    }
    // û��bonus word���������
    return solveGraph(graph);
}