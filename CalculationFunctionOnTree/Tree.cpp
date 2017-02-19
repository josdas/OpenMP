#include <iostream>
#include <cmath>
#include <assert.h>
#include <time.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <functional>
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

struct vertex{
	vertex *left, *right;
	int value;
	vertex(int _value) : value(_value){
		left = NULL;
		right = NULL;
	}
};

void deleteTree(vertex *root){
	if(root != NULL){
		deleteTree(root->left);
		deleteTree(root->right);
		delete root;
	}
}

struct myFunction{
private:
	function<int(int, int)> F;
	int neutralElement;
public:
	myFunction(int _neutralElement, function<int(int, int)> _F) :
			neutralElement(_neutralElement), F(_F) { }
	int getNeutralElement() const {
		return neutralElement;
	}
	int get(int a, int b) const {
		return F(a, b);
	}
};

class FunctionTree{
protected:
	myFunction functionForTree;
public:
	FunctionTree(myFunction _functionForTree) : functionForTree(_functionForTree){ }
	virtual int getFunction(vertex*) = 0;
	virtual string getName() = 0;
};

class ParallelFunctionTree : public FunctionTree{
private:
	int maxLevelWithParallel = 3;
	const string str = "fast";
	int dfs(vertex *root, int maxDownLevel){
		if(root == NULL){
			return functionForTree.getNeutralElement();
		}
		//cout << omp_get_thread_num() << '\n';
		int a, b;
		if(maxDownLevel > 0){
			#pragma omp task shared(a)
			a = dfs(root->left, maxDownLevel - 1);
			#pragma omp task shared(b)
			b = dfs(root->right, maxDownLevel - 1);
			#pragma omp taskwait
		}
		else{
			a = dfs(root->left, maxDownLevel);
			b = dfs(root->right, maxDownLevel);
		}
		return functionForTree.get(functionForTree.get(a, root->value), b);
	}
public:
	ParallelFunctionTree(myFunction _functionForTree) : FunctionTree(_functionForTree){ }
	ParallelFunctionTree(myFunction _functionForTree, int _maxLevelWithParallel) :
			FunctionTree(_functionForTree), maxLevelWithParallel(_maxLevelWithParallel){ }
	string getName(){
		return str;
	}
	int getFunction(vertex *root){
		int result;
		#pragma omp parallel shared(result)
		#pragma omp single
		result = dfs(root, maxLevelWithParallel);
		return result;
	}
};

class SlowFunctionTree : public FunctionTree{
private:
	const string str = "slow";
	int dfs(vertex *root){
		if(root == NULL){
			return functionForTree.getNeutralElement();
		}
		int a, b;
		a = dfs(root->left);
		b = dfs(root->right);
		return functionForTree.get(functionForTree.get(a, root->value), b);
	}
public:
	SlowFunctionTree(myFunction _functionForTree) : FunctionTree(_functionForTree){ }
	string getName(){
		return str;
	}
	int getFunction(vertex *root){
		int result;
		result = dfs(root);
		return result;
	}
};

vertex *generationFullBinaryTree(int hight){
	if(hight == 0){
		return NULL;
	}
	vertex *root = new vertex(rand());
	root->left = generationFullBinaryTree(hight - 1);
	root->right = generationFullBinaryTree(hight - 1);
	return root;
}

vertex *generationRandomBinaryTree(int numberOfVertexInSubtree){
	if(numberOfVertexInSubtree == 0){
		return NULL;
	}
	vertex *root = new vertex(rand());
	int s = rand() % numberOfVertexInSubtree;
	root->left = generationRandomBinaryTree(s);
	root->right = generationRandomBinaryTree(numberOfVertexInSubtree - s - 1);
	return root;
}

int testTree(vertex *root, FunctionTree *T, string conditionOfTest){
	Timer timer;
	int result = T->getFunction(root);
	printf("%s Time of %s solution: %0.3f\n", conditionOfTest.c_str(), T->getName().c_str(), timer.getTime());
	fprintf(stderr, "%s Time of %s solution: %0.3f\n", conditionOfTest.c_str(), T->getName().c_str(), timer.getTime());
	return result;
}

int functionSum(int a, int b){
	return a + b;
}

void genTestCompFullBinaryTree(int maxHight){
	myFunction myfunction(0, functionSum);
	for(int i = 1; i < maxHight; i++){
		vertex *root = generationFullBinaryTree(i);

		SlowFunctionTree slow(myfunction);
		testTree(root, &slow, "Full binary tree of " + toString(i) + "hight.");

		ParallelFunctionTree fast(myfunction);
		testTree(root, &fast, "Full binary tree of " + toString(i) + "hight.");

		deleteTree(root);
	}
}


void genTestCompRandomBinaryTree(int maxLog){
	myFunction myfunction(0, functionSum);
	for(int i = 1; i < maxLog; i++){
		vertex *root = generationRandomBinaryTree(1 << i);

		SlowFunctionTree slow(myfunction);
		testTree(root, &slow, "Random binary tree of " + toString(i) + "hight.");

		ParallelFunctionTree fast(myfunction);
		testTree(root, &fast, "Random binary tree of " + toString(i) + "hight.");

		deleteTree(root);
	}
}
int main(){
	freopen("compare_slow_fast_full_random_tree.txt", "w", stdout);
	srand(time(0));
	omp_set_nested(true);
	genTestCompRandomBinaryTree(27);
}
