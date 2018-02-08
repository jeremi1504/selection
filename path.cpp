/*
 *  path.cpp
 *  Selection_Recombination
 *
 *  Created by Joshua Schraiber on 4/29/13.
 *  Copyright 2013 UC Berkeley. All rights reserved.
 *
 */

#include "path.h"
#include "math.h"
#include "MbRandom.h"
#include "settings.h"
#include "measure.h"
#include "popsize.h"
#include "param.h"

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>

struct compare_index
{
    const std::vector<double> base_arr;
    compare_index (const std::vector<double> &arr) : base_arr (arr) {}
    
    bool operator () (int a, int b) const
    {
        return (base_arr[a] < base_arr[b]);
    }
};

//builds a bridge from x0 to xt 
path::path(double x0, double xt, double t0, double t, measure* m, settings& s) {
	//create the time vector
	double dt = s.get_dt();
	int min_steps = s.get_grid();
	int steps = (t-t0)/dt+1;
	if (steps < min_steps) {
		steps = min_steps;
	}
	steps += 1;
	dt = (t-t0)/(steps-1);
	time.resize(steps);
	time[0] = t0;
	for (int i = 1; i < steps; i++) {
		time[i] = time[i-1] + dt;
	}
	time[steps-1] = t; //HACK TO MAKE SURE THAT MACHINE ERROR DOESN'T FUCK ME UP
	path* temp = m->prop_bridge(x0, xt, t0, t,time);
	trajectory = temp->get_traj();
	delete temp;
}

//builds a bridge from x0 to xt with a fixed time vector
path::path(double x0, double xt, double t0, double t, measure* m, std::vector<double> tvec) {
	time = tvec;
	path* temp = m->prop_bridge(x0, xt, t0, t,time);
	trajectory = temp->get_traj();
	delete temp;
}

void path::print(std::ostream& o) {
	int i;
	o << "trajectory ";
	for (i = 0; i < trajectory.size(); i++) {
		o << trajectory[i] << " ";
	}
	o << std::endl;
	o << "time ";
	for (i = 0; i < trajectory.size(); i++) {
		o << time[i] << " ";
	}
	o << std::endl;
}

void path::print_tsv(std::ostream& o) {
	int i;
	o << "trajectory\ttime" << std::endl ;
	for (i = 0; i < trajectory.size(); i++) {
		o << trajectory[i] << "\t" << time[i] << std::endl;
	}
}

void path::print_traj(std::ostream& o) {
	int i;
	for (i = 0; i < trajectory.size(); i++) {
		o << trajectory[i] << " ";
	}
	o << std::endl;
}

void path::print_time(std::ostream& o) {
	int i;
	for (i = 0; i < time.size(); i++) {
		o << time[i] << " ";
	}
	o << std::endl;
}

void path::flipCbp() {
	for (int i = 0; i < trajectory.size(); i++) {
		trajectory[i] = PI-trajectory[i];
	}
}

void path::append(path* p) {
	for (int i = 0; i < p->get_length(); i++) {
		trajectory.push_back(p->get_traj(i));
		time.push_back(p->get_time(i));
	}
}

void path::append(path* p, int i) {
	for (int j = i; j < p->get_length(); j++) {
		trajectory.push_back(p->get_traj(j));
		time.push_back(p->get_time(j));
	}
}

void path::insert(path* p, int i) {
	trajectory.insert(trajectory.begin()+i,p->get_traj_iterator(0),p->get_traj_iterator(p->get_length()));
	time.insert(time.begin()+i,p->get_time_iterator(0),p->get_time_iterator(p->get_length()));
}

void path::modify(path* p, int i) {
	if (i != -1 || p != NULL) {
		old_index = i;
		int old_length = p->get_length();
		old_trajectory.resize(0);
		old_time.resize(0);
		for (int j = 0; j < old_length; j++) {
			old_trajectory.push_back(trajectory[i+j]);
			trajectory[i+j] = p->get_traj(j);
			old_time.push_back(time[i+j]);
			time[i+j] = p->get_time(j);
		}
	} else {
		old_trajectory.resize(0);
		old_time.resize(0);
		old_index = -1;
	}
}

