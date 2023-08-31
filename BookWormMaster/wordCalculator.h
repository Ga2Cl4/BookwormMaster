#pragma once

#include <Windows.h>
#include <list>
#include <map>
#include <iostream>
#include <fstream>

#define ROWS 8
#define COLUMNS 7

using namespace std;

typedef struct {
	string tiles;
	int colors[ROWS * COLUMNS];
} bkGraph;

typedef struct Solution {
	string word;
	list<int> path;
} solution;

extern void getDict();
extern solution solveGraphAndBonus(bkGraph graph, string bonus);
//extern solution solveGraph(bkGraph graph);