#include <algorithm>
#include <chrono>
#include <tuple>
#include <random>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>

#include "MDP_type_definitions.h"
#include "pretty_printing_MDP.h"
#include "MDP_generation.h"
#include "VI_algorithms_helper_methods.h"
#include "VI_algorithm.h"
#include "BVI_algorithm.h"
#include "VIAE_algorithm.h"
#include "VIAEH_algorithm.h"
#include "VIH_algorithm.h"
#include "experiments.h"

using namespace std;
using namespace std::chrono;

V_type bounded_value_iteration(S_type S, R_type R, A_type A, P_type P, double gamma, double epsilon){
		
		//Find relevant values from the R parameter
		auto [r_star_min, r_star_max, r_star_values] = find_all_r_values(R);

		//Initialize with improved upper bound
		double** V_U = new double*[2];
		double** V_L = new double*[2];
		for(int i = 0; i < 2; ++i) {
				V_U[i] = new double[S];
				V_L[i] = new double[S];
		}
		int siz=sqrt(S-1)-2;
		int Xmax=siz+2;
		siz=Xmax/2;
		for(int s = 0; s < S; s++) {
				V_U[1][s] = 1;
				int x_curr=s%Xmax;
				int y_curr=s/Xmax;
				int xa1= abs(x_curr-siz);
				int ya1= abs(y_curr-siz);
				double x2=0;
				if (xa1>ya1)
					x2=xa1;
				else
					x2=ya1;
				double x1= sqrt( pow( abs(x_curr-siz),2)+pow(abs(y_curr-siz),2));
				
				//double x1= sqrt( pow( abs(x_curr-siz),2)+pow(abs(y_curr-siz),2));
				//V_U[0][s] = (gamma / (1.0 - gamma)) * r_star_max + r_star_values[s];
				V_U[0][s] = -x2;
				//V_U[0][s] = 1.0;
				V_L[0][s] = -x1*5-10;
				V_L[1][s] = 1.0;
		}V_U[0][S-1] = 0;
		V_L[0][S-1] = 0;

		//Initialize with improved lower bound
	

		//Keep track of work done in each iteration in microseconds
		//Start from iteration 1
		vector<microseconds> work_per_iteration(1);

		//Init criteria variables to know which value to return based on why the algorithm terminated
		//Set to true if we have converged!
		bool bounded_convergence_criteria = false;
		bool upper_convergence_criteria = false;
		bool lower_convergence_criteria = false;

		//Pre-compute convergence criteria for efficiency to not do it in each iteration of while loop
		//const double convergence_bound_precomputed = (epsilon * (1.0 - gamma)) / gamma;
		const double convergence_bound_precomputed = 0.0005;
		const double two_epsilon = 2 * epsilon;

		//Keep count of number of iterations
		int iterations = 0;

		//Record actions eliminated in each iteration
		//Push empty vector for 0-index. Iterations start with 1
		vector<vector<pair<int, int>>> actions_eliminated;
		actions_eliminated.push_back(vector<pair<int, int>>());
		gamma=1;
		//while any of the criteria are NOT, !, met, run the loop
		while ( (!bounded_convergence_criteria) && (!upper_convergence_criteria) && (!lower_convergence_criteria) ){

				//Increment iteration counter i
				iterations++;	
				
				//begin timing of this iteration
				auto start_of_iteration = high_resolution_clock::now();
				
				//If i is even, then (i & 1) is 0, and the one to change is V[0]
				double *V_U_current_iteration = V_U[(iterations & 1)];
				double *V_U_previous_iteration = V_U[1 - (iterations & 1)];

				double *V_L_current_iteration = V_L[(iterations & 1)];
				double *V_L_previous_iteration = V_L[1 - (iterations & 1)];
				
				//for all states in each iteration
				for (int s = 0; s < S; s++) {

						//initial values to smalles possible
						//V_U_current_iteration[s] = numeric_limits<double>::min();
						//V_L_current_iteration[s] = numeric_limits<double>::min();
						V_U_current_iteration[s] = -100000;
						V_L_current_iteration[s] = -100000;

						//Ranged for loop over all actions in the action set of state s
						for (auto a : A[s]) {
								auto& [P_s_a, P_s_a_nonzero] = P[s][a];

								double R_U_s_a = R[s][a] + gamma * sum_of_mult_nonzero_only(P_s_a, V_U_previous_iteration, P_s_a_nonzero); 
								if (R_U_s_a > V_U_current_iteration[s]) {
										V_U_current_iteration[s] = R_U_s_a;
								}

								double R_L_s_a = R[s][a] + gamma * sum_of_mult_nonzero_only(P_s_a, V_L_previous_iteration, P_s_a_nonzero); 
								if (R_L_s_a > V_L_current_iteration[s]) {
										V_L_current_iteration[s] = R_L_s_a;
								}
						}
				}

				//see if any of the convergence criteria are met
				//1. bounded criteria
				bounded_convergence_criteria = abs_max_diff(V_U[(iterations & 1)], V_L[(iterations & 1)], S) <= two_epsilon;

				//2. upper criteria
				upper_convergence_criteria = abs_max_diff(V_U[0], V_U[1], S) <= convergence_bound_precomputed;

				//3. lower criteria
				lower_convergence_criteria = abs_max_diff(V_L[0], V_L[1], S) <= convergence_bound_precomputed;
				
				//end timing of this iteration and record it in work vector
				auto end_of_iteration = high_resolution_clock::now();
				auto duration_of_iteration = duration_cast<microseconds>(end_of_iteration - start_of_iteration);
				work_per_iteration.push_back(duration_of_iteration);

		}

		//case return value on which convergence criteria was met
		vector<double> result(S); //set it so have size S from beginning to use copy
		
		if (bounded_convergence_criteria) {
				result = V_upper_lower_average(V_U[(iterations & 1)], V_L[(iterations & 1)], S);
		} else if (upper_convergence_criteria) {
				copy(V_U[(iterations & 1)], V_U[(iterations & 1)] + S, result.begin());
		} else if (lower_convergence_criteria) {
				copy(V_L[(iterations & 1)], V_L[(iterations & 1)] + S, result.begin());
		}
		V_type result_tuple = make_tuple(result, iterations, work_per_iteration, actions_eliminated);
		
		//DEALLOCATE MEMORY
		for(int i = 0; i < 2; ++i) {
				delete [] V_U[i];
		}
		delete [] V_U;
		for(int i = 0; i < 2; ++i) {
				delete [] V_L[i];
		}
		delete [] V_L;

		return result_tuple;
}


