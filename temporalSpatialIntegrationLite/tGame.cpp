/*
 *  tGame.cpp
 *  HMMBrain
 *
 *  Created by Arend on 9/23/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "tGame.h"
#include <math.h>

#define KISSRND (((((rndZ=36969*(rndZ&65535)+(rndZ>>16))<<16)+(rndW=18000*(rndW&65535)+(rndW>>16)) )^(rndY=69069*rndY+1234567))+(rndX^=(rndX<<17), rndX^=(rndX>>13), rndX^=(rndX<<5)))
#define INTABS(number) (((((0x80)<<((sizeof(int)-1)<<3))&number) ? (~number)+1 : number))

#define randDouble ((double)rand()/(double)RAND_MAX)

int rndX,rndY,rndZ,rndW;


tGame::tGame(char* filename){
    FILE *f=fopen(filename,"r+w");
    int i,j;
    patterns.clear();
    while(!feof(f)){
        fscanf(f,"%i  ",&i);
        patterns.push_back(i&65535);
    }
    fclose(f);
}

tGame::~tGame(){
}

double tGame::agentDependentRandDouble(void){
    int A=KISSRND;
    return (double)((INTABS(A))&65535)/(double)65535;
}
int tGame::agentDependentRandInt(void){
    int A=KISSRND;
    return (INTABS(A));
}


void tGame::applyNoise(tAgent *agent,double sensorNoise){
    //if(agentDependentRandDouble()<sensorNoise){
    if(randDouble<sensorNoise)  //Larissa: If I don't have noise in evaluation, then I can just use random numbers always
          agent->states[0]=!agent->states[0];
    //if(agentDependentRandDouble()<sensorNoise)
    if(randDouble<sensorNoise)
        agent->states[1]=!agent->states[1];
}

void tGame::executeGame(tAgent* agent,FILE *f,double sensorNoise, int repeat){
    int world,botPos,blockPos;
    int i,j,k,l,m;
    unsigned char W;
    int action;
    rndW=agent->ID+repeat; // make random seeds unique from one another by including index
	rndX=~(agent->ID+repeat);
	rndY=(agent->ID+repeat)^0b01010101010101010101010101010101;
	rndZ=(agent->ID+repeat)^0b10101010101010101010101010101010;
    //cout<<rndZ<<endl;

    agent->fitness=1.0;
    agent->correct=agent->incorrect=0;
    bool hit;
    agent->differentialCorrects.resize(patterns.size());
    for(i=0;i<agent->differentialCorrects.size();i++)
        agent->differentialCorrects[i]=0;
    for(i=0;i<patterns.size();i++){
        for(j=-1;j<2;j+=2){
            for(k=0;k<16;k++){
                //Larissa: Change environment after 30,000 Gen, if patterns is 1 7 15 3 it changes 
                //from 2 blocks with 1 7 to 4 blocks with 1 7 15 3 
                //if (agent->born > nowUpdate || i<2){
//                if (agent->born > nowUpdate){
                    world=patterns[i];
//                    //cout<<world<<endl;
//                } else{
//                    //world=patterns[i-2];
//                    if (i == 0 || i == 2) world=7;
//                    else if (i==1 || i == 3) world=15;
//                    //cout<<world<<endl;
//                }    
                agent->resetBrain();
                botPos=k;
                blockPos=0;
                //loop the world
                for(l=0;l<loopTicks;l++){
//                    for(m=0;m<16;m++)
//                        printf("%i",(world>>m)&1);
//                    printf("\n");
                    //AH: Sensors have no noise in them now
                    agent->states[0]=(world>>botPos)&1;
//                    agent->states[1]=0;                      
                    agent->states[1]=(world>>((botPos+2)&15))&1;
                    //Larissa: Set to 0 to evolve animats with just one sensor
//                    if (agent->born > nowUpdate){
//                        agent->states[0]=0;
//                        agent->states[1]=(world>>((botPos+2)&15))&1;
//                    }
                    //AH: apply noise does apply noise to them now
                    applyNoise(agent, sensorNoise);
                    // set motors to 0 to preven reading from them
                    agent->states[6]=0; agent->states[7]=0;
                    agent->updateStates();
                    //Larissa: limit to one Motor
                    //agent->states[7]=0;
//                    if (agent->born < nowUpdate){
//                        agent->states[7]=0;
//                    }
                    action=agent->states[6]+(agent->states[7]<<1);
                   // action=0; //Larissa: this makes the animat stop moving
                    switch(action){
                        case 0:
                        case 3:// nothing! 
                            break;
                        case 1:
                            botPos=(botPos+1)&15;
                            break;
                        case 2:
                            botPos=(botPos-1)&15;
                            break;
                    }
                    if(j==-1){
                        world=((world>>1)&65535)+((world&1)<<15);
                    } else {
                        world=((world<<1)&65535)+((world>>15)&1);
                    }
                }
                //check for hit
                hit=false;
                for(m=0;m<3;m++)
                    if(((world>>((botPos+m)&15))&1)==1)
                        hit=true;
                if((i&1)==0){
                    if(hit){
                        agent->correct++;
                        agent->fitness*=1.01;
                        agent->differentialCorrects[i]++;
                    } else {
                        agent->fitness/=1.01;
                        agent->incorrect++;
                    }
                } else {
                    if(hit){
                        agent->incorrect++;
                        agent->fitness/=1.01;
                    } else {
                        agent->correct++;
                        agent->fitness*=1.01;
                        agent->differentialCorrects[i]++;
                    }
                }
            }
        }
            
    }
}
vector<vector<int> > tGame::executeGameLogStates(tAgent* agent,double sensorNoise){
    int world,botPos,blockPos;
    int i,j,k,l,m,T0,T1,u;
    unsigned char W;
    int action;
  	vector<vector<int> > retValue;
    agent->fitness=1.0;
    agent->correct=agent->incorrect=0;
    rndW=agent->ID; // make random seeds unique from one another by including index
	rndX=~agent->ID;
	rndY=agent->ID^0b01010101010101010101010101010101;
	rndZ=agent->ID^0b10101010101010101010101010101010;
    bool hit;
    retValue.clear();
    retValue.resize(6);
    for(i=0;i<patterns.size();i++){
        for(j=-1;j<2;j+=2){
            for(k=0;k<16;k++){
                //Larissa: Change environment after 30,000 Gen
                //if (agent->born > nowUpdate || i<2){
//                if (agent->born > nowUpdate){
                   world=patterns[i];
//                    //cout<<world<<endl;
//                } else{
//                    //world=patterns[i-2];
//                    if (i == 0 || i == 2) world=7;
//                    else if (i==1 || i == 3) world=15;
//                    //cout<<world<<endl;
//                }    
                agent->resetBrain();
                botPos=k;
                blockPos=0;
                //loop the world
                for(l=0;l<loopTicks;l++){
                    //                    for(m=0;m<16;m++)
                    //                        printf("%i",(world>>m)&1);
                    //                    printf("\n");
                    
                    //AH: Sensors have no noise in them now
                    agent->states[0]=(world>>botPos)&1;
                    agent->states[1]=(world>>((botPos+2)&15))&1;
//                    agent->states[1]=0;                      

//                    //Larissa: Set to 0 to evolve animats with just one sensor
//                    if (agent->born > nowUpdate){
//                        agent->states[0]=0;
//                          agent->states[1]=(world>>((botPos+2)&15))&1;
//                    }
                    //AH: apply noise does apply noise to them now
                    applyNoise(agent, sensorNoise);  
                    // set motors to 0 to preven reading from them
                    agent->states[6]=0; agent->states[7]=0;
                    T0=0;
					for(u=0;u<8;u++)
						T0|=(agent->states[u]&1)<<u;
					agent->updateStates();
                    T1=0;
					for(u=0;u<8;u++)
						T1|=(agent->states[u]&1)<<u;
					int action=(agent->states[maxNodes-1])+(agent->states[maxNodes-2]<<1);
                    retValue[0].push_back(T0);
                    retValue[1].push_back(T1);
                    int W=i<<1;
                    if(j!=-1)
                        W|=1;
                    retValue[2].push_back(W);
                    retValue[3].push_back((T0>>6)&3);
                    retValue[4].push_back((T1>>2)&15);
                    retValue[5].push_back(T1&3);
                    //Larissa: limit to one Motor
                    //agent->states[7]=0;
//                    if (agent->born < nowUpdate){
//                        agent->states[7]=0;
//                    }
                    action=agent->states[6]+(agent->states[7]<<1);
                    switch(action){
                        case 0:
                        case 3:// nothing! 
                            break;
                        case 1:
                            botPos=(botPos+1)&15;
                            break;
                        case 2:
                            botPos=(botPos-1)&15;
                            break;
                    }
                    if(j==-1){
                        world=((world>>1)&65535)+((world&1)<<15);
                    } else {
                        world=((world<<1)&65535)+((world>>15)&1);
                    }
                }
                //check for hit
                hit=false;
                for(m=0;m<3;m++)
                    if(((world>>((botPos+m)&15))&1)==1)
                        hit=true;
                if((i&1)==0){
                    if(hit){
                        agent->correct++;
                        agent->fitness*=1.01;
                    } else {
                        agent->fitness/=1.01;
                        agent->incorrect++;
                    }
                } else {
                    if(hit){
                        agent->incorrect++;
                        agent->fitness/=1.01;
                    } else {
                        agent->correct++;
                        agent->fitness*=1.01;
                    }
                }
            }
        }
        
    }
    return retValue;
}

void tGame::analyseKO(tAgent* agent,int which, int setTo,double sensorNoise){
    int world,botPos,blockPos;
    int i,j,k,l,m;
    unsigned char W;
    int action;
    agent->fitness=1.0;
    agent->correct=agent->incorrect=0;
    rndW=agent->ID; // make random seeds unique from one another by including index
	rndX=~agent->ID;
	rndY=agent->ID^0b01010101010101010101010101010101;
	rndZ=agent->ID^0b10101010101010101010101010101010;
    bool hit;
    for(i=0;i<patterns.size();i++){
        for(j=-1;j<2;j+=2){
            for(k=0;k<16;k++){
                //Larissa: Change environment after 30,000 Gen
                //if (agent->born > nowUpdate || i<2){
//                if (agent->born > nowUpdate){
                    world=patterns[i];                  
//                    //cout<<world<<endl;
//                } else{
//                    //world=patterns[i-2];
//                    if (i == 0 || i == 2) world=7;
//                    else if (i==1 || i == 3) world=15;
//                    //cout<<world<<endl;
//                }    
                agent->resetBrain();
                botPos=k;
                blockPos=0;
                //loop the world
                for(l=0;l<loopTicks;l++){
                    //                    for(m=0;m<16;m++)
                    //                        printf("%i",(world>>m)&1);
                    //                    printf("\n");
                    //AH: Sensors have no noise in them now
                    agent->states[0]=(world>>botPos)&1;
                    agent->states[1]=(world>>((botPos+2)&15))&1;
//                    //Larissa: Set to 0 to evolve animats with just one sensor
//                    agent->states[1]=0;                
//                    if (agent->born > nowUpdate){
//                        agent->states[0]=0;
//                        agent->states[1]=(world>>((botPos+2)&15))&1;
//                    }
                    //AH: apply noise does apply noise to them now
                    applyNoise(agent, sensorNoise);
                    // set motors to 0 to preven reading from them
                    agent->states[6]=0; agent->states[7]=0;
                    agent->states[which]=setTo;
                    agent->updateStates();
                    //Larissa: limit to one Motor
                    //agent->states[7]=0;
//                    if (agent->born < nowUpdate){
//                        agent->states[7]=0;
//                    }
                    action=agent->states[6]+(agent->states[7]<<1);
                    switch(action){
                        case 0:
                        case 3:// nothing! 
                            break;
                        case 1:
                            botPos=(botPos+1)&15;
                            break;
                        case 2:
                            botPos=(botPos-1)&15;
                            break;
                    }
                    if(j==-1){
                        world=((world>>1)&65535)+((world&1)<<15);
                    } else {
                        world=((world<<1)&65535)+((world>>15)&1);
                    }
                }
                //check for hit
                hit=false;
                for(m=0;m<3;m++)
                    if(((world>>((botPos+m)&15))&1)==1)
                        hit=true;
                if((i&1)==0){
                    if(hit){
                        agent->correct++;
                        agent->fitness*=1.01;
                    } else {
                        agent->fitness/=1.01;
                        agent->incorrect++;
                    }
                } else {
                    if(hit){
                        agent->incorrect++;
                        agent->fitness/=1.01;
                    } else {
                        agent->correct++;
                        agent->fitness*=1.01;
                    }
                }
            }
        }
        
    }
}


double tGame::mutualInformation(vector<int> A,vector<int>B){
	set<int> nrA,nrB;
	set<int>::iterator aI,bI;
	map<int,map<int,double> > pXY;
	map<int,double> pX,pY;
	int i,j;
	double c=1.0/(double)A.size();
	double I=0.0;
	for(i=0;i<A.size();i++){
		nrA.insert(A[i]);
		nrB.insert(B[i]);
		pX[A[i]]=0.0;
		pY[B[i]]=0.0;
	}
	for(aI=nrA.begin();aI!=nrA.end();aI++)
		for(bI=nrB.begin();bI!=nrB.end();bI++){
			pXY[*aI][*bI]=0.0;
		}
	for(i=0;i<A.size();i++){
		pXY[A[i]][B[i]]+=c;
		pX[A[i]]+=c;
		pY[B[i]]+=c;
	}
	for(aI=nrA.begin();aI!=nrA.end();aI++)
		for(bI=nrB.begin();bI!=nrB.end();bI++)
			if((pX[*aI]!=0.0)&&(pY[*bI]!=0.0)&&(pXY[*aI][*bI]!=0.0))
				I+=pXY[*aI][*bI]*log2(pXY[*aI][*bI]/(pX[*aI]*pY[*bI]));
	return I;
	
}

double tGame::entropy(vector<int> list){
	map<int, double> p;
	map<int,double>::iterator pI;
	int i;
	double H=0.0;
	double c=1.0/(double)list.size();
	for(i=0;i<list.size();i++)
		p[list[i]]+=c;
	for (pI=p.begin();pI!=p.end();pI++) {
			H+=p[pI->first]*log2(p[pI->first]);	
	}
	return -1.0*H;
}

double tGame::ei(vector<int> A,vector<int> B,int theMask){
	set<int> nrA,nrB;
	set<int>::iterator aI,bI;
	map<int,map<int,double> > pXY;
	map<int,double> pX,pY;
	int i,j;
	double c=1.0/(double)A.size();
	double I=0.0;
	for(i=0;i<A.size();i++){
		nrA.insert(A[i]&theMask);
		nrB.insert(B[i]&theMask);
		pX[A[i]&theMask]=0.0;
		pY[B[i]&theMask]=0.0;
	}
	for(aI=nrA.begin();aI!=nrA.end();aI++)
		for(bI=nrB.begin();bI!=nrB.end();bI++){
			pXY[*aI][*bI]=0.0;
		}
	for(i=0;i<A.size();i++){
		pXY[A[i]&theMask][B[i]&theMask]+=c;
		pX[A[i]&theMask]+=c;
		pY[B[i]&theMask]+=c;
	}
	for(aI=nrA.begin();aI!=nrA.end();aI++)
		for(bI=nrB.begin();bI!=nrB.end();bI++)
			if((pX[*aI]!=0.0)&&(pY[*bI]!=0.0)&&(pXY[*aI][*bI]!=0.0))
				I+=pXY[*aI][*bI]*log2(pXY[*aI][*bI]/(pY[*bI]));
	return -I;
}
double tGame::computeAtomicPhi(vector<int>A,int states){
	int i;
	double P,EIsystem;
	vector<int> T0,T1;
	T0=A;
	T1=A;
	T0.erase(T0.begin()+T0.size()-1);
	T1.erase(T1.begin());
	EIsystem=ei(T0,T1,(1<<states)-1);
	P=0.0;
	for(i=0;i<states;i++){
		double EIP=ei(T0,T1,1<<i);
//		cout<<EIP<<endl;
		P+=EIP;
	}
//	cout<<-EIsystem+P<<" "<<EIsystem<<" "<<P<<" "<<T0.size()<<" "<<T1.size()<<endl;
	return -EIsystem+P;
}



double tGame::computeR(vector<vector<int> > table,int howFarBack){
	double Iwh,Iws,Ish,Hh,Hs,Hw,Hhws,delta,R;
	int i;
	for(i=0;i<howFarBack;i++){
		table[0].erase(table[0].begin());
		table[1].erase(table[1].begin());
		table[2].erase(table[2].begin()+(table[2].size()-1));
	}
	table[4].clear();
	for(i=0;i<table[0].size();i++){
		table[4].push_back((table[0][i]<<14)+(table[1][i]<<10)+table[2][i]);
	}
	Iwh=mutualInformation(table[0],table[2]);
    Iws=mutualInformation(table[0],table[1]);
    Ish=mutualInformation(table[1],table[2]);
    Hh=entropy(table[2]);
    Hs=entropy(table[1]);
    Hw=entropy(table[0]);
    Hhws=entropy(table[4]);
    delta=Hhws+Iwh+Iws+Ish-Hh-Hs-Hw;
    R=Iwh-delta;
  	return R;
}

double tGame::computeOldR(vector<vector<int> > table){
	double Ia,Ib;
	Ia=mutualInformation(table[0], table[2]);
	Ib=mutualInformation(table[1], table[2]);
	return Ib-Ia;
}

double tGame::predictiveI(vector<int>A){
	vector<int> S,I;
	S.clear(); I.clear();
	for(int i=0;i<A.size();i++){
		S.push_back((A[i]>>12)&15);
		I.push_back(A[i]&3);
	}
	return mutualInformation(S, I);
}

double tGame::nonPredictiveI(vector<int>A){
	vector<int> S,I;
	S.clear(); I.clear();
	for(int i=0;i<A.size();i++){
		S.push_back((A[i]>>12)&15);
		I.push_back(A[i]&3);
	}
	return entropy(I)-mutualInformation(S, I);
}
double tGame::predictNextInput(vector<int>A){
	vector<int> S,I;
	S.clear(); I.clear();
	for(int i=0;i<A.size();i++){
		S.push_back((A[i]>>12)&15);
		I.push_back(A[i]&3);
	}
	S.erase(S.begin());
	I.erase(I.begin()+I.size()-1);
	return mutualInformation(S, I);
}


void tGame::represenationPerNodeSummary(tAgent* agent,char* filename,double sensorNoise){
    vector<vector<int> > table=executeGameLogStates(agent,sensorNoise);
    int W,B;
    int i,j;
    double R;
    int bitsumW,bitsumB;
    double maxR;
    int minP,bestPartition;
    FILE *F=fopen(filename,"w+t");
    vector<int> world,sensors,brain;
    cout<<"fitness of agent: "<<agent->correct<<endl;
    world.resize(table[0].size());
    sensors.resize(table[0].size());
    brain.resize(table[0].size());
    for(W=1;W<16;W=W<<1){
        maxR=0.0;
        bestPartition=0;
        for(B=1;B<16;B++){
            //cout<<W<<" "<<B<<" ";
            //fprintf(F,"%i   %i  ",W,B);
            bitsumW=0;
            bitsumB=0;
            for(i=0;i<4;i++){
                //cout<<((W>>i)&1)<<" ";
                //fprintf(F,"%i   ",((W>>i)&1));
                bitsumW+=(W>>i)&1;
            }
            for(i=0;i<4;i++){
                //cout<<((B>>i)&1)<<" ";
                //fprintf(F,"%i   ",((B>>i)&1));
                bitsumB+=(B>>i)&1;
            }
            //cout<<bitsumW<<" "<<bitsumB<<" ";
            //fprintf(F,"%i   %i  ",bitsumW,bitsumB);
            for(j=0;j<table[0].size();j++){
                world[j]=table[2][j]&W;
                sensors[j]=table[3][j];
                brain[j]=table[4][j]&B;
            }
            R=computeRGiven(world, sensors, brain, 4,2,4);
            if(R<0.0) R=0.0;
            if(R==maxR){
                if(bitsumB<minP){
                    minP=bitsumB;
                    bestPartition=B;
                }
            }
            if(R>maxR){
                maxR=R;
                minP=bitsumB;
                bestPartition=B;
            }
            //cout<<R<<endl;
            //fprintf(F,"%f\n",R);
        }
        cout<<W<<" "<<bestPartition<<" ";
        fprintf(F,"%i   %i  ",W,bestPartition);
        for(i=0;i<4;i++){
            cout<<((W>>i)&1)<<" ";
            fprintf(F,"%i   ",((W>>i)&1));
        }
        for(i=0;i<4;i++){
            cout<<((bestPartition>>i)&1)<<" ";
            fprintf(F,"%i   ",((bestPartition>>i)&1));
        }
        cout<<R<<endl;
        fprintf(F,"%f\n",R);
    }
    fclose(F);
}

void tGame::makeFullAnalysis(tAgent *agent,char *fileLead,double sensorNoise){
    char filename[1000];
    FILE *f;
    int i,j;
    vector<vector<int> > table;
    while(agent!=NULL){
        if((agent->born&LOD_record_Intervall)==0){
            //representation table
            sprintf(filename,"%s_%i_representation.txt",fileLead,agent->born);
            represenationPerNodeSummary(agent, filename,sensorNoise);
            //state to state table
            sprintf(filename,"%s_%i_FullLogicTable.txt",fileLead,agent->born);
            f=fopen(filename,"w+t");
            agent->saveLogicTable(f);
            fclose(f);
            //state to state table for only the lifetime
            sprintf(filename,"%s_%i_LifetimeLogicTable.txt",fileLead,agent->born);
            f=fopen(filename,"w+t");
            table=this->executeGameLogStates(agent,sensorNoise);
            for(i=0;i<8;i++)
                fprintf(f,"T0_%i,",i);
            fprintf(f,",");
            for(i=0;i<8;i++)
                fprintf(f,"T1_%i,",i);
            fprintf(f,"\n");
            for(j=0;j<table[0].size();j++){
                //printf("%i  %i\n",table[0][j],table[1][j]);
                for(i=0;i<8;i++)
                    fprintf(f,"%i,",(table[0][j]>>i)&1);
                fprintf(f,",");
                for(i=0;i<8;i++)
                    fprintf(f,"%i,",(table[1][j]>>i)&1);
                fprintf(f,"\n");
            }
            fclose(f);
            //ko table
            sprintf(filename,"%s_%i_KOdata.txt",fileLead,agent->born);
            f=fopen(filename,"w+t");
            executeGameLogStates(agent,sensorNoise);
            fprintf(f,"%i",agent->correct);
            for(i=0;i<8;i++)
                for(j=0;j<2;j++){
                    analyseKO(agent,i,j,sensorNoise);
                    fprintf(f,"	%i",agent->correct);
                }
            fprintf(f,"\n");
            fclose(f);
            //dot file
            sprintf(filename,"%s_%i_EdgeList.txt",fileLead,agent->born);
            agent->saveEdgeList(filename);
        }
        agent=agent->ancestor;
    }
}

void tGame::makeSingleAgentAnalysis(tAgent *agent,char *fileLead, int agent_num){
    char filename[1000];
    FILE *f;
    int i,j;
    //state to state table
    sprintf(filename,"%s_%i_%i_FullLogicTable.txt",fileLead,agent->born,agent_num);
    f=fopen(filename,"w+t");
    agent->saveLogicTableSingleAnimat(f);
    fclose(f);
    //fitness value
    sprintf(filename,"%s_%i_Fitness.txt",fileLead,agent->born);
    if(agent_num==0)
        f=fopen(filename,"w+t");
    else
        f=fopen(filename,"a");
    fprintf(f,"%i",agent->correct);
    fprintf(f,"\n");
    fclose(f);
    //dot file
    sprintf(filename,"%s_%i_%i_EdgeList.txt",fileLead,agent->born, agent_num);
    agent->saveEdgeList(filename);
}

double tGame::computeRGiven(vector<int>W,vector<int>S,vector<int>B,int nrWstates,int nrSstates,int nrBstates){
	double Iwh,Iws,Ish,Hh,Hs,Hw,Hhws,delta,R;
	int i;
    vector<int> total;
	total.clear();
	for(i=0;i<W.size();i++){
		total.push_back((W[i]<<(nrBstates+nrWstates))+(S[i]<<nrBstates)+B[i]);
	}
	Iwh=mutualInformation(W,B);
    Iws=mutualInformation(W,S);
    Ish=mutualInformation(S,B);
    Hh=entropy(B);
    Hs=entropy(S);
    Hw=entropy(W);
    Hhws=entropy(total);
    delta=Hhws+Iwh+Iws+Ish-Hh-Hs-Hw;
    R=Iwh-delta;
  	return R;
}






