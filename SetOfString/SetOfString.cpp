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
#include <map>
#include <vector>
#include <fstream>
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

class Timer{
private:
    double startTime;
public:
    Timer(){
        startTime = getCurTime();
    }
    Timer(double time) : startTime(time){ }
    void setStartTime(double time){
        startTime = time;
    }
    double getTime() const {
        return getCurTime() - startTime;
    }
};

class myLock{
private:
    omp_lock_t* T;
    bool isParallel;
public:
    myLock(){
        T = new omp_lock_t();
        isParallel = true;
        omp_init_lock(T);
    }
    myLock(bool _isParallel) : isParallel(_isParallel){
        if(isParallel){
            omp_set_lock(T);
        }
    }
    ~myLock(){
        if(isParallel){
            omp_destroy_lock(T);
        }
        delete T;
    }
    void set(){
        if(isParallel){
            omp_set_lock(T);
        }
    }
    void unset(){
        if(isParallel){
            omp_unset_lock(T);
        }
    }
};

class StringSet{ // Хотелось бы добавить сюда getName(), но str у каждого свой
public:
    virtual void addString(const string&) = 0;
    virtual int count(const string&) = 0;
    virtual string getName() = 0;
};

class StlSet : public StringSet{
private:
    const string str = "stl";
    map<string, int> T;
public:
    string getName(){
        return str;
    }
    void addString(const string& s){
        T[s]++;
    }
    int count(const string& s){
        auto t = T.find(s);
        if(t == T.end()){
            return 0;
        }
        return t->second;
    }
};


class Trie : public StringSet{
private:
    const string str = "trie";
    int alphabet, size;
    vector<vector<int> > nextVertex;
    vector<int> numberString;
    vector<myLock> lock;
    int next;
    int createNewVertex(){
        int temp;
        #pragma omp critical
        temp = next++;
        return temp;
    }
    int getNext(int v, int nextChar) const {
        return nextVertex[v][nextChar];
    }
    void setNext(int v, int nextChar, int temp){
        nextVertex[v][nextChar] = temp;
    }
    void addVertex(int v, int s){
        #pragma omp atomic
        numberString[v] += s;
    }
    int getValue(int v) const {
        return numberString[v];
    }
    void setLock(int v){
        lock[v].set();
    }
    void unsetLock(int v){
        lock[v].unset();
    }
public:
    Trie(int _alphabet, int _size) : alphabet(_alphabet), size(_size){
        next = 0;
        lock.resize(size);
        numberString.resize(size);
        nextVertex.resize(size, vector<int> (alphabet));
        createNewVertex();
    }
    string getName(){
        return str;
    }
    void print(){
        printf("It's trie{\n");
        for(int i = 0; i < size; i++){
            for(int j = 0; j  < alphabet; j++){
                printf("%d ", nextVertex[i][j]);
            }
            printf("\n");
        }
        printf("}\n");
    }
    void addString(const string& s){
        int curVertex = 0;
        for(int i = 0; i < (int)s.size(); i++){
            int ch = s[i] - 'a';
            setLock(curVertex);
            if(getNext(curVertex, ch) == 0){
                setNext(curVertex, ch, createNewVertex());
            }
            int tempVertex = getNext(curVertex, ch);
            unsetLock(curVertex);
            curVertex = tempVertex;
        }
        addVertex(curVertex, 1);
    }
    int count(const string& s){
        int curVertex = 0;
        for(int i = 0; i < (int)s.size(); i++){
            int ch = s[i] - 'a';
            //setLock(curVertex);
            int tempVertex = getNext(curVertex, ch);
            //unsetLock(curVertex);
            if(tempVertex == 0){
                return 0;
            }
            curVertex = tempVertex;
        }
        int result;
        result = getValue(curVertex);
        return result;
    }
};

struct Test{
private:
    vector<string> str;
    vector<string> query;
    string parameter;
public:
    Test(string _parameter) : parameter(_parameter){ }
    void addStr(const string &s){
        str.push_back(s);
    }
    void addQuery(const string &s){
        query.push_back(s);
    }
    string getStr(int index) const {
        return str[index];
    }
    string getQuery(int index) const {
        return query[index];
    }
    string getParameter() const {
        return parameter;
    }
    int getNumberStr() const {
        return str.size();
    }
    int getNumberQuery() const {
        return query.size();
    }
    void print(const char *name){
	    ofstream out(name, std::ofstream::out);
        out << getParameter() << '\n';
        out << getNumberStr() << ' ' << getNumberQuery() << '\n';
        for(auto s : str){
            out << s << '\n';
        }
        out << "\n\n";
        for(auto s : query){
            out << s << '\n';
        }
	    out.close();
    }
};


