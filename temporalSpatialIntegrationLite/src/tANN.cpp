/*
 *  tANN.cpp
 *  HMM_representations
 *
 *  Created by Arend on 12/20/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "tANN.h"
#include <math.h>

void tLayer::setup(int n_in,int n_out){
	int i,j;
	weights.resize(n_in);
	for(i=0;i<n_in;i++){
		weights[i].resize(n_out);
		for(j=0;j<n_out;j++)
			weights[i][j]=(((double)rand()/(double)RAND_MAX)*2.0)-1.0;
	}
	inStates.resize(n_in);
	outStates.resize(n_out);
	for(i=0;i<n_in;i++) inStates[i]=0.0;
	for(i=0;i<n_out;i++) outStates[i]=0.0;
}

void tLayer::update(bool useTanH){
	int i,j;
	for(i=0;i<outStates.size();i++)
		outStates[i]=0.0;
	for(i=0;i<weights.size();i++)
		for(j=0;j<weights[i].size();j++){
			outStates[j]+=inStates[i]*this->weights[i][j];
		}
	if(useTanH)
		for(i=0;i<outStates.size();i++)
			outStates[i]=tanh(outStates[i]);
}

void tANN::setup(void){
	int i;
	layers.resize(3);
	layers[0].setup(8,6);
	layers[1].setup(6,4);
	layers[2].setup(6,2);
	cout<<"setup executed"<<endl;
}


void tANN::inherit(tANN *ancestor,double mutationRate){
	int i,j,k;
	layers.resize(ancestor->layers.size());
	for(k=0;k<layers.size();k++){
		layers[k].inStates.resize(ancestor->layers[k].inStates.size());
		layers[k].outStates.resize(ancestor->layers[k].outStates.size());
		layers[k].weights.resize(ancestor->layers[k].weights.size());
		for(i=0;i<layers[k].weights.size();i++){
			layers[k].weights[i].resize(ancestor->layers[k].weights[i].size());
			for(j=0;j<layers[k].weights[i].size();j++)
				if((double)rand()/(double)RAND_MAX<mutationRate)
					layers[k].weights[i][j]+=(((double)rand()/(double)RAND_MAX)*0.4)-0.2;
				else
					layers[k].weights[i][j]=ancestor->layers[k].weights[i][j];
		}
	}
}

void tANN::saveLOD(FILE *genomeFile){
	int k,i,j;
	for(k=0;k<layers.size();k++){
		for(i=0;i<layers[k].weights.size();i++){
			for(j=0;j<layers[k].weights[i].size();j++)
				fprintf(genomeFile,"	%f",layers[k].weights[i][j]);
		}
	}
	fprintf(genomeFile,"\n");
}
void tANN::load(char *filename){
	int k,i,j;
	float l;
	FILE *f;
	f=fopen(filename,"r+t");
	fscanf(f,"%i",&i);
	this->setup();
	for(k=0;k<layers.size();k++){
		for(i=0;i<layers[k].weights.size();i++){
			for(j=0;j<layers[k].weights[i].size();j++){
				fscanf(f,"	%f",&l);
				layers[k].weights[i][j]=l;
			}
		}
	}
	fclose(f);
}

void tANN::update(unsigned char *states){
	int i,j;
	//*
	for(i=0;i<4;i++)
		if(states[i]==0)
			layers[0].inStates[i]=-1.0;
		else
			layers[0].inStates[i]=1.0;
	for(i=0;i<4;i++)
		layers[0].inStates[4+i]=layers[1].outStates[i];
	layers[0].update(true);
	for(i=0;i<6;i++){
		if(layers[0].outStates[i]>0.0)
			states[8+i]=1;
		else
			states[8+i]=0;
		layers[1].inStates[i]=layers[0].outStates[i];
		layers[2].inStates[i]=layers[0].outStates[i];
	}
	layers[1].update(true);
	layers[2].update(false);
	for(i=0;i<4;i++)
		if(layers[1].outStates[i]>0.0)
			states[4+i]=1;
		else 
			states[4+i]=0;
	for(i=0;i<2;i++)
		if(layers[2].outStates[i]>0.0)
			states[14+i]=1;
		else 
			states[14+i]=0;
	 //*/
	/*
	for(i=0;i<8;i++)
		if(states[i]==0)
			layers[0].inStates[i]=-1.0;
		else
			layers[0].inStates[i]=1.0;
	layers[0].update();
	for(i=0;i<5;i++){
		if(layers[0].outStates[i]>0.0){
			states[8+i]=1;
			layers[1].inStates[i]=1.0;
			layers[2].inStates[i]=1.0;
		}
		else{
			states[8+i]=0;
			layers[1].inStates[i]=-1.0;
			layers[2].inStates[i]=-1.0;
		}
	}
	layers[1].update();
	layers[2].update();
	for(i=0;i<4;i++)
		if(layers[1].outStates[i]>0.0)
			states[4+i]=1;
		else 
			states[4+i]=0;
	for(i=0;i<3;i++)
		if(layers[2].outStates[i]>0.0)
			states[13+i]=1;
		else 
			states[13+i]=0;
	 */
}
void tANN::resetBrain(void){
	int i,j;
	for(i=0;i<layers.size();i++){
		for(j=0;j<layers[i].inStates.size();j++) layers[i].inStates[j]=0.0;
		for(j=0;j<layers[i].outStates.size();j++) layers[i].outStates[j]=0.0;
	}
}

