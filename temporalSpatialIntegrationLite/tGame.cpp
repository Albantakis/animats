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
#include <iomanip>

#define KISSRND (((((rndZ=36969*(rndZ&65535)+(rndZ>>16))<<16)+(rndW=18000*(rndW&65535)+(rndW>>16)) )^(rndY=69069*rndY+1234567))+(rndX^=(rndX<<17), rndX^=(rndX>>13), rndX^=(rndX<<5)))
#define INTABS(number) (((((0x80)<<((sizeof(int)-1)<<3))&number) ? (~number)+1 : number))

#define randDouble ((double)rand()/(double)RAND_MAX)

int rndX,rndY,rndZ,rndW;
bool errorabs=true;


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
//game->executeGame(agent[i],NULL,sensorNoise,j);
void tGame::executeGame(tAgent* agent,FILE *f,double sensorNoise, int repeat){
    int world,botPos,blockPos;
    int i,j,k,l,m,ll;
    int action;
    rndW=agent->ID+repeat; // make random seeds unique from one another by including index
	rndX=~(agent->ID+repeat);
	rndY=(agent->ID+repeat)^0b01010101010101010101010101010101;
	rndZ=(agent->ID+repeat)^0b10101010101010101010101010101010;

    int sensors [loopTicks][NumSensors];
    int motors [loopTicks][2];
    int hiddensL1 [loopTicks][2], hiddensL2 [loopTicks][2], hiddenLayer[loopTicks][3];
    
    //parameters of residual error
    double wh2,wh1;
    double mutinfT;
    double residualerror,residualerrorH1,residualerrorH2,totalsqerror, mypredic,myupdate;
    //vector of 36x1 with sensor and motorstates for 36 ticks
    std::vector<int> listsensors,listmotors,listhiddensL1, listhiddensL2,listhiddenLayer;
    std::vector<int> actionvector; // if not move(action 0 or 3) 0,1,-1
    listsensors.clear();
    listmotors.clear();
    listhiddensL1.clear();
    listhiddensL2.clear();
    listhiddenLayer.clear();
    actionvector.clear();
    wh1=1;
    wh2=1-wh1;
    mutinfT=0;
    int numsensors=2;
    int nummotors=2;
    //step=0; //distance by which sensors are predictors of motors
    // max entropy of sm vector H(s0) + h(m0)+ h(s1) + h(m1)=4 bits
    agent->fitness=1.0;
    //initialize mut inf agent
    agent->mutinfagent=1.0;
    //
    agent->correct=agent->incorrect=0;
    //initialize total squared error
    agent->predcoderror=1.0;
    totalsqerror=0;
    listhiddensL1.clear();
    listhiddensL2.clear();
    // Glucose project
    actionvector.clear();
    //Initialize values for glucose level
    agent->glucose_current_value=agent->glucose_init_level;
    agent->hitvector.clear();
    agent->hitvector.shrink_to_fit();
    int counterhits=0; // counter 0..128 number of times the block has fallen
    bool hit;
    agent->differentialCorrects.resize(patterns.size());
    for(i=0;i<agent->differentialCorrects.size();i++)
        agent->differentialCorrects[i]=0;
    for(i=0;i< patterns.size();i++){ // 4, each for block size specified in basic.txt
        for(j=-1;j<2;j+=2){ // 2 options, left and right
            // Initialize motors[0]=0 because sensors_t = motors_t+1
          for(k=0;k<16;k++){ // 16 possible initial positions (16 horizontal points) k SHOULD BE RANDOM
                //Larissa: Change environment after 30,000 Gen
//                if (agent->born > nowUpdate || i<2){
                //world is block size
                world=patterns[i];
//                } else{
//                    world=patterns[i-2];
//                }    
                agent->resetBrain();
                botPos=k;// initial position of the bot of 16 possibles
                blockPos=0; //this is a constant?
                //loop the world
                //initialize first motor state stop
                //motors[0][0]= 1;
                //motors[0][1]= 1;
                //Initialize
              
                //listsensors.clear();
                residualerror=0;
                residualerrorH1=0;
                residualerrorH2=residualerrorH1;
                for(l=0;l<loopTicks;l++){
                    //AH: Sensors have no noise in them now
                    //value of sensors at t=1 (t,1,36) based on k (botpos) and block size(world), block always start falling in pos 0
                    agent->states[0]=(world>>botPos)&1;
                    // agent->states[1]=0;
                    agent->states[1]=(world>>((botPos+2)&15))&1;
                    sensors[l][0]=agent->states[0];
                    sensors[l][1]=agent->states[1];
                    //3 sensors
                    if (NumSensors==3) {agent->states[2]=(world>>((botPos+1)&15))&1; sensors[l][2]=agent->states[2];}
                    //Update internal node that encodes the glucose level
                    if (counterhits > 0) {
                        // if hitvector[counterhits-1] == -1 last game bot failed to get the block
                        if (agent->hitvector[counterhits-1] == -1) agent->states[glucose_node]=0;
                        // if hitvector[counterhits-1] == 1 last game bot got the block and increased its energy
                        if (agent->hitvector[counterhits-1] == 1)  agent->states[glucose_node]=1;
                    }
                    applyNoise(agent, sensorNoise);
                    // set motors to 0 to preven reading from them
                    //initialize mmotors initial value to 0
                    agent->states[6]=0; agent->states[7]=0;
                    agent->updateStates();

                    action=agent->states[6]+(agent->states[7]<<1);
                    
                   // action=0; //Larissa: this makes the animat stop moving
                    switch(action){
                        case 0:
                        case 3:// nothing!
                            actionvector.push_back(0);
                            break;
                        case 1:
                            //move bot on the right
                            if (glucose_project ==1) {
                                 if (agent->glucose_current_value >= agent->glucose_threshold) {
                                     actionvector.push_back(1);
                                     botPos=(botPos+1)&15;
                                 }
                            } else {
                                actionvector.push_back(1);
                                botPos=(botPos+1)&15;
                            }
                            break;
                        case 2:
                            //move bot on the left
                            if (glucose_project ==1) {
                                if (agent->glucose_current_value >= agent->glucose_threshold) {
                                    actionvector.push_back(-1);
                                    botPos=(botPos-1)&15;
                                }
                            } else {
                                actionvector.push_back(-1);
                                botPos=(botPos-1)&15;
                            }
                            break;
                    }
                    //update world value depending on blocks falling from right or left
                    if(j==-1){
                        // >> right shift operator, sensor[6]= R
                        world=((world>>1)&65535)+((world&1)<<15);
                    } else {// << left shift operator, sensor[7]= L
                        world=((world<<1)&65535)+((world>>15)&1);
                    }
                    listmotors.push_back(action);
               
                } //END loopticks
                //convert at new alphabet , detect pairs and assign
                // convert sensors 36x2 and motors 36x2 into (new alphabet) nasensor 36x1 namotor36x1
                if (vFitnessFunction ==2)
                {
                    for(ll=0;ll<loopTicks;ll++){
                        if (hiddensL1[ll][0]==0 &&  hiddensL1[ll][1]==0) listhiddensL1.push_back(0);
                        else if (hiddensL1[ll][0]==0 &&  hiddensL1[ll][1]==1) listhiddensL1.push_back(1);
                        else if (hiddensL1[ll][0]==1 &&  hiddensL1[ll][1]==0) listhiddensL1.push_back(2);
                        else if (hiddensL1[ll][0]==1 &&  hiddensL1[ll][1]==1) listhiddensL1.push_back(3);
                        if (hiddensL2[ll][0]==0 && hiddensL2[ll][1]==0) listhiddensL2.push_back(0);
                        else if (hiddensL2[ll][0]==0 &&  hiddensL2[ll][1]==1) listhiddensL2.push_back(1);
                        else if (hiddensL2[ll][0]==1 &&  hiddensL2[ll][1]==0) listhiddensL2.push_back(2);
                        else if (hiddensL2[ll][0]==1 &&  hiddensL2[ll][1]==1) listhiddensL2.push_back(3);
                        /*
                        if (hiddenLayer[ll][0]==0 &&  hiddenLayer[ll][1]==0 &&  hiddenLayer[ll][2]==0) listhiddenLayer.push_back(0);
                        else if (hiddenLayer[ll][0]==0 &&  hiddenLayer[ll][1]==0 &&  hiddenLayer[ll][2]==1) listhiddenLayer.push_back(1);
                        else if (hiddenLayer[ll][0]==0 &&  hiddenLayer[ll][1]==1 &&  hiddenLayer[ll][2]==0) listhiddenLayer.push_back(2);
                        else if (hiddenLayer[ll][0]==0 &&  hiddenLayer[ll][1]==1 &&  hiddenLayer[ll][2]==1) listhiddenLayer.push_back(3);
                        else if (hiddenLayer[ll][0]==1 &&  hiddenLayer[ll][1]==0 &&  hiddenLayer[ll][2]==0) listhiddenLayer.push_back(4);
                        else if (hiddenLayer[ll][0]==1 &&  hiddenLayer[ll][1]==0 &&  hiddenLayer[ll][2]==1) listhiddenLayer.push_back(5);
                        else if (hiddenLayer[ll][0]==1 &&  hiddenLayer[ll][1]==1 &&  hiddenLayer[ll][2]==0) listhiddenLayer.push_back(6);
                        else if (hiddenLayer[ll][0]==1 &&  hiddenLayer[ll][1]==1 &&  hiddenLayer[ll][2]==1) listhiddenLayer.push_back(7); */
                        
                    }
                }
                // List sensors
              for(ll=0;ll<loopTicks;ll++){
                  if (NumSensors==3) { //
                      if (sensors[ll][0]==0 &&  sensors[ll][1]==0 &&  sensors[ll][2]==0) listsensors.push_back(0);
                      else if (sensors[ll][0]==0 &&  sensors[ll][1]==0 &&  sensors[ll][2]==1) listsensors.push_back(1);
                      else if (sensors[ll][0]==0 &&  sensors[ll][1]==1 &&  sensors[ll][2]==0) listsensors.push_back(2);
                      else if (sensors[ll][0]==0 &&  sensors[ll][1]==1 &&  sensors[ll][2]==1) listsensors.push_back(3);
                      else if (sensors[ll][0]==1 &&  sensors[ll][1]==0 &&  sensors[ll][2]==0) listsensors.push_back(4);
                      else if (sensors[ll][0]==1 &&  sensors[ll][1]==0 &&  sensors[ll][2]==1) listsensors.push_back(5);
                      else if (sensors[ll][0]==1 &&  sensors[ll][1]==1 &&  sensors[ll][2]==0) listsensors.push_back(6);
                      else if (sensors[ll][0]==1 &&  sensors[ll][1]==1 &&  sensors[ll][2]==1) listsensors.push_back(7);
                  }
                  else { //2 sensors
                      if (sensors[ll][0]==0 &&  sensors[ll][1]==0) listsensors.push_back(0);
                      if (sensors[ll][0]==0 &&  sensors[ll][1]==1) listsensors.push_back(1);
                      if (sensors[ll][0]==1 &&  sensors[ll][1]==0) listsensors.push_back(2);
                      if (sensors[ll][0]==1 &&  sensors[ll][1]==1) listsensors.push_back(3);
                  }
              }
              
                //Calculate fitness for vFitnessFunction==1
                hit=false;
                counterhits++;
                for(m=0;m<3;m++)
                    if(((world>>((botPos+m)&15))&1)==1)
                        hit=true;
              if((i&1)==0){ // agent->fitness ois never used, fitness in main is always with fitnesses->correct
                  if(hit){
                      agent->correct++;
                      agent->fitness*=1.01;
                      agent->differentialCorrects[i]++;
                      //correct, increase glucose
                      agent->hitvector.push_back(1);
                      agent->glucose_current_value ++;
                  } else {
                      agent->fitness/=1.01; // agent->fitness= agent->fitness/1.01
                      agent->incorrect++;
                      //if no hit always decreases glucose
                      agent->hitvector.push_back(-1);
                      agent->glucose_current_value --;
                  }
              } else {
                  if(hit){
                      agent->incorrect++;
                      agent->fitness/=1.01;
                      //incorrect, reduce glucose
                      //if hit always increment
                      agent->hitvector.push_back(1);
                      agent->glucose_current_value ++;
                  } else {
                      agent->correct++;
                      agent->fitness*=1.01;//// agent->fitness= agent->fitness*1.01
                      agent->differentialCorrects[i]++;
                      //correct, increase glucose
                      //neutral
                      agent->hitvector.push_back(-1);
                      agent->glucose_current_value --;
                  }
                } // End check for hits, we dont need this measure for fitness now
              
              //cout<<"totalsqerror="<<totalsqerror<<endl;
              
            }// End initial positions 16
            //jj++;
        }// End block falling R or L j
            
    } // End block size or pattern i

        //agent->predcoderror=totalsqerror/128;
        // Calculate Conditional entropy between sensory information and hidden units layer1
        // calculate conditional entropy between motor information and hidden layer 2
        //min (H(p(xs_k+s|y_k-1)) and min(H(p(xm_k+s|y_k-1)) , where xs sensory signal, xm motor signal k time, s step
        // H(x|y=H(x) - I(x,y))
    
        if (vFitnessFunction==2) {
            mypredic= entropy(listsensors) - mutualInformation(listsensors, listhiddensL1);
            mypredic=numsensors -mypredic;
            myupdate=entropy(listmotors) - mutualInformation(listmotors, listhiddensL2);
            myupdate= nummotors-myupdate;
    
            //max mypredic listsensors and max myupdate mymotors
            agent->predcoderror=mypredic+ myupdate;
            // Max predcoderror = nS + nM -min(mypredic)==0 -min(myupdate)==0  =4
            // min predcoderror = nS + nM -max(mypredic)==ns -max(myupdate)==nm  =0
            
            //listhiddenLayer.erase(listhiddenLayer.begin(), listhiddenLayer.end());
            
            listhiddensL1.erase(listhiddensL1.begin(), listhiddensL1.end());
            listhiddensL2.erase(listhiddensL1.begin(), listhiddensL1.end());
            listhiddensL2.clear();
            listhiddensL1.clear();
        }
        agent->mutinfagent=mutualInformation(listsensors,listmotors);
        agent->sentropy=entropy(listsensors);
        // calculate distance and energy
        agent->distance=0; // the mean or final position
        agent->energy=agent->distance;
        for(i=0;i< actionvector.size();i++){
            agent->distance+=actionvector[i];
            if (actionvector[i]!=0) agent->energy++;
        }
        //if (-agent->energy) !=agent->distance )
        //    cout << agent->distance << " and " <<agent->energy<<endl;
        listsensors.clear();
        listmotors.clear();
        actionvector.clear();
 }



