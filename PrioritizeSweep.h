#ifndef PRIORITIZE_SWEEP_H
#define PRIORITIZE_SWEEP_H
#include <queue>
#include <utility>
#include <iostream>
#include <vector>  // for std::pair
#include <queue>
#include <functional>
#include "MDP_type_definitions.h"
#include "VI_algorithms_helper_methods.h"
#include "updateable_priority_queue.h"
using ComparatorType = std::function<bool(std::pair<double, int>, std::pair<double, int>)>;


const auto cmp = [](std::pair<double, int> a, std::pair<double, int> b) {
		if (a.first == b.first) {
			return a.second > b.second; 
		} else {
			return a.first < b.first;
	}
    };

void performIterationUPDPred(int s, A_type &A, R_type &R, P_type &P, double gamma, double* V_current_iteration,
                      better_priority_queue::updatable_priority_queue<int, double>& PriorityHeap,
                      std::vector<int>& policy, std::vector<std::vector<int>>& predecessor,
                      double* reverseV,double convergence_bound_precomputed);

void performIterationUPDprestep(int S, A_type &A, R_type &R, P_type &P, double gamma, double* V_current_iteration,
                      better_priority_queue::updatable_priority_queue<int, double>& PriorityHeap,
                      std::vector<int>& policy, std::vector<std::vector<int>>& predecessor,
                      double* reverseV);

void performIteration(int S, A_type &A, R_type &R, P_type &P, double gamma, double* V_current_iteration,
                      std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>, ComparatorType>& PriorityHeap,
                      std::vector<int>& policy, std::vector<std::vector<int>>& predecessor,
                      double* reverseV);
void performIterationUP(int S, A_type &A, R_type &R, P_type &P, double gamma, double* V_current_iteration,
                      std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>, ComparatorType>& PriorityHeap,
                      std::vector<int>& policy, std::vector<std::vector<int>>& predecessor,
                      double* reverseV);
void performIterationPred(int s, A_type &A, R_type &R, P_type &P, double gamma, double* V_current_iteration,
                      std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>, ComparatorType>& PriorityHeap,
                      std::vector<int>& policy, std::vector<std::vector<int>>& predecessor,
                      double* reverseV,double convergence_bound_precomputed);
void performIterationPredUP(int s, A_type &A, R_type &R, P_type &P, double gamma, double* V_current_iteration,
                      std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>, ComparatorType>& PriorityHeap,
                      std::vector<int>& policy, std::vector<std::vector<int>>& predecessor,
                      double* reverseV,double convergence_bound_precomputed);

#endif //PRIORITIZE_SWEEP_H