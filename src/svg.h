#pragma once

#include <vector>
#include <string>
#include <math.h>
#include "rapidxml.hpp"

namespace svg {

    using namespace std;

    struct Point {
        double x = 0;
        double y = 0;
    };


    class Transformation {
    protected:
        double matrix[3][3]; //All transformations are 3x3 matrices
    public:
        Transformation();
        Transformation(double, double, double, double, double, double);
        Transformation operator*(const Transformation&) const; //Transformations can be combined by multiplying them
        friend Point operator*(const Point&, const Transformation&);
        friend Point operator*(const Transformation&, const Point&);
        void print();
    };

    Point operator+(const Point&, const Point&);
    Point operator-(const Point&, const Point&);
    Point operator*(const Point&, const Point&);


    class Translation : public Transformation {
    public:
        Translation(double, double);
    };

    class Rotation : public Transformation {
    public:
        Rotation(double, double, double);
    };

    class SkewX : public Transformation {
    public:
        SkewX(double);
    };

    class SkewY : public Transformation {
    public:
        SkewY(double);
    };

    class Scale : public Transformation {
    public:
        Scale(double, double);
    };

    class PathElement {
    private:
        friend class Path;
    
    public:
        virtual ~PathElement() {};
        virtual Point getPoint(double t) {return Point{0, 0};}; //Calculates point for 0 <= t <= 1
        virtual void print() = 0;
        virtual PathElement* clone() = 0;
    };

    class Path : public vector<PathElement*> { //Path is a vector of PathElements with transformation
    private:
        Transformation transformation;
    
    public:
        Path(const string&, Transformation t); //First argument is 'd' attribute of SVG path.
        Path(const Path& path);
        ~Path();
        Transformation getTransformation() {return transformation;};
    };
    
    //The function that loads a vector of all paths from SVG file
    vector<Path>* loadPaths(string path);


    class Line : public PathElement {
    private:
        friend class Path;
        Point p1;
        Point p2;
    public:
        Line(Point, Point);
        void print();
        Point getPoint(double);
        Line* clone();
        Point getP1() {return p1;};
        Point getP2() {return p2;};
    };

    class CubicBezier : public PathElement {
    private:
        friend class Path;
        Point p1;
        Point p2;
        Point p3;
        Point p4;
    public:
        CubicBezier(Point, Point, Point, Point);
        void print();
        Point getPoint(double);
        CubicBezier* clone();
        Point getP1() {return p1;};
        Point getP2() {return p2;};
        Point getP3() {return p3;};
        Point getP4() {return p4;};
    };

    class QuadraticBezier : public PathElement {
    private:
        friend class Path;
        Point p1;
        Point p2;
        Point p3;
    public:
        QuadraticBezier(Point, Point, Point);
        void print();
        Point getPoint(double);
        QuadraticBezier* clone();
        Point getP1() {return p1;};
        Point getP2() {return p2;};
        Point getP3() {return p3;};
    };

    class Arc : public PathElement {

    };

    //No need to use those from the outside
    vector<Path>* parseNode(rapidxml::xml_node<> *node);
    vector<string>* splitD(const string &d);
    Transformation parseTransformation(const string str);
    double* getValues(const string &str, const string name, const int argc);
    vector<Point> iterateAndGetPoints(int n, vector<string>::iterator &iterator, const vector<string>::iterator &end, Point relative);

}