V_type bounded_value_iterationGS(S_type S, R_type R, A_type A, P_type P, double gamma, double epsilon){
		
		//Find relevant values from the R parameter
		auto [r_star_min, r_star_max, r_star_values] = find_all_r_values(R);

		//Initialize with improved upper bound
		double** V_U = new double*[1];
		double** V_L = new double*[1];
		for(int i = 0; i < 1; ++i) {
				V_U[i] = new double[S];
				V_L[i] = new double[S];
		}
		//int siz=sqrt(S-1)-2;
		//int Xmax=siz+2;
		//gamma=1;	
		for(int s = 0; s < S; s++) {
			/*
			int x_curr=s%Xmax;
				int y_curr=s/Xmax;
				int xa1= abs(x_curr-siz);
				int ya1= abs(y_curr-siz);
				double x2=0;
				if (xa1>ya1)
					x2=xa1;
				else
					x2=ya1;
				double x1= sqrt( pow( abs(x_curr-siz),2)+pow(abs(y_curr-siz),2));
				V_U[0][s] = -x2+10;
				V_L[0][s] = -x1*5-10; 
				*/
				V_U[0][s] = (gamma / (1.0 - gamma)) * r_star_max + r_star_values[s];
				V_L[0][s] = (gamma / (1.0 - gamma)) * r_star_min + r_star_values[s];
				
				//V_U[0][s] = 1.0;
				
		}
		//V_U[0][S-1] = 0;
		//V_L[0][S-1] = 0;

		//Initialize with improved lower bound

		//Keep track of work done in each iteration in microseconds
		//Start from iteration 1
		vector<microseconds> work_per_iteration(1);

		//Init criteria variables to know which value to return based on why the algorithm terminated
		//Set to true if we have converged!
		bool bounded_convergence_criteria = false;
		bool upper_convergence_criteria = false;
		bool lower_convergence_criteria = false;

		//Pre-compute convergence criteria for efficiency to not do it in each iteration of while loop
		//const double convergence_bound_precomputed = (epsilon * (1.0 - gamma)) / gamma;
		const double two_epsilon = 2 * epsilon;
		const double convergence_bound_precomputed = 0.0005;

		//Keep count of number of iterations
		int iterations = 0;

		//Record actions eliminated in each iteration
		//Push empty vector for 0-index. Iterations start with 1
		vector<vector<pair<int, int>>> actions_eliminated;
		actions_eliminated.push_back(vector<pair<int, int>>());

		//while any of the criteria are NOT, !, met, run the loop
				double *V_U_current_iteration = V_U[0];
				double *V_L_current_iteration = V_L[0];	
				
		while ( (!bounded_convergence_criteria) && (!upper_convergence_criteria) && (!lower_convergence_criteria) ){
					 bounded_convergence_criteria = true;
					 upper_convergence_criteria = true;
					 lower_convergence_criteria = true;
				//Increment iteration counter i
				iterations++;	
				
				//begin timing of this iteration
				auto start_of_iteration = high_resolution_clock::now();
				
				//If i is even, then (i & 1) is 0, and the one to change is V[0]
				
				//for all states in each iteration
				for (int s = 0; s < S; s++) {

						//initial values to smalles possible
						double oldVU=V_U_current_iteration[s];
						double oldVL=V_L_current_iteration[s];
						//Ranged for loop over all actions in the action set of state s
						//double Q_max=numeric_limits<double>::min();
						double Q_max=-100000;
						for (auto a : A[s]) {
								auto& [P_s_a, P_s_a_nonzero] = P[s][a];

								double R_U_s_a = R[s][a] + gamma * sum_of_mult_nonzero_only(P_s_a, V_U_current_iteration, P_s_a_nonzero); 
								double R_L_s_a = R[s][a] + gamma * sum_of_mult_nonzero_only(P_s_a, V_L_current_iteration, P_s_a_nonzero); 

								
								if(R_U_s_a>Q_max)
									Q_max=R_U_s_a;
								if (R_L_s_a > V_L_current_iteration[s]) {
										V_L_current_iteration[s] = R_L_s_a;
								}
						}
						V_U_current_iteration[s] = Q_max;

				if ((V_U_current_iteration[s]-V_L_current_iteration[s])> two_epsilon)
					bounded_convergence_criteria = false;
				if (abs(V_U_current_iteration[s]-oldVU)> convergence_bound_precomputed)
					upper_convergence_criteria=false;
				if (abs(V_L_current_iteration[s]-oldVL)> convergence_bound_precomputed)
					lower_convergence_criteria=false;
				}

				//see if any of the convergence criteria are met
				//1. bounded criteria

				//3. lower criteria
				 
				//end timing of this iteration and record it in work vector
				auto end_of_iteration = high_resolution_clock::now();
				auto duration_of_iteration = duration_cast<microseconds>(end_of_iteration - start_of_iteration);
				work_per_iteration.push_back(duration_of_iteration);

		}

		//case return value on which convergence criteria was met
		vector<double> result(S); //set it so have size S from beginning to use copy
		
		if (bounded_convergence_criteria) {
				result = V_upper_lower_average(V_U[0], V_L[0], S);
		} else if (upper_convergence_criteria) {
				copy(V_U[0], V_U[0] + S, result.begin());
		} else if (lower_convergence_criteria) {
				copy(V_L[0], V_L[0] + S, result.begin());
		}
		V_type result_tuple = make_tuple(result, iterations, work_per_iteration, actions_eliminated);
		
		//DEALLOCATE MEMORY
		for(int i = 0; i < 1; ++i) {
				delete [] V_U[i];
		}
		delete [] V_U;
		for(int i = 0; i < 1; ++i) {
				delete [] V_L[i];
		}
		delete [] V_L;

		return result_tuple;
}

