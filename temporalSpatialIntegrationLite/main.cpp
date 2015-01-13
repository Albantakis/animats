
#include <stdio.h>
#include <stdlib.h>

// For getpid function
#include <unistd.h>

#include <vector>
#include <map>
#include <math.h>
#include <time.h>
#include <iostream>
#include "globalConst.h"
#include "tHMM.h"
#include "tAgent.h"
#include "tGame.h"
#include <string.h>

#define randDouble ((double)rand()/(double)RAND_MAX)

using namespace std;

//double replacementRate=0.1;
double perSiteMutationRate=0.005;
int update=0;
int repeats=1; //1;
int maxAgent=100;
int totalGenerations=5000;
char trialName[1000];
double sensorNoise=0.0;

void saveLOD(tGame *game,tAgent *agent,FILE *statsFile,FILE *genomeFile);

int main(int argc, char *argv[])
{
	vector<tAgent*>agent;
	vector<tAgent*>nextGen;
	tAgent *masterAgent;
	int i,j,who=0;
	tGame *game;
	double maxFitness;
	FILE *resFile;
	FILE *LOD;
	FILE *genomeFile;
	LOD=fopen(argv[2],"w+t");
	genomeFile=fopen(argv[3],"w+t");
    int localSeed=atoi(argv[5]);
    if(localSeed!=-1)
        srand(localSeed);
    else
        srand(getpid());
	agent.resize(maxAgent);
	game=new tGame(argv[1]);
    game->nowUpdate=floor(totalGenerations/2); //Larissa
    sensorNoise=atof(argv[6]);
    /*
	masterAgent=new tAgent;
    masterAgent->loadAgent("evolved10TickData/GEN_56_LAST.txt");
    masterAgent->setupPhenotype();
    masterAgent->showPhenotype();
    masterAgent->saveToDotFullLayout("GEN_56_LAST.dot");
    game->executeGame(masterAgent, NULL);
    cout<<masterAgent->correct<<" "<<83*82<<endl;
    exit(0);
    //*/
//    masterAgent->loadAgent("best5100.txt");
//    masterAgent->setupPhenotype();
//    masterAgent->saveToDot("brain.dot");
//    game->executeGame(masterAgent, NULL);
//    cout<<masterAgent->correct<<endl;
    //game->computeAllMI(argv[1]);
    //exit(0);
    /*
    FILE *R=fopen("resultConnections5KRuns.txt","w+t");
    int I[maxNodes][34],O[maxNodes][34];
    for(i=0;i<34;i++)
        for(j=0;j<maxNodes;j++){
            I[j][i]=0; O[j][i]=0;
        }
    int _genToList=159;
    for(i=0;i<300;i++){
        char filename[1000];
        if(i<100)
            sprintf(filename,"/Users/arend/science/matlab/GCP/data10Ticks/GEN_%02i.txt",i);
        else
            sprintf(filename,"/Users/arend/science/matlab/GCP/data10Ticks/GEN_%03i.txt",i);
        FILE *F=fopen(filename,"r+t");
        cout<<filename<<endl;
        for(int l=0;l<_genToList;l++)
        {
            char data[1000000];
            fscanf(F,"%[^\n]\n",&data);
            FILE *G=fopen("dummy.txt","w+t");
            fprintf(G,"%s\n",data);
            fclose(G);
            masterAgent=new tAgent;
            masterAgent->loadAgent("dummy.txt");
            masterAgent->setupPhenotype();
            for(j=0;j<masterAgent->hmmus.size();j++){
                for(int k=0;k<masterAgent->hmmus[j]->ins.size();k++)
                    I[masterAgent->hmmus[j]->ins[k]][l]++;
                for(int k=0;k<masterAgent->hmmus[j]->outs.size();k++)
                    O[masterAgent->hmmus[j]->outs[k]][l]++;
            }
        }
    }
    for(int l=0;l<_genToList;l++){
        fprintf(R,"%i",l);
        for(j=0;j<maxNodes;j++)
            fprintf(R," %i",I[j][l]);
        for(j=0;j<maxNodes;j++)
            fprintf(R," %i",O[j][l]);
        fprintf(R,"\n");

    }
    exit(0);
    // */
    //masterAgent->loadAgent("TTB_genome.txt");
    masterAgent=new tAgent;
	masterAgent->setupRandomAgent(5000);
	masterAgent->setupPhenotype();
	for(i=0;i<agent.size();i++){
		agent[i]=new tAgent;
		agent[i]->inherit(masterAgent,0.01,0);
	}
	nextGen.resize(agent.size());
	masterAgent->nrPointingAtMe--;
	cout<<"setup complete"<<endl;
	while(update<totalGenerations){
		for(i=0;i<agent.size();i++){
			agent[i]->fitness=0.0;
			agent[i]->fitnesses.clear();
		}
		for(i=0;i<agent.size();i++){
			for(j=0;j<repeats;j++){
				game->executeGame(agent[i],NULL,sensorNoise,j);
				agent[i]->fitnesses.push_back((float)agent[i]->correct);
			}
		}
        if(update == 2500){
            for(i=0;i<agent.size();i++){
                cout<<agent[i]->correct<<endl;
                game->makeSingleAgentAnalysis(agent[i],argv[4],i);
            }
        }
            
            
		//fflush(resFile);
		maxFitness=0.0;

		for(i=0;i<agent.size();i++){
            agent[i]->fitness=1.0;
			for(j=0;j<repeats;j++)
                agent[i]->fitness*=agent[i]->fitnesses[j];
            //agent[i]->fitness=sqrt(agent[i]->fitness);
            if(repeats <= 1){
            //cout<<agent[i]->fitness<<endl;
            agent[i]->fitness=pow(1.02,agent[i]->fitness);  //Larissa: This for one repeat
            }else {
            agent[i]->fitness=pow(agent[i]->fitness,1.0/repeats);
            agent[i]->fitness=pow(1.02,agent[i]->fitness);
//            cout<<agent[i]->fitness<<endl;
            }
			if(agent[i]->fitness>maxFitness){
                who=i;
				maxFitness=agent[i]->fitness;
            }
		}
//		cout<<update<<" "<<(double)maxFitness<<" "<<agent[who]->correct<<" "<<(float)agent[who]->correct/1000.0<<endl;
		cout<<update<<" "<<(double)maxFitness<<" "<<agent[who]->correct<<"/"<<agent[who]->incorrect<<" "<<(float)agent[who]->correct/(83.0*82.0)<<endl;
		for(i=0;i<agent.size();i++)
		{
			tAgent *d;
			d=new tAgent;
			do{ j=rand()%(int)agent.size(); } while(randDouble>(agent[j]->fitness/maxFitness));
			d->inherit(agent[j],perSiteMutationRate,update);
			nextGen[i]=d;
		}
		for(i=0;i<agent.size();i++){
			agent[i]->retire();
			agent[i]->nrPointingAtMe--;
			if(agent[i]->nrPointingAtMe==0)
				delete agent[i];
			agent[i]=nextGen[i];
		}
		agent=nextGen;
		update++;
	}
//	agent[0]->saveLOD(LOD,genomeFile);
//    game->makeFullAnalysis(agent[0],argv[4],sensorNoise);
// Larissa: put noise to 0 for analysis
    game->makeFullAnalysis(agent[0],argv[4],0);
    saveLOD(game,agent[0],LOD,genomeFile);
	//agent[0]->ancestor->ancestor->saveGenome(genomeFile);
    return 0;
}

void saveLOD(tGame *game,tAgent *agent,FILE *statsFile,FILE *genomeFile){
    vector<tAgent*> list;
    tAgent *localAgent=agent;
    while(localAgent!=NULL){
        list.push_back(localAgent);
        localAgent=localAgent->ancestor;
    }
    for(int i=(int)list.size()-1;i>0;i--){
        agent=list[i];
        if((agent->born&LOD_record_Intervall)==0){
//            vector<vector<int> > T=game->executeGameLogStates(agent,sensorNoise);
//          Larissa: set noise to 0 for analysis
            vector<vector<int> > T=game->executeGameLogStates(agent,0);
            double R=game->computeRGiven(T[2], T[3], T[4], 4, 2, 4);
            fprintf(statsFile,"%i   %i  %i  %f",agent->born,agent->correct,agent->incorrect,R);
            for(int i=0;i<agent->differentialCorrects.size();i++)
                fprintf(statsFile," %i",agent->differentialCorrects[i]);
            fprintf(statsFile,"\n");
            agent->saveGenome(genomeFile);
        }
    }
}
