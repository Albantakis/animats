/*
 *  tAgent.h
 *  HMMBrain
 *
 *  Created by Arend on 9/16/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef _tAgent_h_included_
#define _tAgent_h_included_

#include "globalConst.h"
#include "tHMM.h"
#include <vector>
#include "tANN.h"
#include <stdio.h>
#include <stdlib.h>


//#define useANN

using namespace std;

static int masterID=0;


class tDot{
public:
	double xPos,yPos;
};


class tAgent{
public:
	vector<tHMMU*> hmmus;
	vector<unsigned char> genome;
	vector<tDot> dots;
#ifdef useANN
	tANN *ANN;
#endif
	
	tAgent *ancestor;
	unsigned int nrPointingAtMe;
	unsigned char states[maxNodes],newStates[maxNodes];
	double fitness,convFitness;
	vector<double> fitnesses;
	int food;
	
	double xPos,yPos,direction;
	double sX,sY;
	bool foodAlreadyFound;
	int steps,bestSteps,totalSteps;
	int ID,nrOfOffspring;
	bool saved;
	bool retired;
	int born;
	int correct,incorrect;
    vector<int> differentialCorrects;
	
	tAgent();
	~tAgent();
	void setupRandomAgent(int nucleotides);
	void loadAgent(char* filename);
	void loadAgentWithTrailer(char* filename);
	void setupPhenotype(void);
	void inherit(tAgent *from,double mutationRate,int theTime);
	unsigned char * getStatesPointer(void);
	void updateStates(void);
	void resetBrain(void);
	void ampUpStartCodons(void);
	void showBrain(void);
	void showPhenotype(void);
	void saveToDot(char *filename);
	void saveToDotFullLayout(char *filename);
	
	void initialize(int x, int y, int d);
	tAgent* findLMRCA(void);
	void saveFromLMRCAtoNULL(FILE *statsFile,FILE *genomeFile);
	void saveLOD(FILE *statsFile,FILE *genomeFile);
	void retire(void);
	void setupDots(int x, int y,double spacing);
	void saveLogicTable(FILE *f);
    void saveLogicTableSingleAnimat(FILE *f);
	void saveGenome(FILE *f);
    void saveEdgeList(char *filename);
};

#endif