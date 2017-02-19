#include <iostream>
#include <cmath>
#include <assert.h>
#include <time.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include "omp.h"
using namespace std;

typedef long long ll;

double getCurTime(){
	return clock() * 1.0 / CLOCKS_PER_SEC;
}

template <typename T>
string toString(T temp){
	ostringstream ss;
	ss << temp;
	return ss.str();
}

struct Timer{
	double startTime;
	Timer(){
		startTime = getCurTime();
	}
	Timer(double time) : startTime(time){ }
	void setStartTime(double time){
		startTime = time;
	}
	double getTime(){
		return getCurTime() - startTime;
	}
};

class Fibonacci{
public:
	virtual int get(int) = 0;
	virtual string getName() = 0;
};

class ParallelFib : public Fibonacci{
private:
	const string str = "fast";
	int minWithoutParallel = 4;
	int F(int n, int threshold){
		if (n == 1 || n == 2){
			return 1;
		}
		int a, b;
		if (n > threshold){
            #pragma omp task shared(a)
			a = F(n - 1, threshold);
            #pragma omp task shared(b)
			b = F(n - 2, threshold);
            #pragma omp taskwait
		}
		else{
			a = F(n - 1, threshold);
			b = F(n - 2, threshold);
		}
		return a + b;
	}
public:
	ParallelFib(){ }
	ParallelFib(int _minWithoutParallel) : minWithoutParallel(_minWithoutParallel){ }
	virtual string getName(){
		return str + "(" + toString(minWithoutParallel) + ")";
	}
	int get(int n){
		int result;
        #pragma omp parallel shared(result)
        #pragma omp single
		result = F(n, n - minWithoutParallel);
		return result;
	}
};

class SlowFib : public Fibonacci{
private:
	const string str = "slow";
	int F(int n) {
		if (n == 1 || n == 2){
			return 1;
		}
		int a, b;
		a = F(n - 1);
		b = F(n - 2);
		return a + b;
	}
public:
	SlowFib(){ }
	virtual string getName(){
		return str;
	}
	virtual int get(int n) {
		return F(n);
	}
};

int runTest(int n, Fibonacci *T){
	Timer timer;
	int result = T->get(n);
	printf("n equals %d. Time of %s solution: %0.3f\n", n, T->getName().c_str(), timer.getTime());
	fprintf(stderr, "n equals %d. Time of %s solution: %0.3f\n", n, T->getName().c_str(), timer.getTime());
	return result;
}

void genTestForConst(int n){
	for (int i = 25; i > 0; i--){
		ParallelFib T(i);
		runTest(n, &T);
	}
}

void genTestComp(int n){
	for (int i = 1; i < n; i++){
		SlowFib Slow;
		runTest(i, &Slow);

		ParallelFib Fast;
		runTest(i, &Fast);
	}
}

int main(){
	freopen("compare_slow_fast_4.txt", "w", stdout);
	omp_set_nested(true);
	genTestComp(50);
}