//invoke from main, inside saveLOD once for each agent. Reurns the table, cols 2,3,4 are world, sensors and hidden.
//col 5 output?
vector<vector<int> > tGame::executeGameLogStates(tAgent* agent,double sensorNoise){
    int world,botPos,blockPos;
    int i,j,k,l,m,T0,T1,u,ll;
    unsigned char W;
    int action;
  	vector<vector<int> > retValue; //output of this function is a matrix
    agent->fitness=1.0;
    agent->correct=agent->incorrect=0;
    //initialize mut inf agent
    agent->mutinfagent=1.0;
    //initialize opred cod error
    agent->predcoderror=1.0;
    
    rndW=agent->ID; // make random seeds unique from one another by including index
	rndX=~agent->ID;
	rndY=agent->ID^0b01010101010101010101010101010101;
	rndZ=agent->ID^0b10101010101010101010101010101010;
    int sensors [loopTicks][NumSensors];
    int motors [loopTicks][2];
    int hiddensL1 [loopTicks][2], hiddensL2 [loopTicks][2], hiddenLayer[loopTicks][3];
    
    double wh1,wh2,mutinfT;
    double residualerror,residualerrorH1,residualerrorH2,totalsqerror, mypredic,myupdate;
    //vector of 36x1 with sensor and motorstates for 36 ticks
    std::vector<int> listsensors,listmotors,listhiddensL1, listhiddensL2,listhiddenLayer,actionvector;
    listsensors.clear();
    listmotors.clear();
    listhiddensL1.clear();
    listhiddensL2.clear();
    listhiddenLayer.clear();
    actionvector.clear();
    //Initialize values for glucose level
    // Parameters to be estimated , no clear how
    //agent->glucose_init_level= 2304; // for the entire game 4608 ticks 38(height block) x 16 (init pos bot)x2 (dir)x 4 (block size)
    //agent->glucose_threshold=0;
    agent->glucose_current_value=agent->glucose_init_level;
    agent->hitvector.clear();
    agent->hitvector.shrink_to_fit();
    int counterhits=0; // counter 0..128 number of times the block has fallen
    wh1=1;
    wh2=1-wh1;
    mutinfT=0;
    totalsqerror=0;
    //Initialize sensors matrix
    for(i=0;i<loopTicks;i++){
        for(j=0;j<NumSensors;j++){
            sensors [i][j]=0;
        }
    }
    bool hit;
    retValue.clear();
    retValue.resize(6); //2 4 2 input, hidden and output unit
    agent->differentialCorrects.resize(patterns.size());
    for(i=0;i<agent->differentialCorrects.size();i++)
        agent->differentialCorrects[i]=0;
    for(i=0;i<patterns.size();i++){
        for(j=-1;j<2;j+=2){
            for(k=0;k<16;k++){
                //Larissa: Change environment after 30,000 Gen
//                if (agent->born > nowUpdate || i<2){
                world=patterns[i];
//                } else{
//                    world=patterns[i-2];
//                }  
                agent->resetBrain();
                botPos=k;
                blockPos=0; //
                //loop the world
                //motors[0][0]= sensors[0][1];
                //motors[0][1]= sensors[0][0];
                //motors[0][0]=1;
                //motors[0][1]=1;
                //Initialize
                listhiddensL1.clear();
                listhiddensL2.clear();
                //listsensors.clear();
                residualerror=0;
                residualerrorH1=0;
                residualerrorH2=residualerrorH1;
                for(l=0;l<loopTicks;l++){ //loopTicks=36
                    //AH: Sensors have no noise in them now
                    agent->states[0]=(world>>botPos)&1;
                    //                    agent->states[1]=0;
                    agent->states[1]=(world>>((botPos+2)&15))&1;
                    sensors[l][0]=agent->states[0];
                    sensors[l][1]=agent->states[1];
                    //3 sensors
                    if (NumSensors==3)  {agent->states[2]=(world>>((botPos+1)&15))&1;sensors[l][2]=agent->states[2];}

                    //Sensors
                    //sensors[l][2]=agent->states[2];
                    // The best agents , now sense NOISE
                    //NOISE PERCEPTION
                    //agent->states[0]=rand() %2;
                    //agent->states[1]=rand() %2;
                    
                    hiddensL1[l][0]=agent->states[3];
                    hiddensL1[l][1]=agent->states[4];
                    hiddensL2[l][0]=agent->states[5];
                    hiddensL2[l][1]=agent->states[6];
     
                    applyNoise(agent, sensorNoise);  
                    // set motors to 0 to preven reading from them
                    if (counterhits > 0) {
                        // if hitvector[counterhits-1] == -1 last game bot failed to get the block
                        if (agent->hitvector[counterhits-1] == -1) agent->states[glucose_node]=0;
                        // if hitvector[counterhits-1] == 1 last game bot got the block and increased its energy
                        if (agent->hitvector[counterhits-1] == 1)  agent->states[glucose_node]=1;
                    }
                    agent->states[6]=0; agent->states[7]=0;
                    T0=0;
					for(u=0;u<8;u++)
						T0|=(agent->states[u]&1)<<u;
					agent->updateStates();
                    T1=0;
					for(u=0;u<8;u++)
						T1|=(agent->states[u]&1)<<u;
                    //esta linea sobra lo hace despues
					int action=(agent->states[maxNodes-1])+(agent->states[maxNodes-2]<<1);
                    //action = 0|1 + 0|2 (0 none, 1 motorR 2 motorL 3 both)
                    retValue[0].push_back(T0);
                    retValue[1].push_back(T1);
                    int W=i<<1;
                    
                    if(j!=-1)
                        W|=1;//bitwise or
                    
                    
                    //int W;
                    // learn the size, for task 9 i=0 s,1 b,2 s 3 big
                    //if (i%2 == 0) {W|=1;}
                    //else {W|=0;}
                    
                    retValue[2].push_back(W);   // W
                    retValue[3].push_back((T0>>6)&3);  // Sensors
                    retValue[4].push_back((T1>>2)&15); // Hidden units
                    retValue[5].push_back(T1&3); // Outputs?
                    //Larissa: limit to one Motor
                    //
                    //agent->states[7]=0;
                    //
//                    if (agent->born < nowUpdate){
//                        agent->states[7]=0;
//                    }
                    //NOISE PERCEPTION
                    //agent->states[0]=rand() %2;
                    //agent->states[1]=rand() %2;

                    action=agent->states[6]+(agent->states[7]<<1);
                    switch(action){
                        case 0:
                        case 3:// nothing!
                            actionvector.push_back(0);
                            break;
                        case 1: // a derechas
                            //move bot on the right
                            if (glucose_project ==1) {
                                if (agent->glucose_current_value >= agent->glucose_threshold) {
                                    actionvector.push_back(1);
                                    botPos=(botPos+1)&15;
                                }
                            } else {
                                actionvector.push_back(1);
                                botPos=(botPos+1)&15;
                            }
                            break;
                        case 2: // a izquierdas
                            //move bot on the left
                            if (glucose_project ==1) {
                                if (agent->glucose_current_value >= agent->glucose_threshold) {
                                    actionvector.push_back(-1);
                                    botPos=(botPos-1)&15;
                                }
                            } else {
                                actionvector.push_back(-1);
                                botPos=(botPos-1)&15;
                            }
                            break;
                    }

                    if(j==-1){
                        // >> right shift operator, sensor[6]= R
                        world=((world>>1)&65535)+((world&1)<<15);
                    } else {// << left shift operator, sensor[7]= L
                        world=((world<<1)&65535)+((world>>15)&1);
                    }

                    listmotors.push_back(action);
                } //END loopticks
           
                if (vFitnessFunction ==2)
                {
                    for(ll=0;ll<loopTicks;ll++){
                        if (hiddensL1[ll][0]==0 &&  hiddensL1[ll][1]==0) listhiddensL1.push_back(0);
                        else if (hiddensL1[ll][0]==0 &&  hiddensL1[ll][1]==1) listhiddensL1.push_back(1);
                        else if (hiddensL1[ll][0]==1 &&  hiddensL1[ll][1]==0) listhiddensL1.push_back(2);
                        else if (hiddensL1[ll][0]==1 &&  hiddensL1[ll][1]==1) listhiddensL1.push_back(3);
                        if (hiddensL2[ll][0]==0 && hiddensL2[ll][1]==0) listhiddensL2.push_back(0);
                        else if (hiddensL2[ll][0]==0 &&  hiddensL2[ll][1]==1) listhiddensL2.push_back(1);
                        else if (hiddensL2[ll][0]==1 &&  hiddensL2[ll][1]==0) listhiddensL2.push_back(2);
                        else if (hiddensL2[ll][0]==1 &&  hiddensL2[ll][1]==1) listhiddensL2.push_back(3);
                        
                        if (hiddensL2[ll][0]==0 && hiddensL2[ll][1]==0) listhiddensL2.push_back(0);
                        else if (hiddensL2[ll][0]==0 &&  hiddensL2[ll][1]==1) listhiddensL2.push_back(1);
                        else if (hiddensL2[ll][0]==1 &&  hiddensL2[ll][1]==0) listhiddensL2.push_back(2);
                        else if (hiddensL2[ll][0]==1 &&  hiddensL2[ll][1]==1) listhiddensL2.push_back(3);
                        /*
                         //if (motors[ll][0]==0 &&  motors[ll][1]==0) listmotors.push_back(0);
                         //else if (motors[ll][0]==0 &&  motors[ll][1]==1) listmotors.push_back(1);
                         //else if (motors[ll][0]==1 &&  motors[ll][1]==0) listmotors.push_back(2);
                         //else if (motors[ll][0]==1 &&  motors[ll][1]==1) listmotors.push_back(3);
                         if (hiddenLayer[ll][0]==0 &&  hiddenLayer[ll][1]==0 &&  hiddenLayer[ll][2]==0) listhiddenLayer.push_back(0);
                         else if (hiddenLayer[ll][0]==0 &&  hiddenLayer[ll][1]==0 &&  hiddenLayer[ll][2]==1) listhiddenLayer.push_back(1);
                         else if (hiddenLayer[ll][0]==0 &&  hiddenLayer[ll][1]==1 &&  hiddenLayer[ll][2]==0) listhiddenLayer.push_back(2);
                         else if (hiddenLayer[ll][0]==0 &&  hiddenLayer[ll][1]==1 &&  hiddenLayer[ll][2]==1) listhiddenLayer.push_back(3);
                         
                         else if (hiddenLayer[ll][0]==1 &&  hiddenLayer[ll][1]==0 &&  hiddenLayer[ll][2]==0) listhiddenLayer.push_back(4);
                         else if (hiddenLayer[ll][0]==1 &&  hiddenLayer[ll][1]==0 &&  hiddenLayer[ll][2]==1) listhiddenLayer.push_back(5);
                         else if (hiddenLayer[ll][0]==1 &&  hiddenLayer[ll][1]==1 &&  hiddenLayer[ll][2]==0) listhiddenLayer.push_back(6);
                         else if (hiddenLayer[ll][0]==1 &&  hiddenLayer[ll][1]==1 &&  hiddenLayer[ll][2]==1) listhiddenLayer.push_back(7);
                         */
                    } // end loopsticks to build listhiddenlayers
                }
                for(ll=0;ll<loopTicks;ll++){
                    //if (motors[ll][0]==0 &&  motors[ll][1]==0) listmotors.push_back(0);
                    //else if (motors[ll][0]==0 &&  motors[ll][1]==1) listmotors.push_back(1);
                    //else if (motors[ll][0]==1 &&  motors[ll][1]==0) listmotors.push_back(2);
                    //else if (motors[ll][0]==1 &&  motors[ll][1]==1) listmotors.push_back(3);
                    /* 3 sensors
                    if (sensors[ll][0]==0 &&  sensors[ll][1]==0 &&  sensors[ll][2]==0) listsensors.push_back(0);
                    else if (sensors[ll][0]==0 &&  sensors[ll][1]==1 &&  sensors[ll][2]==0) listsensors.push_back(1);
                    else if (sensors[ll][0]==1 &&  sensors[ll][1]==0 &&  sensors[ll][2]==0) listsensors.push_back(2);
                    else if (sensors[ll][0]==1 &&  sensors[ll][1]==1 &&  sensors[ll][2]==0) listsensors.push_back(3);
                    
                    else if (sensors[ll][0]==0 &&  sensors[ll][1]==0 &&  sensors[ll][2]==1) listsensors.push_back(4);
                    else if (sensors[ll][0]==0 &&  sensors[ll][1]==1 &&  sensors[ll][2]==1) listsensors.push_back(5);
                    else if (sensors[ll][0]==1 &&  sensors[ll][1]==0 &&  sensors[ll][2]==1) listsensors.push_back(6);
                    else if (sensors[ll][0]==1 &&  sensors[ll][1]==1 &&  sensors[ll][2]==1) listsensors.push_back(7); */
                    
                    // 2 sensors
                    if (sensors[ll][0]==0 &&  sensors[ll][1]==0) listsensors.push_back(0);
                    else if (sensors[ll][0]==0 &&  sensors[ll][1]==1 ) listsensors.push_back(1);
                    else if (sensors[ll][0]==1 &&  sensors[ll][1]==0 ) listsensors.push_back(2);
                    else if (sensors[ll][0]==1 &&  sensors[ll][1]==1 ) listsensors.push_back(3);
                    
                }
          
                //check for hit
                hit=false;
                counterhits++; // increment hte number of games of blocks fallen
                for(m=0;m<3;m++)
                    if(((world>>((botPos+m)&15))&1)==1)
                        hit=true;
                if((i&1)==0){ // agent->fitness ois never used, fitness in main is always with fitnesses->correct
                    if(hit){
                        agent->correct++;
                        agent->fitness*=1.01;
                        agent->differentialCorrects[i]++;
                        //correct, increase glucose
                        agent->hitvector.push_back(1);
                        agent->glucose_current_value ++;
                    } else {
                        agent->fitness/=1.01; // agent->fitness= agent->fitness/1.01
                        agent->incorrect++;
                        //if no hit always decreases glucose
                        agent->hitvector.push_back(-1);
                        agent->glucose_current_value --;
                    }
                } else {
                    if(hit){
                        agent->incorrect++;
                        agent->fitness/=1.01;
                        //incorrect, reduce glucose
                        //if hit always increment
                        agent->hitvector.push_back(1);
                        agent->glucose_current_value ++;
                    } else {
                        agent->correct++;
                        agent->fitness*=1.01;//// agent->fitness= agent->fitness*1.01
                        agent->differentialCorrects[i]++;
                        //correct, increase glucose
                        //neutral
                        agent->hitvector.push_back(-1);
                        agent->glucose_current_value --;
                    }
                } // end check fit

                if (vFitnessFunction==2) {
                    residualerror=0;
                    for(ll=loopTicks-1;ll>1;ll--){
                        //residualerror+=pow(listhiddensL1[ll] - listsensors[ll-1],2);
                        //residualerrorH1+=pow(listhiddensL1[ll-1] - listsensors[ll-2],2);
                        
                        //residualerrorH2+=pow(listhiddensL2[ll] - listhiddensL1[ll-1],2);
                        //residualerrorH1+=pow(listhiddenLayer[ll-1] - listsensors[ll-2],2);
                        //residualerror= (wh1*residualerrorH1+ wh2*residualerrorH2)+residualerror;
                        if (errorabs) residualerror+= abs(listhiddenLayer[ll-1] - listsensors[ll-2]);
                        else residualerror+= pow(listhiddenLayer[ll-1] - listsensors[ll-2],2);
                    }
                    //residualerror=(wh2*pow(listhiddensL2[2] - listsensors[1],2)+ wh1*pow(listhiddensL1[1] - listsensors[0],2))+ residualerror;
                    //residualerror=(wh2*pow(listhiddensL2[2] - listsensors[1],2)+ wh1*pow(listhiddenLayer[1] - listsensors[0],2))+ residualerror;
                    if (errorabs) residualerror+= abs(listhiddenLayer[1] - listsensors[0]);
                    else residualerror+=pow(listhiddenLayer[1] - listsensors[0],2);
                    
                    residualerror= residualerror/loopTicks;
                    totalsqerror+=residualerror;
                } // End vFitnessFunction==2

            }// End initial positions 16
        }// End block falling R or L
        
    }// End block size or pattern
    
    
    if (vFitnessFunction==2) {
        mypredic= entropy(listsensors) - mutualInformation(listsensors, listhiddensL1);
        //mypredic=numsensors -mypredic;
        myupdate=entropy(listmotors) - mutualInformation(listmotors, listhiddensL2);
        // myupdate= nummotors-myupdate;
        
        //max mypredic listsensors and max myupdate mymotors
        agent->predcoderror=mypredic+ myupdate;
    
        //listhiddenLayer.erase(listhiddenLayer.begin(), listhiddenLayer.end());
       
        listhiddensL1.erase(listhiddensL1.begin(), listhiddensL1.end());
        listhiddensL2.erase(listhiddensL2.begin(), listhiddensL2.end());
        listhiddensL1.clear();
        listhiddensL2.clear();
    }
    agent->sentropy=entropy(listsensors);
    agent->mutinfagent=mutualInformation(listsensors,listmotors);
    agent->agfitness=pow(1.02, agent->correct);
    if (NumSensors==3) {
        agent->fitnessmif=pow(1.02, agent->mutinfagent*24 + 56);
        agent->agcombinedfitness= wpc*agent->predcoderror + wco*agent->agfitness + wmi*agent->fitnessmif;
    }
    else { //2 sensors
        agent->fitnessmif=pow(1.02, agent->mutinfagent*36 + 56);
        agent->agcombinedfitness= wpc*agent->predcoderror + wco*agent->agfitness + wmi*agent->fitnessmif;
    }
    // calculate distance and energy
    agent->distance=0; // the mean or final position
    agent->energy=agent->distance;
    for(i=0;i< actionvector.size();i++){
        agent->distance+=actionvector[i];
        if (actionvector[i]!=0) agent->energy++;
    }
    listsensors.erase(listsensors.begin(), listsensors.end());
    listsensors.clear();
    listmotors.clear();
    actionvector.clear();
    return retValue;
    
    
}