V_type bounded_value_iterationGSTest(S_type S, R_type R, A_type A, P_type P, double gamma, double epsilon){
		
		//Find relevant values from the R parameter
		auto [r_star_min, r_star_max, r_star_values] = find_all_r_values(R);

		//Initialize with improved upper bound
		double** V_U = new double*[1];
		for(int i = 0; i < 1; ++i) {
				V_U[i] = new double[S];
		}
		for(int s = 0; s < S; s++) {
				V_U[0][s] = (gamma / (1.0 - gamma)) * r_star_max + r_star_values[s];
		}

		//Initialize with improved lower bound
		double** V_L = new double*[1];
		for(int i = 0; i < 1; ++i) {
				V_L[i] = new double[S];
		}
		for(int s = 0; s < S; s++) {
				V_L[0][s] = (gamma / (1.0 - gamma)) * r_star_min + r_star_values[s];
		}

		//Keep track of work done in each iteration in microseconds
		//Start from iteration 1
		vector<microseconds> work_per_iteration(1);

		//Init criteria variables to know which value to return based on why the algorithm terminated
		//Set to true if we have converged!
		bool bounded_convergence_criteria = false;
		bool upper_convergence_criteria = false;
		bool lower_convergence_criteria = false;

		//Pre-compute convergence criteria for efficiency to not do it in each iteration of while loop
		const double convergence_bound_precomputed = (epsilon * (1.0 - gamma)) / gamma;
		const double two_epsilon = 2 * epsilon;

		//Keep count of number of iterations
		int iterations = 0;

		//Record actions eliminated in each iteration
		//Push empty vector for 0-index. Iterations start with 1
		vector<vector<pair<int, int>>> actions_eliminated;
		actions_eliminated.push_back(vector<pair<int, int>>());

		//while any of the criteria are NOT, !, met, run the loop
				double *V_U_current_iteration = V_U[0];
				double *V_L_current_iteration = V_L[0];		
		while ( (!bounded_convergence_criteria) && (!upper_convergence_criteria) && (!lower_convergence_criteria) ){
					 bounded_convergence_criteria = true;
					 upper_convergence_criteria = true;
					 lower_convergence_criteria = true;
				//Increment iteration counter i
				iterations++;	
				
				//begin timing of this iteration
				auto start_of_iteration = high_resolution_clock::now();
				
				//If i is even, then (i & 1) is 0, and the one to change is V[0]
				pair<double,double> sm;
				//for all states in each iteration
				for (int s = 0; s < S; s++) {

						//initial values to smalles possible
						double oldVU=V_U_current_iteration[s];
						double oldVL=V_L_current_iteration[s];
						//Ranged for loop over all actions in the action set of state s
						double Q_max=numeric_limits<double>::min();
						double Q_min=numeric_limits<double>::min();
						for (auto a : A[s]) {
								auto& [P_s_a, P_s_a_nonzero] = P[s][a];

								sm=sum_of_mult_nonzero_onlyT(P_s_a, V_U_current_iteration,V_L_current_iteration, P_s_a_nonzero); 
								double R_U_s_a = R[s][a] + gamma * sm.first;
								double R_L_s_a= R[s][a] + gamma * sm.second;


									if(R_U_s_a>Q_max)
										Q_max=R_U_s_a;
								if (R_L_s_a > Q_min) {
										Q_min = R_L_s_a;
								}
						}
						V_U_current_iteration[s] = Q_max;
						V_L_current_iteration[s]=  Q_min;

				if ((V_U_current_iteration[s]-V_L_current_iteration[s])> two_epsilon)
					bounded_convergence_criteria = false;
				if (abs(V_U_current_iteration[s]-oldVU)> convergence_bound_precomputed)
					upper_convergence_criteria=false;
				if (abs(V_L_current_iteration[s]-oldVL)> convergence_bound_precomputed)
					lower_convergence_criteria=false;
				}

				//see if any of the convergence criteria are met
				//1. bounded criteria

				//3. lower criteria
				 
				//end timing of this iteration and record it in work vector
				auto end_of_iteration = high_resolution_clock::now();
				auto duration_of_iteration = duration_cast<microseconds>(end_of_iteration - start_of_iteration);
				work_per_iteration.push_back(duration_of_iteration);

		}

		//case return value on which convergence criteria was met
		vector<double> result(S); //set it so have size S from beginning to use copy
		
		if (bounded_convergence_criteria) {
				result = V_upper_lower_average(V_U[0], V_L[0], S);
		} else if (upper_convergence_criteria) {
				copy(V_U[0], V_U[0] + S, result.begin());
		} else if (lower_convergence_criteria) {
				copy(V_L[0], V_L[0] + S, result.begin());
		}
		V_type result_tuple = make_tuple(result, iterations, work_per_iteration, actions_eliminated);
		
		//DEALLOCATE MEMORY
		for(int i = 0; i < 1; ++i) {
				delete [] V_U[i];
		}
		delete [] V_U;
		for(int i = 0; i < 1; ++i) {
				delete [] V_L[i];
		}
		delete [] V_L;

		return result_tuple;
}