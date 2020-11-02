#include <cstring>
#include <stdio.h>
#include <fstream>
#include <streambuf>
#include <algorithm>
#include <stdlib.h>
#include <sstream>

#include "svg.h"
#include "utils.h"

#define PI 3.14159265

namespace svg {

    using namespace rapidxml;
    using namespace std;

    double* getValues(const string &str, const string name, const int argc) {
        int index = str.find(name);
        int lb_index = index + name.length();
        int rb_index;

        if(index != -1 && str[lb_index] == '(' && str.length() - lb_index > 1 && (rb_index = str.find(")", lb_index + 1)) != -1) {
            string brackets = str.substr(lb_index + 1, rb_index - lb_index - 1);
            int commas = count(brackets.begin(), brackets.end(), ',');
            if(commas + 1 == argc) {
                vector<string> strings;
                int n = -1;
                for(int i = 0 ; i < brackets.length() ; i++) {
                    char c = brackets[i];
                    if(n == -1) {
                        n += 1;
                        strings.push_back(string());
                        strings[n].push_back(c);
                    } else if(c == ',') {
                        n += 1;
                        strings.push_back(string());
                    } else if(c == '-' || c == '.' || (c >= '0' && c <= '9')) {
                        strings[n].push_back(c);
                    }
                }
                double* result = new double[argc];
                for(int i = 0 ; i < argc ; i++) result[i] = parseDouble(strings[i]);
                return result;
            }
        }
        return nullptr;
    }

    //Converts SVG transformations to a matrix
    Transformation parseTransformation(const string str) {
        Transformation t;
        double* values;

        values = getValues(str, "translate", 2);
        if(values != nullptr) {
            t = t * Translation(values[0], values[1]);
            delete[] values;
        }
        values = getValues(str, "scale", 2);
        if(values != nullptr) {
            t = t * Scale(values[0], values[1]);
            delete[] values;
        }
        values = getValues(str, "scale", 1);
        if(values != nullptr) {
            t = t * Scale(values[0], values[0]);
            delete[] values;
        }
        values = getValues(str, "rotate", 3);
        if(values != nullptr) {
            t = t * Rotation(values[1], values[2], values[0] / 180 * PI);
            delete[] values;
        }
        values = getValues(str, "rotate", 1);
        if(values != nullptr) {
            t = t * Rotation(0, 0, values[0] / 180 * PI);
            delete[] values;
        }
        values = getValues(str, "skewX", 1);
        if(values != nullptr) {
            t = t * SkewX(values[0] / 180 * PI);
            delete[] values;
        }
        values = getValues(str, "skewY", 1);
        if(values != nullptr) {
            t = t * SkewY(values[0] / 180 * PI);
            delete[] values;
        }
        values = getValues(str, "matrix", 6);
        if(values != nullptr) {
            t = t * Transformation(values[0], values[1], values[2], values[3], values[4], values[5]);
            delete[] values;
        }
        return t;
    }

    //Splits d attribute of SVG path 
    vector<string>* splitD(const string &d) {
        enum CharType {number, other, white};
        vector<string>* result = new vector<string>();
        string word;
        CharType type = white;
        for(int i = 0 ; i < d.length(); i++) {
            char c = d[i];
            CharType t;

            if(c == ' ' || c == ',') t = white;
            else if(c == '.' || c == '-' || c == 'e' || c == '+' || (c >= '0' && c <= '9')) t = number;
            else t = other;
            if((t == type || type == white) && t != white) word += c;
            else {
                if(word.length() > 0) {
                    result->push_back(word);
                    word.clear();
                }
                if(t != white)
                    word += c;
            }
            type = t;
        }
        if(word.length() > 0) {
            result->push_back(word);
            word.clear();
        }
        return result;
    }