void path::reset() {
	if (old_index != -1) {
		for (int j = 0; j < old_trajectory.size(); j++) {
			trajectory[old_index+j] = old_trajectory[j];
			time[old_index+j] = old_time[j];
		}
	}
}

std::vector<double> path::get_time(int i, int j) {
	std::vector<double> timeSlice(0,0);
	for (int k = i; k < j+1; k++) {
		timeSlice.push_back(time[k]);
	}
	return timeSlice;
}

std::vector<double> path::get_traj(int i, int j) {
	std::vector<double> trajSlice(0,0);
	for (int k = i; k < j+1; k++) {
		trajSlice.push_back(trajectory[k]);
	}
	return trajSlice;
}

path* path::extract_path(int i, int j) {
	std::vector<double> new_time(0);
	std::vector<double> new_traj(0);
	for (int k = i; k < j; k++) {
		new_time.push_back(time[k]);
		new_traj.push_back(trajectory[k]);
	}
	path* new_path = new path(new_traj,new_time);
	return new_path;
}

std::vector<double> wfSamplePath::parse_comma_sep(char* c) {
	std::vector<double> pars(0);
	std::string string_pars(c);
	std::istringstream stringstream_pars(string_pars);
	std::string cur_par;
	
	while (std::getline(stringstream_pars,cur_par,',')) {
		pars.push_back(atof(cur_par.c_str()));
	}
	return pars;
}

void wfSamplePath::parse_input_file(std::string fin, int g, double N0) {
    std::cout << "Parsing input" << std::endl;
    std::ifstream inFile(fin.c_str());
    std::string curLineString;
    int curCount;
    int curSS;
    double curLowTime;
    double curHighTime;
    double curTime;
    
    while (getline(inFile, curLineString)) {
        std::istringstream curLine(curLineString);
        curLine >> curCount >> curSS >> curLowTime >> curHighTime;
        if (curCount < 0 || curCount > curSS) {
            std::cout << "Allele count is not between 0 and sample size: X = " << curCount << ", SS = " << curSS << std::endl;
            exit(1);
        }
        if (curLowTime > curHighTime) {
            std::cout << "Low end of time range higher than high end: t_low = " << curLowTime << ", t_high = " << curHighTime << std::endl;
            exit(1);
        }
        if (curLowTime == curHighTime) {
            //Time is not uncertain
            curTime = curLowTime/(g*2*N0);
        } else {
            //Set the sample time to mean to initialize
            std::cout << "Uncertainty in sample age not yet implemented, so just setting to mean" << std::endl;
            curTime = (curLowTime+curHighTime)/2.0/(g*2*N0);
        }
        sampleTimeValues.push_back(curTime);
        sampleSize.push_back(curSS);
        sampleCount.push_back(curCount);
    }
    
    //Check that everything has the same size
    int num_samples = sampleCount.size();
    if (sampleSize.size() != num_samples) {
        std::cout << "Allele frequencies for " << num_samples << " times but sample sizes for " << sampleSize.size() << " times" << std::endl;
        exit(1);
    } else if (sampleTimeValues.size() != num_samples) {
        std::cout << "Allele frequencies for " << num_samples << " times but sample times for " << sampleTimeValues.size() << " times" << std::endl;
        exit(1);
    }
    
    //Sort so that samples are in order by mean
    std::vector<int> index = orderTimeIndex();
    sampleTimeValues = sortByIndex(sampleTimeValues, index);
    sampleSize = sortByIndex(sampleSize, index);
    sampleCount = sortByIndex(sampleCount, index);
}

std::vector<int> wfSamplePath::orderTimeIndex() {
    std::vector<int> index(sampleTimeValues.size());
    for (int i = 0; i < sampleTimeValues.size(); i++) {
        index[i] = i;
    };
    std::sort(index.begin(), index.end(), compare_index(sampleTimeValues));
    return index;
}

