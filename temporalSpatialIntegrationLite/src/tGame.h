/*
 *  tGame.h
 *  HMMBrain
 *
 *  Created by Arend on 9/23/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
 
#ifndef _tGame_h_included_
#define _tGame_h_included_

#include "globalConst.h"
#include "tAgent.h"
#include <vector>
#include <map>
#include <set>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#define xDim 256
#define yDim 16
#define startMazes 1
#define cPI 3.14159265

class tGame{
public:
    vector<int> patterns;
	void executeGame(tAgent* agent,FILE *f,double sensorNoise,int repeat);
	tGame(char* filename);
	~tGame();
	double mutualInformation(vector<int> A,vector<int>B);
	double ei(vector<int> A,vector<int> B,int theMask);
	double computeAtomicPhi(vector<int>A,int states);
	double predictiveI(vector<int>A);
	double nonPredictiveI(vector<int>A);
	double predictNextInput(vector<int>A);
	double computeR(vector<vector<int> > table,int howFarBack);
	double computeOldR(vector<vector<int> > table);
	double entropy(vector<int> list);

    vector<vector<int> > executeGameLogStates(tAgent* agent,double sensorNoise);
	void analyseKO(tAgent* agent,int which, int setTo,double sensorNoise);

    void represenationPerNodeSummary(tAgent* agent,char* filename,double sensorNoise);
    void makeFullAnalysis(tAgent *agent,char *fileLead,double sensorNoise);
    double computeRGiven(vector<int>W,vector<int>S,vector<int>B,int nrWstates,int nrSstates,int nrBstates);
    void applyNoise(tAgent *agent,double sensorNoise);
    double agentDependentRandDouble(void);
    int agentDependentRandInt(void);
    int nowUpdate;
};
#endif