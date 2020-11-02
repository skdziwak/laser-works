#include <sstream>
#include <string>
#include "utils.h"

using namespace std;

double parseDouble(const string& str) {
    stringstream ss(str);
    double d = 0;
    ss >> d;

    if(ss.fail()) {
        string s = "Unable to format ";
        s += str;
        s += " as a number!";
        throw invalid_argument(s);
    }
    return d;
}