std::vector<double> wfSamplePath::sortByIndex(std::vector<double>& vec, std::vector<int> index) {
    std::vector<double> temp_vec(index.size());
    for (int i = 0; i < index.size(); i++) {
        temp_vec[i] = vec[index[i]];
    }
    return temp_vec;
}

double wfSamplePath::sampleProb(int i) {
	int cur_index = sampleTime[i];
	double sp = 0;
	if (cur_index != -1) {
		sp += lgamma(sampleSize[i]+1)-lgamma(sampleCount[i]+1)-lgamma(sampleSize[i]-sampleCount[i]+1);
		sp += sampleCount[i]*log((1.0-cos(trajectory[sampleTime[i]]))/2.0);
		sp += (sampleSize[i]-sampleCount[i])*log(1-(1.0-cos(trajectory[sampleTime[i]]))/2.0);
	} else {
		if (sampleCount[i] == 0) {
			sp += 0;
		} else {
			sp += -INFINITY;
		}
	}
	return sp;
}

std::vector<double> wfSamplePath::sampleProb() {
	std::vector<double> sp(0);
	for (int i = 0; i < sampleTime.size(); i++) {
		sp.push_back(sampleProb(i));
	}
	return sp;
}

void wfSamplePath::print_traj(std::ostream& o) {
	int i;
	for (i = 0; i < trajectory.size(); i++) {
		o << (1.0-cos(trajectory[i]))/2.0 << " ";
	}
	o << std::endl;
}

wfSamplePath::wfSamplePath(std::vector<sample_time*>& times, popsize* p, wfMeasure* wf, settings& s): path() {
    myPop = p;
    
    int num_samples = times.size();
    
    sampleSize.resize(num_samples);
    sampleCount.resize(num_samples);
    sampleTime.resize(0);
    sampleTimeValues.resize(num_samples);
    
    for (int i = 0; i < times.size(); i++) {
        sampleSize[i] = times[i]->get_ss();
        sampleCount[i] = times[i]->get_sc();
        sampleTimeValues[i] = times[i]->get();
    }
    
    //Initialize path
    std::vector<double> initial_data(num_samples);
    first_nonzero = -1;
    for (int i = 0; i < num_samples; i++) {
        initial_data[i] = sampleCount[i]/sampleSize[i];
        if (initial_data[i] == 0) {
            initial_data[i] += exp(-10);
        } else if (initial_data[i] == 1) {
            initial_data[i] -= exp(-10);
        }
        if (first_nonzero == -1 && sampleCount[i] != 0) {
            first_nonzero = i;
        }
    }
    
    std::vector<double> breakPoints;
    for (int i = 0; i < num_samples-1; i++) {
        std::vector<double> curBreaks = myPop->getBreakTimes(sampleTimeValues[i],sampleTimeValues[i+1]);
        for (int j = 0; j < curBreaks.size()-1; j++) {
            breakPoints.push_back(curBreaks[j]);
        }
    }
    breakPoints.push_back(sampleTimeValues[sampleTimeValues.size()-1]);
    
    
    //create the time vector
    double dt = s.get_dt();
    int min_steps = s.get_grid();
    int cur_end_ind = 0;
    int curTimeInd = 1;
    int curSampleInd = 1;
    int curBreakStart = 0;
    path* nextPath;
    
    sampleTime.push_back(cur_end_ind);
    std::vector<double> time_vec;
    time_vec.resize(0);
    time_vec.push_back(breakPoints[0]);
    for (int curBreak = 0; curBreak < breakPoints.size()-1; curBreak++) {
        double curStart = breakPoints[curBreak];
        double curEnd = breakPoints[curBreak+1];
        int steps = (curEnd-curStart)/dt+1;
        if (steps < min_steps) {
            steps = min_steps;
        }
        steps += 1;
        dt = (curEnd-curStart)/(steps-1);
        cur_end_ind++;
        int end_k = time_vec.size()-1+steps;
        for (int k = time_vec.size(); k < end_k; k++) {
            time_vec.push_back(time_vec[k-1]+dt);
            cur_end_ind++;
        }
        time_vec[time_vec.size()-1] = curEnd;
        if (curEnd == sampleTimeValues[curTimeInd]) {
            nextPath = new path(wf->fisher(initial_data[curBreakStart]), wf->fisher(initial_data[curBreakStart+1]), time_vec[0], time_vec[time_vec.size()-1], wf, time_vec);
            if (curBreakStart == 0) {
                this->append(nextPath);
            } else {
                this->append(nextPath,1);
            }
            sampleTime.push_back(cur_end_ind-1);
            curBreakStart++;
            curTimeInd++;
            delete nextPath;
            time_vec.resize(0);
            time_vec.push_back(curEnd);
            
        } 
        cur_end_ind--;
    }
}