//Called in tGame::makeFullAnalysis
void tGame::analyseKO(tAgent* agent,int which, int setTo,double sensorNoise){
    int world,botPos,blockPos;
    int i,j,k,l,m,ll;
    int action;
    agent->fitness=1.0;
    //initialize mut inf agent
    agent->mutinfagent=1.0;
    //
    agent->predcoderror=1.0;
    agent->correct=agent->incorrect=0;
    rndW=agent->ID; // make random seeds unique from one another by including index
	rndX=~agent->ID;
	rndY=agent->ID^0b01010101010101010101010101010101;
	rndZ=agent->ID^0b10101010101010101010101010101010;
    int sensors [loopTicks][NumSensors];
    int motors [loopTicks][2];
    int hiddensL1 [loopTicks][2], hiddensL2 [loopTicks][2], hiddenLayer[loopTicks][3];
    double wh1,wh2,mutinfT;
  
    //Initialize values for glucose level
    // Parameters to be estimated , no clear how
    //agent->glucose_init_level= 2304; // for the entire game 4608 ticks 38(height block) x 16 (init pos bot)x2 (dir)x 4 (block size)
    //agent->glucose_threshold=0;
    agent->glucose_current_value=agent->glucose_init_level;
    agent->hitvector.clear();
    agent->hitvector.shrink_to_fit();
    double residualerror,residualerrorH1,residualerrorH2,totalsqerror, mypredic,myupdate;
    //vector of 36x1 with sensor and motorstates for 36 ticks
    std::vector<int> listsensors,listmotors,listhiddensL1, listhiddensL2,listhiddenLayer,actionvector;
    actionvector.clear();
    int counterhits=0; // counter 0..128 number of times the block has fallen
    //Initialize sensors matrix
    for(i=0;i<loopTicks;i++){
        for(j=0;j<NumSensors;j++){
            sensors [i][j]=0;
        }
    }
    
    //std::vector<int> listmotorsA (listmotors, listmotors + sizeof(listmotors) / sizeof(int) );
    listsensors.clear();
    listmotors.clear();
    listhiddensL1.clear();
    listhiddensL2.clear();
    listhiddenLayer.clear();
    actionvector.clear();
    wh1=1;
    wh2=1-wh1;
    mutinfT=0;
    totalsqerror=0;
    // max entropy of vector sm vector
    bool hit;
    agent->differentialCorrects.resize(patterns.size());
    for(i=0;i<agent->differentialCorrects.size();i++)
        agent->differentialCorrects[i]=0;
    for(i=0;i<patterns.size();i++){
        for(j=-1;j<2;j+=2){
            // Initialize motors[0]=0 because sensors_t = motors_t+1

            for(k=0;k<16;k++){
                //Larissa: Change environment after 30,000 Gen
//                if (agent->born > nowUpdate || i<2){
                    world=patterns[i];                  
//                } else{
//                    world=patterns[i-2];
//                }  
                agent->resetBrain();
                botPos=k;
                blockPos=0;
                //loop the world
                //motors[0][0]= 1;
                //motors[0][1]= 1;
                //Initialize
                listhiddensL1.clear();
                listhiddensL2.clear();
                //listsensors.clear();
                residualerror=0;
                residualerrorH1=0;
                residualerrorH2=residualerrorH1;
                for(l=0;l<loopTicks;l++){
                    agent->states[0]=(world>>botPos)&1;
                    //                    agent->states[1]=0;
                    agent->states[1]=(world>>((botPos+2)&15))&1;
                    sensors[l][0]=agent->states[0];
                    sensors[l][1]=agent->states[1];
                    //3 sensors
                    if (NumSensors==3) {agent->states[2]=(world>>((botPos+1)&15))&1;sensors[l][2]=agent->states[2];}
                    
                    //Sensors
                    //sensors[l][2]=agent->states[2];
                    // The best agents , now sense NOISE
                    //NOISE PERCEPTION
                    //agent->states[0]=rand() %2;
                    //agent->states[1]=rand() %2;

                    applyNoise(agent, sensorNoise);
                    // set motors to 0 to preven reading from them
                    //I  modified this, which 0..7 so it is not set to 0 as it said...
                    if (counterhits > 0) {
                        // if hitvector[counterhits-1] == -1 last game bot failed to get the block
                        if (agent->hitvector[counterhits-1] == -1) agent->states[glucose_node]=0;
                        // if hitvector[counterhits-1] == 1 last game bot got the block and increased its energy
                        if (agent->hitvector[counterhits-1] == 1)  agent->states[glucose_node]=1;
                    }
                    agent->states[which]=setTo;
                    agent->states[6]=0; agent->states[7]=0;
                    //agent->states[which]=setTo;
                    agent->updateStates();
                    //Larissa: limit to one Motor
                    //
                    //agent->states[7]=0;
                    //
//                    if (agent->born < nowUpdate){
//                        agent->states[7]=0;
//                    }
                    //NOISE PERCEPTION
                    //agent->states[0]=rand() %2;
                    //agent->states[1]=rand() %2;

                    action=agent->states[6]+(agent->states[7]<<1);
                    switch(action){
                        case 0:
                        case 3:// nothing!
                            actionvector.push_back(0);
                            break;
                        case 1:
                            //move bot on the right
                            if (glucose_project ==1) {
                                if (agent->glucose_current_value >= agent->glucose_threshold) {
                                    actionvector.push_back(1);
                                    botPos=(botPos+1)&15;
                                }
                            } else {
                                actionvector.push_back(1);
                                botPos=(botPos+1)&15;
                            }
                            break;
                        case 2:
                            //move bot on the left
                            if (glucose_project ==1) {
                                if (agent->glucose_current_value >= agent->glucose_threshold) {
                                    actionvector.push_back(-1);
                                    botPos=(botPos-1)&15;
                                }
                            } else {
                                actionvector.push_back(-1);
                                botPos=(botPos-1)&15;
                            }
                            break;
                    }
                    if(j==-1){
                        // >> right shift operator, sensor[6]= R
                        world=((world>>1)&65535)+((world&1)<<15);
                    } else {// << left shift operator, sensor[7]= L
                        world=((world<<1)&65535)+((world>>15)&1);
                    }
                    listmotors.push_back(action);
                }  // End LooTicks
                if (vFitnessFunction ==2){
                    for(ll=0;ll<loopTicks;ll++){
                        if (hiddensL1[ll][0]==0 &&  hiddensL1[ll][1]==0) listhiddensL1.push_back(0);
                        else if (hiddensL1[ll][0]==0 &&  hiddensL1[ll][1]==1) listhiddensL1.push_back(1);
                        else if (hiddensL1[ll][0]==1 &&  hiddensL1[ll][1]==0) listhiddensL1.push_back(2);
                        else if (hiddensL1[ll][0]==1 &&  hiddensL1[ll][1]==1) listhiddensL1.push_back(3);
                        if (hiddensL2[ll][0]==0 && hiddensL2[ll][1]==0) listhiddensL2.push_back(0);
                        else if (hiddensL2[ll][0]==0 &&  hiddensL2[ll][1]==1) listhiddensL2.push_back(1);
                        else if (hiddensL2[ll][0]==1 &&  hiddensL2[ll][1]==0) listhiddensL2.push_back(2);
                        else if (hiddensL2[ll][0]==1 &&  hiddensL2[ll][1]==1) listhiddensL2.push_back(3);
                        
                        if (hiddensL2[ll][0]==0 && hiddensL2[ll][1]==0) listhiddensL2.push_back(0);
                        else if (hiddensL2[ll][0]==0 &&  hiddensL2[ll][1]==1) listhiddensL2.push_back(1);
                        else if (hiddensL2[ll][0]==1 &&  hiddensL2[ll][1]==0) listhiddensL2.push_back(2);
                        else if (hiddensL2[ll][0]==1 &&  hiddensL2[ll][1]==1) listhiddensL2.push_back(3);
                        /*
                         //if (motors[ll][0]==0 &&  motors[ll][1]==0) listmotors.push_back(0);
                         //else if (motors[ll][0]==0 &&  motors[ll][1]==1) listmotors.push_back(1);
                         //else if (motors[ll][0]==1 &&  motors[ll][1]==0) listmotors.push_back(2);
                         //else if (motors[ll][0]==1 &&  motors[ll][1]==1) listmotors.push_back(3);
                         if (hiddenLayer[ll][0]==0 &&  hiddenLayer[ll][1]==0 &&  hiddenLayer[ll][2]==0) listhiddenLayer.push_back(0);
                         else if (hiddenLayer[ll][0]==0 &&  hiddenLayer[ll][1]==0 &&  hiddenLayer[ll][2]==1) listhiddenLayer.push_back(1);
                         else if (hiddenLayer[ll][0]==0 &&  hiddenLayer[ll][1]==1 &&  hiddenLayer[ll][2]==0) listhiddenLayer.push_back(2);
                         else if (hiddenLayer[ll][0]==0 &&  hiddenLayer[ll][1]==1 &&  hiddenLayer[ll][2]==1) listhiddenLayer.push_back(3);
                         
                         else if (hiddenLayer[ll][0]==1 &&  hiddenLayer[ll][1]==0 &&  hiddenLayer[ll][2]==0) listhiddenLayer.push_back(4);
                         else if (hiddenLayer[ll][0]==1 &&  hiddenLayer[ll][1]==0 &&  hiddenLayer[ll][2]==1) listhiddenLayer.push_back(5);
                         else if (hiddenLayer[ll][0]==1 &&  hiddenLayer[ll][1]==1 &&  hiddenLayer[ll][2]==0) listhiddenLayer.push_back(6);
                         else if (hiddenLayer[ll][0]==1 &&  hiddenLayer[ll][1]==1 &&  hiddenLayer[ll][2]==1) listhiddenLayer.push_back(7);
                         */
                    } // end loopsticks to build listhiddenlayers
                }
                for(ll=0;ll<loopTicks;ll++){
                    if (NumSensors==3) { //better do as a function
                        if (sensors[ll][0]==0 &&  sensors[ll][1]==0 &&  sensors[ll][2]==0) listsensors.push_back(0);
                        else if (sensors[ll][0]==0 &&  sensors[ll][1]==0 &&  sensors[ll][2]==1) listsensors.push_back(1);
                        else if (sensors[ll][0]==0 &&  sensors[ll][1]==1 &&  sensors[ll][2]==0) listsensors.push_back(2);
                        else if (sensors[ll][0]==0 &&  sensors[ll][1]==1 &&  sensors[ll][2]==1) listsensors.push_back(3);
                        else if (sensors[ll][0]==1 &&  sensors[ll][1]==0 &&  sensors[ll][2]==0) listsensors.push_back(4);
                        else if (sensors[ll][0]==1 &&  sensors[ll][1]==0 &&  sensors[ll][2]==1) listsensors.push_back(5);
                        else if (sensors[ll][0]==1 &&  sensors[ll][1]==1 &&  sensors[ll][2]==0) listsensors.push_back(6);
                        else if (sensors[ll][0]==1 &&  sensors[ll][1]==1 &&  sensors[ll][2]==1) listsensors.push_back(7);
                    }
                    else {
                        if (sensors[ll][0]==0 &&  sensors[ll][1]==0) listsensors.push_back(0);
                        else if (sensors[ll][0]==0 &&  sensors[ll][1]==1) listsensors.push_back(1);
                        else if (sensors[ll][0]==1 &&  sensors[ll][1]==0) listsensors.push_back(2);
                        else if (sensors[ll][0]==1 &&  sensors[ll][1]==1) listsensors.push_back(3);
                    }
                }
                hit=false;
                counterhits++; // increment hte number of games of blocks fallen
                for(m=0;m<3;m++)
                    if(((world>>((botPos+m)&15))&1)==1)
                        hit=true;
                if((i&1)==0){ // agent->fitness ois never used, fitness in main is always with fitnesses->correct
                    if(hit){
                        agent->correct++;
                        agent->fitness*=1.01;
                        agent->differentialCorrects[i]++;
                        //correct, increase glucose
                        agent->hitvector.push_back(1);
                        agent->glucose_current_value ++;
                    } else {
                        agent->fitness/=1.01; // agent->fitness= agent->fitness/1.01
                        agent->incorrect++;
                        //if no hit always decreases glucose
                        agent->hitvector.push_back(-1);
                        agent->glucose_current_value --;
                    }
                } else {
                    if(hit){
                        agent->incorrect++;
                        agent->fitness/=1.01;
                        //incorrect, reduce glucose
                        //if hit always increment
                        agent->hitvector.push_back(1);
                        agent->glucose_current_value ++;
                    } else {
                        agent->correct++;
                        agent->fitness*=1.01;//// agent->fitness= agent->fitness*1.01
                        agent->differentialCorrects[i]++;
                        //correct, increase glucose
                        //neutral
                        agent->hitvector.push_back(-1);
                        agent->glucose_current_value --;
                    }
                }//check for hit
                if (vFitnessFunction==2) {
                    residualerror=0;
                    for(ll=loopTicks-1;ll>1;ll--){
                        //residualerror+=pow(listhiddensL1[ll] - listsensors[ll-1],2);
                        //residualerrorH1+=pow(listhiddensL1[ll-1] - listsensors[ll-2],2);
                        
                        //residualerrorH2+=pow(listhiddensL2[ll] - listhiddensL1[ll-1],2);
                        //residualerrorH1+=pow(listhiddenLayer[ll-1] - listsensors[ll-2],2);
                        //residualerror= (wh1*residualerrorH1+ wh2*residualerrorH2)+residualerror;
                        if (errorabs) residualerror+= abs(listhiddenLayer[ll-1] - listsensors[ll-2]);
                        else residualerror+= pow(listhiddenLayer[ll-1] - listsensors[ll-2],2);
                    }
                    //residualerror=(wh2*pow(listhiddensL2[2] - listsensors[1],2)+ wh1*pow(listhiddensL1[1] - listsensors[0],2))+ residualerror;
                    //residualerror=(wh2*pow(listhiddensL2[2] - listsensors[1],2)+ wh1*pow(listhiddenLayer[1] - listsensors[0],2))+ residualerror;
                    if (errorabs) residualerror+= abs(listhiddenLayer[1] - listsensors[0]);
                    else residualerror+=pow(listhiddenLayer[1] - listsensors[0],2);
                    
                    residualerror= residualerror/loopTicks;
                    totalsqerror+=residualerror;
                } // End vFitnessFunction==2
                
                
            }// End initial positions 16
        }// End block falling R or L j
    }// End block size or pattern i

    if (vFitnessFunction==2) {
        mypredic= entropy(listsensors) - mutualInformation(listsensors, listhiddensL1);
        //mypredic=numsensors -mypredic;
        myupdate=entropy(listmotors) - mutualInformation(listmotors, listhiddensL2);
        // myupdate= nummotors-myupdate;
        
        //max mypredic listsensors and max myupdate mymotors
        agent->predcoderror=mypredic+ myupdate;
        
        //listhiddenLayer.erase(listhiddenLayer.begin(), listhiddenLayer.end());
        
        listhiddensL1.erase(listhiddensL1.begin(), listhiddensL1.end());
        listhiddensL2.erase(listhiddensL2.begin(), listhiddensL2.end());
        listhiddensL1.clear();
        listhiddensL2.clear();
    }
    agent->sentropy=entropy(listsensors);
    agent->mutinfagent=mutualInformation(listsensors,listmotors);
    agent->agfitness=pow(1.02, agent->correct);
    if (NumSensors==3) {
        agent->fitnessmif=pow(1.02, agent->mutinfagent*24 + 56);
        agent->agcombinedfitness= wpc*agent->predcoderror + wco*agent->agfitness + wmi*agent->fitnessmif;
    }
    else { //2 sensors
        agent->fitnessmif=pow(1.02, agent->mutinfagent*36 + 56);
        agent->agcombinedfitness= wpc*agent->predcoderror + wco*agent->agfitness + wmi*agent->fitnessmif;
    }
    agent->distance=0; // the mean or final position
    agent->energy=agent->distance;
    for(i=0;i< actionvector.size();i++){
        agent->distance+=actionvector[i];
        if (actionvector[i]!=0) agent->energy++;
    }
    listsensors.erase(listsensors.begin(), listsensors.end());
    listsensors.clear();
    listmotors.clear();
    actionvector.clear();
}


