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

//
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
    //
	double fitness,convFitness,sentropy, energy, distance;
    // Agent properties for glucose project
    //hitvector 128 compoents with -1 if block missed +1 (0 neutral) if block catched. Note in this schema we do not conider the std fitness of catching vs avoiding. This schema can be modified
    vector<int> hitvector;
    //initial level of glucose for an agent, this attribute isused if the agent can mobe for having a glucose level beyond a threhold
    int glucose_init_level;
    //the minimum value of glucose required for making the agent moves
    int glucose_threshold; //
    
    //glucose_value= sum(hitvector) + glucose_init_level
    int glucose_current_value;
    // agent moves if motors && glucose_threshold <= glucose
    vector<int> distancestates_list, glucose_level, glucose_level_list;
    
    
	vector<double> fitnesses;
    // new (correct) index for mutual information
    double mutinfagent,fitnessmif,agcombinedfitness,origprecoderr,agfitness;
    //
    // new fitnesses for mutual information
    vector<double> mutinffitnesses;
    //
    //Predicitve coding
    double predcoderror;
    vector<double> vecpredcoderror;
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
	void saveGenome(FILE *f);
    void saveEdgeList(char *filename);
};

#endif