wfSamplePath::wfSamplePath(settings& s, wfMeasure* wf) : path() {
	
	sampleSize.resize(0);
	sampleCount.resize(0);
	sampleTime.resize(0);
	
	if (s.get_popFile() == "") {
		std::cout << "ERROR: No population size history specified! Use -P option" << std::endl;
		exit(1);
	}
	

    if (s.get_set_gen() && !s.get_set_N0()) {
        std::cout << "Specified a generation time but not a base population size. Unless your times are measured in units of 2N0 years, this is likely an error" << std::endl;
    } else if (!s.get_set_gen() && s.get_set_N0()) {
        std::cout << "Specified base population size but not a generation time. Assuming times are measured in generations, converting all units to 2N0 generations" << std::endl;
    } else if (s.get_set_gen() && s.get_set_N0()) {
        std::cout << "Specified both generation time and base population size. Assuming times are measured in years, converting all units to 2N0 generations" << std::endl;
    } else {
        std::cout << "Did not specify either generation time or base population size. Assuming times are in units of 2N0 generations" << std::endl;
    }
    
    //make pop sizes
    myPop = new popsize(s);
    
    //Make input data
    if (s.get_infile() == "") {
        std::cout << "No input data specified!" << std::endl;
        exit(1);
    } else {
        parse_input_file(s.get_infile(), s.get_gen_time(), s.get_N0());
    }
    
    //Initialize path
    int num_samples = sampleCount.size();
	std::vector<double> initial_data(num_samples);
	first_nonzero = -1;
	for (int i = 0; i < num_samples; i++) {
		initial_data[i] = sampleCount[i]/sampleSize[i];
		if (initial_data[i] == 0) {
			initial_data[i] += exp(-10);
		} else if (initial_data[i] == 1) {
			initial_data[i] -= exp(-10);
		}
		if (first_nonzero == -1 && sampleCount[i] != 0) {
			first_nonzero = i;
		}
	}
	
	std::vector<double> breakPoints;
	for (int i = 0; i < num_samples-1; i++) {
		std::vector<double> curBreaks = myPop->getBreakTimes(sampleTimeValues[i],sampleTimeValues[i+1]);
		for (int j = 0; j < curBreaks.size()-1; j++) {
			breakPoints.push_back(curBreaks[j]);
		}
	}
	breakPoints.push_back(sampleTimeValues[sampleTimeValues.size()-1]);
	
	
	//create the time vector
	double dt = s.get_dt();
	int min_steps = s.get_grid();
	int cur_end_ind = 0; 
	int curTimeInd = 1;
	int curSampleInd = 1;
	int curBreakStart = 0;
	path* nextPath;
	
    //TODO: MODIFY TO INITALIZE FROM THE MOST ANCIENT END OF THE TIME UNCERTAINTY
	sampleTime.push_back(cur_end_ind);
	std::vector<double> time_vec;
	time_vec.resize(0);
	time_vec.push_back(breakPoints[0]);
	for (int curBreak = 0; curBreak < breakPoints.size()-1; curBreak++) {
		double curStart = breakPoints[curBreak];
		double curEnd = breakPoints[curBreak+1];
		int steps = (curEnd-curStart)/dt+1;
		if (steps < min_steps) {
			steps = min_steps;
		}
		steps += 1;
		dt = (curEnd-curStart)/(steps-1);
		cur_end_ind++;
		int end_k = time_vec.size()-1+steps;
		for (int k = time_vec.size(); k < end_k; k++) {
			time_vec.push_back(time_vec[k-1]+dt);
			cur_end_ind++;
		}
		time_vec[time_vec.size()-1] = curEnd;
		if (curEnd == sampleTimeValues[curTimeInd]) {
			nextPath = new path(wf->fisher(initial_data[curBreakStart]), wf->fisher(initial_data[curBreakStart+1]), time_vec[0], time_vec[time_vec.size()-1], wf, time_vec);
			if (curBreakStart == 0) {
				this->append(nextPath);
			} else {
				this->append(nextPath,1);
			}
			sampleTime.push_back(cur_end_ind-1);
			curBreakStart++;
			curTimeInd++;
			delete nextPath;
			time_vec.resize(0);
			time_vec.push_back(curEnd);
			
		} 
		cur_end_ind--;
	}
}