double tGame::mutualInformation(const vector<int>& A,const vector<int>& B){
	set<int> nrA,nrB;
	set<int>::iterator aI,bI;
	map<int,map<int,double> > pXY;
	map<int,double> pX,pY;
	int i,j;
	double c=1.0/(double)A.size();
	double I=0.0;
    //Initialize marginal probabilities
	for(i=0;i<A.size();i++){
        
        //if (A[i]) cout <<"impos";
        
		nrA.insert(A[i]);
		nrB.insert(B[i]);
		pX[A[i]]=0.0;
		pY[B[i]]=0.0;
	}
    //Initialize joint probabilities
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
                
             //   if  ((pXY[*aI][*bI]!=0.0))
                    I+=pXY[*aI][*bI]*log2(pXY[*aI][*bI]/(pX[*aI]*pY[*bI]));
	return I;
	
}

double tGame::entropy(const vector<int>& list){
	map<int, double> p;
	map<int,double>::iterator pI;
	int i;
	double H=0.0;
	double c=1.0/(double)list.size();
    //Initialize probability distribution
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
    vector<int> world,sensors,brain,motors;
    cout<<"OLD fitness of agent: "<<agent->correct<<endl;
    cout<<"MI fitness of agent: "<<agent->mutinfagent<<endl;
    cout<<"PRED COD fitness of agent: "<<agent->predcoderror<<endl;
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
                //motors[j]=table[5][j];
            }
            R=computeRGiven(world, sensors, brain, 4,2,4);
            //Rmi=computeRGivenMI(world, sensors, brain, motors, 4, 2, 4, 2);
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
        //cout<<R<<endl;
        // Add in agent->mutinfagent in trial0_#_representation penultimo term before R
        //fprintf(F,"%f  ",agent->mutinfagent);
        fprintf(F,"%f  ",agent->predcoderror);
        //fprintf(F,"%f  ",Rmi);
        //fprintf(F,"%f\n",R);
    }
    fclose(F);
}

