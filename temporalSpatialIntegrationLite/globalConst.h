/*
 *  globalConst.h
 *  HMMBrain
 *
 *  Created by Arend on 9/16/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 * Incorporate new metrics for quantitative analysis of behavior
 * Namely a vector with the actions (0,1,-1) to measure how much it moves
 */

#ifndef _globalConst_h_included_
#define _globalConst_h_included_
#define glucose_project 1 // 1 (agent->glucose_current_value >= agent->glucose_threshold)
#define glucose_node 4
#define maxNodes 8
#define definedCue 0
// vFitnessFunction 0 = correct, 1 MI(S,M), 2 Predictive Codding(H,S)
#define vFitnessFunction 1
#define NumSensors 2
#define loopTicks 36
#define step 0
#define LOD_record_Intervall 511
//#define LOD_record_Intervall 63
#define wpc 0.0
#define wco 0.2
#define wmi 0.8
#define cc 36
#define dd 56
//FitnessFunct for mutual information
//#define Fitness_F 1;
//#define Fitness_GA 0;
#endif