wfSamplePath::~wfSamplePath() {
	delete myPop;
}

void wfSamplePath::set_allele_age(double a, path* p, int i) {
	int j = 0;
	int k = 0;
	old_age = allele_age;
	allele_age = a;
	old_begin_traj = this->get_traj(0,i);
	old_begin_time = this->get_time(0,i);
	std::vector<double> tempTraj = p->get_traj();
	std::vector<double> tempTime = p->get_time();
	old_index = tempTime.size() - 1; 
	for (j = i+1; j < trajectory.size(); j++) {
		tempTraj.push_back(trajectory[j]);
		tempTime.push_back(time[j]);
	}
	trajectory = tempTraj;
	time = tempTime;
	
	// for sample times that are older than the allele, set the index to -1
	// for others, reindex
	k = 0;
	for (j = 0; j < sampleTime.size(); j++) {
		if (sampleTimeValues[j] < allele_age) {
			sampleTime[j] = -1;
			continue;
		}
		while (k < time.size()) {
			if (sampleTimeValues[j] == time[k]) {
				sampleTime[j] = k;
				k++;
				break;
			} else {
				k++;
			}
		}
	}

}

void wfSamplePath::reset() {
	if (!update_begin && old_index != -1) {
		//if I didn't update the beginning I can just replace the old stuff directly
		for (int j = 0; j < old_trajectory.size(); j++) {
			trajectory[old_index+j] = old_trajectory[j];
			time[old_index+j] = old_time[j];
		}
		old_index = -1;
	} else if (update_begin) {
		//if I did update the begining, I need to prepend the old begining to the rest of the path!
		allele_age = old_age;
		std::vector<double> tempTraj = old_begin_traj;
		std::vector<double> tempTime = old_begin_time;
		for (int j = old_index+1; j < trajectory.size(); j++) {
			tempTraj.push_back(trajectory[j]);
			tempTime.push_back(time[j]);
		}
		trajectory = tempTraj;
		time = tempTime;
		//also need to reset a bunch of other stuff
		int k = 0;
		for (int j = 0; j < sampleTime.size(); j++) {
			if (sampleTimeValues[j] < allele_age) {
				sampleTime[j] = -1;
				continue;
			}
			while (k < time.size()) {
				if (sampleTimeValues[j] == time[k]) {
					sampleTime[j] = k;
					k++;
					break;
				} else {
					k++;
				}
			}
		}
		update_begin = 0;
	}
}

void path::replace_time(std::vector<double> new_time) {
	if (new_time.size() != time.size()) {
		std::cout << "ERROR: Trying to replace a time vector with one of a different size!" << std::endl;
		exit(1);
	}
	time = new_time;
}