Test readTest(const char *name){
    ifstream in(name);
    string paramter;
    getline(in, paramter);
    Test test(paramter);
    int n, m;
    in >> n >> m;
    for(int i = 0; i < n; i++){
        string s;
        in >> s;
        test.addStr(s);
    }
    for(int i = 0; i < m; i++){
        string s;
        in >> s;
        test.addQuery(s);
    }
    in.close();
    return test;
}

void printVector(const char *name, const vector<int> &T){
    ofstream out(name, std::ofstream::out);
    out << T.size() << '\n';
    for(auto s : T){
        out << s << ' ';
    }
    out << '\n';
}

vector<int> runTestParallel(const Test &test, StringSet* T){
    Timer timer;
    #pragma omp parallel for
    for(int i = 0; i < test.getNumberStr(); i++){
        string t = test.getStr(i);
        T->addString(t);
    }
    #pragma omp barrier

    vector<int> result(test.getNumberQuery());
    #pragma omp parallel for shared(result)
    for(int i = 0; i < test.getNumberQuery(); i++){
        string t = test.getQuery(i);
        result[i] = T->count(t);
    }
    #pragma omp barrier

    printf("%s Parallel solution: %s. Time of solution: %0.3f\n",
        test.getParameter().c_str(), T->getName().c_str(), timer.getTime());
    return result;
}

vector<int> runTestSingle(const Test &test, StringSet* T){
    Timer timer;
    for(int i = 0; i < test.getNumberStr(); i++){
        T->addString(test.getStr(i));
    }
    vector<int> result(test.getNumberQuery());
    for(int i = 0; i < test.getNumberQuery(); i++){
        result[i] = T->count(test.getQuery(i));
    }
    printf("%s Single solution: %s. Time of solution: %0.3f\n",
        test.getParameter().c_str(), T->getName().c_str(), timer.getTime());
    return result;
}

string generatorString(int length, int alphabet){
    string result;
    for(int i = 0; i < length; i++){
        result += rand() % alphabet + 'a';
    }
    return result;
}

Test generatorRandomTest(int length, int alphabet, int numberString, int numberQuery){
    Test test("Random string of length " + toString(length) +
        " with alphabet " + toString(alphabet) + ". " +
        "Number of strings equal to " + toString(numberString) + ". " +
        "Number of quires equal to " + toString(numberQuery) + ".");

    for(int i = 0; i < numberString; i++){
        test.addStr(generatorString(length, alphabet));
    }
    for(int i = 0; i < numberQuery; i++){
        test.addQuery(generatorString(length, alphabet));
    }
    return test;
}

void compareSolutions(vector<StringSet*> solutions, vector<bool> isParallel, Test test){
    vector<vector<int> > answers(solutions.size());
    for(int i = 0; i < solutions.size(); i++){
        if(isParallel[i]){
            answers[i] = runTestParallel(test, solutions[i]);
        }
        else{
            answers[i] = runTestSingle(test, solutions[i]);
        }
    }
    for(int i = 0; i < (int)answers.size(); i++){
        if(answers[0] != answers[i]){
            printf("Something went wrong.\n");
            test.print("BadTest.txt");
            printVector("ResultA.txt", answers[0]);
            printVector("ResultB.txt", answers[i]);
            assert(0);
        }
    }
}

void stressTest(){
    int t = 0;
    while(1){
        t++;
        int n = rand() % 300 + 1;
        int a = rand() % 20 + 1;
        int q = rand() % 500000 + 1;
        int w = rand() % 500000 + 1;
        Test test = generatorRandomTest(n, a, q, w);
        StlSet A;
        Trie B(a, n * q + 1);
        compareSolutions({&A, &B}, {0, 1}, test);
        cout << t << "\n";
        cerr << t << "\n";
    }
}

int main(){
    freopen(".txt", "w", stdout);
    srand(time(0));
    stressTest();
}
