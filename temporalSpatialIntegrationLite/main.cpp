// Implementation of Predictive Coding.
//  error units hidden 23 , update units 45
//2 sensors

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <math.h>
#include <time.h>
#include <iostream>
#include <algorithm>
#include "globalConst.h"
#include "tHMM.h"
#include "tAgent.h"
#include "tGame.h"
#include <string.h>
#include <unistd.h>

#define randDouble ((double)rand()/(double)RAND_MAX)

using namespace std;


//double replacementRate=0.1;
double perSiteMutationRate=0.005;
int update=0; //counter of generations
int repeats=1; //1;
int maxAgent=100;
int totalGenerations= 10515;
char trialName[1000];
double sensorNoise=0.0;
bool combinedfitness=false;
double magic_nb=1.02; // 1.02 for regular fitness, 1.1
//int vFitnessFunction=1;
//extern const int step=2;  //predictor of motors from sensors, sensor t = motor t+step
//double wpc=0.0;
//double wmi=0.3;
//double wco=0.7;


void saveLOD(tGame *game,tAgent *agent,FILE *statsFile,FILE *genomeFile);
//Save the line to descent
int main(int argc, char *argv[])
{
	vector<tAgent*>agent;
	vector<tAgent*>nextGen;
	tAgent *masterAgent;
    //   int vFitness_Function;
    //  vFitness_Function=1; // 0 GA, 1 MI ...

	int i,j,who=0;
    int who_F=0;
    int who_PC=0;
    int who_CF=0;
    int myfactor= 56;
    //double myranddouble;
	tGame *game;

    int countpczeros;
    bool exit;
    double minpcele;
    countpczeros=0;
    //   wmi=1-wpc;
	double maxFitness, mutinfmaxFitness,maxcombinedfitness,origprecoderr,origmutinf;
    double a,b,pcminerror; //Max error for predicitve coding
    // linear relationship between corrects and pred coding error in absolute value
    a=0.0938;
    b=44;
    //linear relationship between corrects and mutual information
    //56=0x + y  128=2x + y
    origprecoderr=0.0;
    origmutinf=0.0;
    //Linear rel. pred coding error for 2 sensors
    // correct= predcoderror* apc + bpc, where apc=18 bpc=56
    int apc=18;
    int bpc =56;
	//FILE *resFile;
	FILE *LOD;
	FILE *genomeFile;
	LOD=fopen(argv[2],"w+t"); //trial0_LOD.txt LOD + agent->born,agent->correct,agent->incorrect,R
	genomeFile=fopen(argv[3],"w+t"); //trial0_GEN.txt
    int localSeed=atoi(argv[5]);
    if(localSeed!=-1)
        srand(localSeed);
    else
        srand(getpid());
    agent.resize(maxAgent);
    game=new tGame(argv[1]); //argv[1]=basic.txt (size of 4 blocks falling r and l) size size size size
    game->nowUpdate=floor(totalGenerations/2); //Larissa Rounds downward, returning the largest integral value.NEVER USED
    sensorNoise=atof(argv[6]); //parses string to float


    masterAgent=new tAgent;
	masterAgent->setupRandomAgent(5000);
	masterAgent->setupPhenotype();
	for(i=0;i<agent.size();i++){
		agent[i]=new tAgent;
		agent[i]->inherit(masterAgent,0.01,0);
	}
	nextGen.resize(agent.size());
	masterAgent->nrPointingAtMe--;

    if (vFitnessFunction==1){
        //wpc=0.0;
        if (combinedfitness)
            cout<<"Maximizing for combined Fitness"<< " wcorrect=" << wco << " wmi=" << wmi << endl;
        else cout<<"Maximizing only for Fitness"<<endl;
    }
    if (vFitnessFunction==2){
        if (combinedfitness)
            cout<<"Maximizing for combined Predictive Coding"<< " wcorrect=" << wco << " wmi=" << wmi << " wpc=" << wpc << endl;
        else cout<<"Maximizing only for Predictive Coding"<<endl;
    }
    cout << "Maximizing for "<< vFitnessFunction <<" for Numsensors=" << NumSensors << endl;
	cout<<"setup complete"<<endl;
    //
    //Begining of the game for totalGenerations
	while(update<totalGenerations){
		for(i=0;i<agent.size();i++){
            //initialize to 0 all agents fitness
			agent[i]->agfitness=0.0;
			agent[i]->fitnesses.clear();
            //initialize to NEW fitness measures: mutinfagent, mutinffitnesses
            agent[i]->mutinfagent=0.0;
			agent[i]->mutinffitnesses.clear();
            //
            //initialize to NEW fitness pred coding
            agent[i]->predcoderror=0.0;
			agent[i]->vecpredcoderror.clear();
            agent[i]->glucose_level.clear();
		}
		for(i=0;i<agent.size();i++){
			for(j=0;j<repeats;j++){
                //exec game for each time repeats times
                //each agent has a fitnesses or mutinffitnesses vector of repeats elements
                //cout <<"previo executegame"<<endl;
				game->executeGame(agent[i],NULL,sensorNoise,j);
				//cout <<"post executegame "<< (float)agent[i]->mutinfagent<<"agenti= " << i << endl;
                agent[i]->fitnesses.push_back((float)agent[i]->correct); //Building the vector of fitnesses: vector<double> fitnesses
                // Calculate new fitnesses for mut inf
                origmutinf=agent[i]->mutinfagent;
                if (NumSensors == 2)
                    agent[i]->mutinffitnesses.push_back((float)agent[i]->mutinfagent*cc +dd);
                else // 3 sensors
                    agent[i]->mutinffitnesses.push_back((float)agent[i]->mutinfagent*24 +56);
                //Predicitve coding
                agent[i]->origprecoderr= (float) agent[i]->predcoderror ;
                //cout <<agent[i]->predcoderror<<agent[i]->predcoderror*apc + bpc<<endl;

                // Glucose project
                if (glucose_project == 1) //glucose_node  > NumSensors identical condition
                 agent[i]->glucose_level.push_back((agent[i]->glucose_current_value));

                if (vFitnessFunction ==2){
                    if (NumSensors==3)
                        agent[i]->vecpredcoderror.push_back( (float) agent[i]->predcoderror*a + b);
                    else if (NumSensors==2) agent[i]->vecpredcoderror.push_back( (float) agent[i]->predcoderror*apc + bpc);
                }
                //cout << "----" <<  agent[i]->predcoderror <<"  "<<(float) agent[i]->predcoderror*apc + bpc<<endl;
                //cout <<"agent="<<i<< " predcoderror="<<agent[i]->predcoderror<<endl;
			}// end repetitions of the same game

		} // end games for all agents
        maxFitness=0.0;
        mutinfmaxFitness=0.0;
        maxcombinedfitness=0.0;

        //
		//Calculate (OLD) the maxFitness for the agents of an actual generation
		for(i=0;i<agent.size();i++){
            agent[i]->fitnessmif=1.0;
            agent[i]->agfitness=1.0;
			for(j=0;j<repeats;j++){
                //calculate the initial fitness of each agent as the product of its fitnesses vector (repeats elements)
                agent[i]->fitnessmif*=agent[i]->mutinffitnesses[j];
                agent[i]->agfitness*=agent[i]->fitnesses[j];
            //agent[i]->fitness=sqrt(agent[i]->fitness);
            // if the fitness function F needs to change is here, 1.02*F
            }
            if(repeats <= 1){ //repeat =1
                //cout<<agent[i]->fitness<<endl;
                 agent[i]->fitnessmif=pow(1.02,agent[i]->fitnessmif);
                 agent[i]->agfitness=pow(1.02,agent[i]->agfitness);  //Larissa: This for one repeat
            }else {
                agent[i]->fitnessmif=pow(agent[i]->fitnessmif,1.0/repeats); //for many repeats 1
                agent[i]->fitnessmif=pow(1.02,agent[i]->fitnessmif);
                agent[i]->agfitness=pow(agent[i]->agfitness,1.0/repeats);
                agent[i]->agfitness=pow(1.02,agent[i]->agfitness); //
                //            cout<<agent[i]->fitness<<endl;
            }
            double localprod=agent[i]->agfitness*agent[i]->fitnessmif;
            if (combinedfitness==true) {
                if(localprod > maxFitness){ // seems to be 0 most of the time, at the beginning
                    who=i; // if agent is not 0 fitness me lo guardo en who to access to its correct and incorrect
                    maxFitness=agent[i]->agfitness*agent[i]->fitnessmif ;
                }
            }
            else {
                if(agent[i]->fitnessmif > maxFitness){ // seems to be 0 most of the time, at the beginning
                    who=i; // if agent is not 0 fitness me lo guardo en who to access to its correct and incorrect
                    maxFitness = agent[i]->fitnessmif ;
                }
            }
        }
		 //End for loop that calculates the maxfitness
		cout<<" GENERATION "<< update<<" "<<(double)maxFitness<<" "<<agent[who]->correct<<"/"<<agent[who]->incorrect<<" "<<(float)agent[who]->correct/(83.0*82.0)<<" "<<who<<endl;
        cout <<  "fitness mif " << agent[who]->fitnessmif << " correct "<< agent[who]->correct<< " agfitness " << agent[who]->agfitness << " Combined F=" << agent[who]->fitnessmif*agent[j]->agfitness<< endl;
        if (glucose_project ==1)
            cout << "glucose current value "<< agent[who]->glucose_current_value << " glucose level threshold "<< agent[who]->glucose_threshold <<endl;
        //

        //Calculate NEW Fitness for Predictive Coding
        // GEt the second minimum value, to avoid having huge majority of 0 error



        //Inherit for Fitness as MI
        if (vFitnessFunction==1 && !combinedfitness) {

            double minf=pow(magic_nb,56);
            double maxagentsfitness=minf;
            double avgfit=0;
            // int maxglucoselevel = INT_MIN;
            //int glucose_average = 0;
            int rightside = 0;
            //int leftside = rightside;
            //Calculate agents in the nth quartile of glucose level
            //int glucose_total = 0;
            //int median = agent[0]->glucose_level[0];
            int maxgluxoselevel = -128;

            int size = sizeof(a)/sizeof(int);
            int totalglucose= 0;
            vector<int> glucosas;
            vector<int> agent_index_inh; // aggents that passed to the next generation (propagate)
            glucosas.clear();
            agent_index_inh.clear();
            for(i = 0;i < agent.size();i++) {
                glucosas.push_back(agent[i]->glucose_level[0]);
                totalglucose+=agent[i]->glucose_level[0];
            }

            double avgglucose=totalglucose/maxAgent;
            totalglucose =0 ;
            std::sort(&glucosas[0],&glucosas[100]);
            double median = 100 % 2 ? glucosas[100 / 2] : (glucosas[100 / 2 - 1] + glucosas[100 / 2]) / 2;

            for(i = 0;i < agent.size();i++) {
                if (agent[i]->glucose_level[0] > maxgluxoselevel)  maxgluxoselevel =  agent[i]->glucose_level[0] ;
            }
            cout << "the median = " << median << " maximum " << maxgluxoselevel <<   endl;
            for(i=0;i<agent.size();i++) {

                if (agent[i]->fitness > maxagentsfitness) maxagentsfitness=agent[i]->fitness;
                avgfit+=agent[i]->fitness;
            }

            avgfit=avgfit/100;


            int count_its = 0 ; // iteraor counter to avoif get stuck in the do while loop, if I check for then leaves the loop

            for(i=0;i<agent.size();i++)
            {// choose one random agent, agent[j] with fitness larger than some random value randDouble
                tAgent *d;
                d=new tAgent;

                do{ j=rand()%(int)agent.size(); count_its++;}

                while( (randDouble>(agent[j]->fitnessmif/maxagentsfitness)  || (agent[j]->glucose_level[0] < (avgglucose - avgglucose*0.25))) && (count_its < maxAgent));

                agent_index_inh.push_back(j);
                cout << "Inherited agent " << j << " Fitness " << agent[j]->fitnessmif << " Ratio Ag_F/Max_F=" << agent[j]->fitness/maxagentsfitness << " Glucose " << agent[j]->glucose_level[0] << " Init level " <<  agent[j]->glucose_init_level << " Average " << avgglucose <<   " Median " << median << endl;
                count_its=0;
                d->inherit(agent[j],perSiteMutationRate,update);
                nextGen[i]=d;
                // cout << "NEW next gen" << d<<endl;;
            } //create the new pool of Agents nextGen
            for(i=0;i<agent.size();i++){
                agent[i]->retire();
                agent[i]->nrPointingAtMe--;
                if(agent[i]->nrPointingAtMe==0)
                    delete agent[i];
                agent[i]=nextGen[i];
            }
        } //End if Inherit fitness as MI


        //Inherit for Fitness Predicitive Coding

        if (combinedfitness) {
            for(i=0;i<agent.size();i++)
            {// choose one random agent, agent[j] with fitness larger than some random value randDouble
                tAgent *d;
                d=new tAgent;
                do{ j=rand()%(int)agent.size(); } while(randDouble>((agent[j]->fitnessmif*agent[j]->agfitness)/maxFitness));

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
        }
		agent=nextGen; // both agent and nextGen are vectors of tAgent vector<tAgent*>nextGen
		update++;
	}  // While End Generations loop

    //	agent[0]->saveLOD(LOD,genomeFile);
    //    game->makeFullAnalysis(agent[0],argv[4],sensorNoise);
    // Larissa: put noise to 0 for analysis
    // Call only once to print out the results in trial0_LOD (nb of catches and falls and  R) and trial0_GEN
    //makeFullAnalysis is called once, and creates 5 files one for each 512 generations, starting from last agent it goes though its ancestors
    game->makeFullAnalysis(agent[0],argv[4],0); // argv[4] = 0 random seed
    // if (vFitnessFunction==1) {
    //    game->makeFullAnalysisMI(agent[0],argv[4],0);
    //  }
    saveLOD(game,agent[0],LOD,genomeFile);
	//agent[0]->ancestor->ancestor->saveGenome(genomeFile);
    return 0;
}

//Build the LOD file goiing back all the ancestors of the last agent
void saveLOD(tGame *game,tAgent *agent,FILE *statsFile,FILE *genomeFile){
    vector<tAgent*> list;
    tAgent *localAgent=agent;
    double R;
    vector <double> vR;
    while(localAgent!=NULL){
        list.push_back(localAgent);
        localAgent=localAgent->ancestor;
    }
    for(int i=(int)list.size()-1;i>0;i--){
        agent=list[i];
        if((agent->born&LOD_record_Intervall)==0){ // bitwise comparison agent->born(update)&511,  LOD_record_Intervall=511
            //            vector<vector<int> > T=game->executeGameLogStates(agent,sensorNoise);
            //          Larissa: set noise to 0 for analysis
            vector<vector<int> > T=game->executeGameLogStates(agent,0); //invoke tGame::executeGameLogStates with sensorNoise=0
            // T retValue, value of the afent states along the game (36) ticks  T0,1 are sensors, T6,7 are motors, only read hidden
            //if (vFitnessFunction==1) R=game->computeRGivenMI(T[2], T[3], T[4], T[5], 4, 2, 4,2);
            //else R=game->computeRGiven(T[2], T[3], T[4], 4, 2, 4); // world conditions, sensors, hidden units
            // R=game->computeRGiven(T[2], T[3], T[4], 4, 2, 4);
            vR=game->computeMultipleRGiven(T[2], T[3], T[4], 4, 2, 4);
            //fprintf(statsFile,"%i   %i  %i   %f %f %f %f %f %f %f",agent->born,agent->correct,agent->incorrect,agent->sentropy, agent->mutinfagent, agent->agfitness, agent->fitnessmif,agent->origprecoderr, agent->predcoderror, agent->agcombinedfitness);
            if (glucose_project ==1)
                 fprintf(statsFile,"%i %i %i %f %f %f %f %f %f %f %f",agent->born,agent->correct,agent->incorrect,agent->sentropy, agent->mutinfagent, agent->fitnessmif, agent->agfitness ,agent->distance, agent->energy, agent->glucose_current_value, R);
            else
                fprintf(statsFile,"%i %i %i %f %f %f %f %f %f %f",agent->born,agent->correct,agent->incorrect,agent->sentropy, agent->mutinfagent, agent->fitnessmif, agent->agfitness ,agent->distance, agent->energy, R);
            // trial0_LOD  nb_generation, nb corrects, nb wrong, R, corrb1, coorrb2, corrb3,corrb4 . note nb corr+sum corr per each of the 4 tasks as denided in basic.txt, max 32 .
            //For example, for basic.txt  1	3 1	3 , blocks falling of that size from left to right and right to left, 32 for each game instance
            for(int i=0;i<agent->differentialCorrects.size();i++)
                fprintf(statsFile," %i",agent->differentialCorrects[i]);
            // vector<int> differentialCorrects array with the nb of corrects for each game instance (max 32), the incorrect is just 32-that
            fprintf(statsFile,"\n");
            agent->saveGenome(genomeFile); // write the genome in the file,  vector<unsigned char> genome is an attribute of tAgent
        }
    }
    cout << "Ending List of Agents" << (int)list.size() << " " << endl;
    cout << "GENOME simlation COMPLETED"<<endl;
}