    //Parses SVG nodes recursively. Applies transformation matrices to paths.
    vector<Path>* parseNode(xml_node<> *node, const Transformation t = Transformation()) {
        vector<Path>* result = new vector<Path>();
        for(xml_node<> *n = node->first_node() ; n ; n = n->next_sibling()) {
            char* name = n->name();
            Transformation t2;
            xml_attribute<> *attr = n->first_attribute("transform");
            if(attr) {
                t2 = parseTransformation(string(attr->value()));
            }
            t2 = t * t2;
            if(strcmp(name, "g") == 0) {
                vector<Path>* paths = parseNode(n, t2);
                result->insert(result->end(), paths->begin(), paths->end());
                delete paths;
            } else if(strcmp(name, "path") == 0) {
                xml_attribute<> *attr = n->first_attribute("d");
                if(attr) {
                    string str(attr->value());
                    result->push_back(Path(str, t2));
                }
            }
        }
        return result;
    }


    //Gets n points from vector
    vector<Point> iterateAndGetPoints(int n, vector<string>::iterator &iterator, const vector<string>::iterator &end, Point relative) {
        vector<Point> result;
        for(int i = 0 ; i < n ; i++) {
            if(iterator + 1 >= end) {
                throw out_of_range("Failed loading SVG Path.");
            }
            Point p = relative;
            p.x += parseDouble(*(iterator++));
            p.y += parseDouble(*(iterator++));
            result.push_back(p);
        }

        return result;
    }

    Path::~Path() {
        for(int i = 0 ; i < size() ; i++) {
            delete at(i);
        }
        clear();
    }

    //Parses SVG Path 
    Path::Path(const string &d, Transformation t) : transformation(t) {
        vector<string> *vec = splitD(d);
        vector<string>::iterator iterator = vec->begin(), end = vec->end();

        char command = 0;
        bool relative;
        Point current {0, 0};
        Point starting {0, 0};

        while(iterator < end) {
            try {
                char c = (*iterator)[0];
                if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                    command = (*iterator++)[0];
                    if(command >= 'a') {
                        command -= 32;
                        relative = true;
                    } else relative = false;
                }
                Point r = relative ? current : Point {0, 0};

                if(command == 'M') { //Parses moveto command
                    Point p = iterateAndGetPoints(1, iterator, end, r)[0];
                    current = p;
                    starting = p;
                    command = 'L'; //moveto is treated as implicit lineto for subsequent points
                } else if(command == 'Z') { //Parses closepath command
                    push_back(new Line(current, starting));
                    current = starting;
                } else if(command == 'L') { //Parses lineto command
                    Point p = iterateAndGetPoints(1, iterator, end, r)[0];
                    push_back(new Line(current, p));
                    current = p;
                } else if(command == 'H' || command == 'V') {  //Parses horizontal and vertical lines
                    if(iterator >= end) throw out_of_range("Failed loading SVG Path.");
                    double d = atof((iterator++)->c_str());
                    Point p {0, 0};
                    if(command == 'H') {
                        p.x = d + r.x;
                        p.y = current.y;
                    } else {
                        p.x = current.x;
                        p.y = d + r.y;
                    }
                    push_back(new Line(current, p));
                    current = p;
                } else if(command == 'C') { //Parses cubic bezier curve
                    vector<Point> points = iterateAndGetPoints(3, iterator, end, r);
                    push_back(new CubicBezier(current, points[0], points[1], points[2]));
                    current = points[2];
                } else if(command == 'S') { //Parses cubic bezier curve. Calculates curve's control point.
                    vector<Point> points = iterateAndGetPoints(2, iterator, end, r);
                    CubicBezier* lastElement = dynamic_cast<CubicBezier*>(at(size() - 1));
                    if(lastElement) {
                        Point assumed = (lastElement->p3 - current) * Point {-1, -1} + current;
                        push_back(new CubicBezier(current, assumed, points[0], points[1]));
                        current = points[1];
                    } else {
                        throw invalid_argument("Failed Loading SVG Path. Invalid usage of \"shorthand\".");
                    }
                } else if(command == 'Q') { //Parses quadratic bezier curve
                    vector<Point> points = iterateAndGetPoints(2, iterator, end, r);
                    push_back(new QuadraticBezier(current, points[0], points[1]));
                    current = points[1];
                } else if(command == 'T') { //Parses quadratic bezier curve. Calculates curve's control point. 
                    Point point = iterateAndGetPoints(1, iterator, end, r)[0];

                    QuadraticBezier* lastElement = dynamic_cast<QuadraticBezier*>(at(size() - 1));
                    if(lastElement) {
                        Point assumed = (lastElement->p2 - current) * Point {-1, -1} + current;
                        push_back(new QuadraticBezier(current, assumed, point));
                        current = point;
                    } else {
                        throw invalid_argument("Failed Loading SVG Path. Invalid usage of \"shorthand\".");
                    }
                } else if(command == 'A') {
                    //Not implemented yet
                    iterator += 7;
                }

            } catch(exception& e) {
                throw;
            }
        }