//Creates 5 files of analysis
//representation.txt by invoking represenationPerNodeSummary
//FullLogicTable.txt invokes  saveLogicTable
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
            // wc -l < trial0_89600_FullLogicTable.txt 257
            sprintf(filename,"%s_%i_FullLogicTable.txt",fileLead,agent->born);
            f=fopen(filename,"w+t");
            agent->saveLogicTable(f);
            fclose(f);
            //state to state table for only the lifetime
            //Format of line in LifetimeLogicTable.txt file is 0,0,1,0,0,1,0,0,,0,0,1,0,0,1,0,0, (8,,8)
            sprintf(filename,"%s_%i_LifetimeLogicTable.txt",fileLead,agent->born);
            f=fopen(filename,"w+t");
            table=this->executeGameLogStates(agent,sensorNoise); //a pointer to the current instance of the class, but we never dereference table to access it table* !?
            for(i=0;i<8;i++)
                fprintf(f,"T0_%i,",i);
            fprintf(f,",");
            for(i=0;i<8;i++)
                fprintf(f,"T1_%i,",i);
            fprintf(f,"\n");
            //pinta header of LifetimeLogicTable T0_0,T0_1,T0_2,T0_3,T0_4,T0_5,T0_6,T0_7,,T1_0,T1_1,T1_2,T1_3,T1_4,T1_5,T1_6,T1_7,
            //wc -l < trial0_89600_LifetimeLogicTable.txt = 4609 -1 = 36x32x4 (36 ticks for each of the 4 tests which run 32 times)
            
            for(j=0;j<table[0].size();j++){
                //printf("%i  %i\n",table[0][j],table[1][j]);
                for(i=0;i<8;i++)
                    //first 8 comes from table[0]
                    fprintf(f,"%i,",(table[0][j]>>i)&1);
                fprintf(f,",");
                for(i=0;i<8;i++)
                    //first 8 comes from table[1]
                    fprintf(f,"%i,",(table[1][j]>>i)&1);
                fprintf(f,"\n");
            }
            fclose(f);
            //ko table
            sprintf(filename,"%s_%i_KOdata.txt",fileLead,agent->born);
            f=fopen(filename,"w+t");
            executeGameLogStates(agent,sensorNoise);
            fprintf(f,"%i",agent->correct);
            fprintf(f,"	%f",agent->agfitness);
            fprintf(f,"	%f",agent->sentropy);
            fprintf(f,"	%f",agent->mutinfagent);
            fprintf(f,"	%f",agent->fitnessmif);
            fprintf(f,"	%f",agent->distance);
            fprintf(f,"	%f",agent->energy);
            fprintf(f,"	%i",agent->glucose_current_value);
            //agent->agcombinedfitness= wpc*agent->predcoderror + wco*agent->agfitness + wmi*agent->fitnessmif;
            fprintf(f,"	%f",agent->agcombinedfitness);
            for(i=0;i<8;i++)
                for(j=0;j<2;j++){
                    analyseKO(agent,i,j,sensorNoise);
                    fprintf(f,"	%i",agent->correct);
                    fprintf(f,"	%f",agent->mutinfagent);
                    fprintf(f,"	%f",agent->predcoderror);
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



double tGame::computeRGiven(vector<int> W,vector<int> S,vector<int> B,int nrWstates,int nrSstates,int nrBstates){
    //nrWstates is the number of environmental conditions you want to compare (block to the left,right, size etc.) (4)
    //nrSstates: number of sensors (2)
    //nrBstates: number of hidden units (4)
    
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


vector<double> tGame::computeMultipleRGiven(vector<int> W,vector<int> S,vector<int> B,int nrWstates,int nrSstates,int nrBstates){
    //nrWstates is the number of environmental conditions you want to compare (block to the left,right, size etc.) (4)
    //nrSstates: number of sensors (2)
    //nrBstates: number of hidden units (4)
    vector <double>  entropies;
    entropies.push_back(entropy(W)); // entropy of the Environment
    entropies.push_back(entropy(S)); // entropy of the sensors
    entropies.push_back(entropy(B)); //entropy of the Brain hidden nodes)
  	return entropies;
}