        delete vec;
    }

    //Loads all paths from svg file
    vector<Path>* loadPaths(string path) {
        unsigned int len = 0;
        ifstream file(path, std::ios::binary);
        len = file.tellg();
        file.seekg(0, ios::end);
        len = static_cast<unsigned int>(file.tellg()) - len;
        char *cstr = new char[len + 1];
        cstr[len] = 0;
        file.seekg(0, ios::beg);
        file.read(cstr, len);
        file.close();

        xml_document<> doc;
        doc.parse<0>(cstr);

        xml_node<> *node = doc.first_node("svg");

        vector<Path>* result = parseNode(node, Transformation());

        delete[] cstr;

        return result;
    }

    Transformation::Transformation() {
        for(int y = 0 ; y < 3; y++) for(int x = 0 ; x < 3; x++) matrix[x][y] = 0;
        matrix[0][0] = 1;
        matrix[1][1] = 1;
        matrix[2][2] = 1;
    }

    Transformation::Transformation(double a, double b, double c, double d, double e, double f) : Transformation() {
        matrix[0][0] = a;
        matrix[0][1] = b;
        matrix[1][0] = c;
        matrix[1][1] = d;
        matrix[2][0] = e;
        matrix[2][1] = f;
        matrix[2][2] = 1;
    }

    void Transformation::print() {
        printf("[\n");
        for(int y = 0 ; y < 3; y++) {
            printf("  [%f, %f, %f],\n", matrix[0][y], matrix[1][y], matrix[2][y]);
        }
        printf("]\n");
    }

    Transformation Transformation::operator*(const Transformation& tm) const {
        Transformation result;
        for(int x = 0 ; x < 3 ; x++) {
            for(int y = 0 ; y < 3 ; y++) {
                result.matrix[x][y] = 0;
                for(int i = 0 ; i < 3 ; i++) {
                    result.matrix[x][y] += matrix[i][y] * tm.matrix[x][i];
                }
            }
        }
        return result;
    }

    Point operator*(const Point& p, const Transformation& t) {
        Point result;
        result.x = p.x * t.matrix[0][0] + p.y * t.matrix[1][0] + t.matrix[2][0];
        result.y = p.x * t.matrix[0][1] + p.y * t.matrix[1][1] + t.matrix[2][1];
        return result;
    }

    Point operator*(const Transformation& t, const Point& p) {
        return operator*(p, t);
    }

    Translation::Translation(double x, double y) : Transformation() {
        matrix[2][0] = x;
        matrix[2][1] = y;
    }

    Rotation::Rotation(double x, double y, double a) : Transformation() {
        matrix[0][0] = cos(a);
        matrix[1][0] = -sin(a);
        matrix[0][1] = sin(a);
        matrix[1][1] = cos(a);
        matrix[2][0] = -x * cos(a) + y * sin(a) + x;
        matrix[2][1] = -x * sin(a) - y  * cos(a) + y;
    }

    SkewX::SkewX(double a) : Transformation() {
        matrix[1][0] = tan(a);
    }

    SkewY::SkewY(double a) : Transformation() {
        matrix[0][1] = tan(a);
    }

    Scale::Scale(double x, double y) : Transformation() {
        matrix[0][0] = x;
        matrix[1][1] = y;
    }

    void Line::print() {
        printf("Line from (%f, %f) to (%f, %f)\n", p1.x, p1.y, p2.x, p2.y);
    }

    void CubicBezier::print() {
        printf("CubicBezier (%f, %f), (%f, %f), (%f, %f), (%f, %f)\n", p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y);
    }

    void QuadraticBezier::print() {
        printf("QuadraticBezier (%f, %f), (%f, %f), (%f, %f)\n", p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
    }

    Line::Line(Point p1, Point p2) : p1(p1), p2(p2) {
    }

    CubicBezier::CubicBezier(Point p1, Point p2, Point p3, Point p4) : p1(p1), p2(p2), p3(p3), p4(p4) {}

    QuadraticBezier::QuadraticBezier(Point p1, Point p2, Point p3) : p1(p1), p2(p2), p3(p3) {}

    //Polymorphic classes need clone function to work properly
    Line* Line::clone() {
        return new Line(*this);
    }

    CubicBezier* CubicBezier::clone() {
        return new CubicBezier(*this);
    }

    QuadraticBezier* QuadraticBezier::clone() {
        return new QuadraticBezier(*this);
    }

    //Calculating points for PathElements
    Point Line::getPoint(double t) {
        if(t > 1) t = 1;
        else if(t < 0) t = 0;
        if(p1.x == p2.x) {
            Point p;
            p.x = p1.x;
            p.y = p1.y + (p2.y - p1.y) * t;
            return p;
        } else {
            Point p;
            double a = (p1.y - p2.y) / (p1.x - p2.x);
            double b = p1.y - a * p1.x;
            p.x = p1.x + t * (p2.x - p1.x);
            p.y = a * p.x + b;
            return p;
        }
    }

    Point CubicBezier::getPoint(double t) {
        if(t > 1) t = 1;
        else if(t < 0) t = 0;
        Point p;
        p.x = pow(1.0 - t, 3) * p1.x + 3 * pow(1.0 - t, 2) * t * p2.x + 3 * (1.0 - t) * pow(t, 2) * p3.x + pow(t, 3) * p4.x;
        p.y = pow(1.0 - t, 3) * p1.y + 3 * pow(1.0 - t, 2) * t * p2.y + 3 * (1.0 - t) * pow(t, 2) * p3.y + pow(t, 3) * p4.y;
        return p;
    }

    Point QuadraticBezier::getPoint(double t) {
        if(t > 1) t = 1;
        else if(t < 0) t = 0;
        Point p;
        p.x = pow(1.0 - t, 2) * p1.x + 2.0 * (1.0 - t) * t * p2.x + pow(t, 2) * p3.x;
        p.y = pow(1.0 - t, 2) * p1.y + 2.0 * (1.0 - t) * t * p2.y + pow(t, 2) * p3.y;
        return p;
    }

    //Path needs cloning constructor to avoid memory errors connected with pointers
    Path::Path(const Path& path) {
        this->transformation = path.transformation;
        for(int i = 0 ; i < path.size() ; i++) {
            push_back(path.at(i)->clone());
        }
    }

    Point operator+(const Point& p1, const Point& p2) {
        Point p3;
        p3.x = p1.x + p2.x;
        p3.y = p1.y + p2.y;
        return p3;
    }

    Point operator-(const Point& p1, const Point& p2) {
        Point p3;
        p3.x = p1.x - p2.x;
        p3.y = p1.y - p2.y;
        return p3;
    }

    Point operator*(const Point& p1, const Point& p2) {
        Point p3;
        p3.x = p1.x * p2.x;
        p3.y = p1.y * p2.y;
        return p3;
    }

}