#include <iostream>
#include <opencv2/objdetect.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <zconf.h>

#include "sio_client.h"
#include "sio_message.h"
#include "sio_socket.h"

#include "curl/curl.h"


using namespace std;
using namespace cv;

struct Boundary {
    Scalar min;
    Scalar max;
    string name;
};

struct Line {
    Point from;
    Point to;
public:
    pair<double, double> findEquation() {
        double a = (this->from.y - this->to.y) / ((this->from.x - this->to.x != 0) ? this->from.x - this->to.x : 0.00001);
        double b = this->from.y - a * this->from.x;
        return make_pair(a, b);
    }
};

class Board {
private:
    vector<Line> vertical;
    vector<Line> horizontal;
    vector<vector<char> > board;
public:
    Board(vector<Line> &vertical, vector<Line> &horizontal, unsigned int size) {
        this->vertical = vertical;
        this->horizontal = horizontal;
        this->board.resize(size);
        for(auto row : board) {
            row.resize(size);
            for(auto cell : row) cell = '0';
        }
    }
    pair<unsigned int, unsigned int> locate(const Point_<int>& coordinates) {
        auto vertical = this->vertical;
        auto horizontal = this->horizontal;

        for(int i = 0; i < vertical.size(); i++) {

            if((0 >
                    (vertical[i].to.x - vertical[i].from.x) *
                    (coordinates.y - vertical[i].from.y) -
                    (vertical[i].to.y - vertical[i].from.y) *
                    (coordinates.x - vertical[i].from.x))) {
                if(i == 0) return make_pair(-1, -1);
                for (int j = 0; j < horizontal.size(); ++j) {
                    if((0 >
                            (horizontal[j].to.x - horizontal[j].from.x) *
                            (coordinates.y - horizontal[j].from.y) -
                            (horizontal[j].to.y - horizontal[j].from.y) *
                            (coordinates.x - horizontal[j].from.x))) {
                        if(j == 0) return make_pair(-1, -1);
                        return make_pair(i - 1, j - 1);
                    }
                }
            }
        }
        return make_pair(-1, -1);

    };


};
Line make_line(Point& from, Point& to) {
    Line l;
    l.from = from;
    l.to = to;
    return l;
}

struct Pawn {
    unsigned short int rows;
    unsigned short int columns;
    char colour;
};

class Broadcast {
private:
    string address;
    string flushAddress;
public:
    Broadcast(string& address){
        this->address = address;
        this->flushAddress = address + ((address[address.length() - 1] == '/') ? "flush" : "/flush");

    }
    void start() {
        CURL* curl;
        CURLcode res;
        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, this->address.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "start=true");
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        cout<<res<<endl;
    }
    void update(vector<Pawn> &positions) {

        for(int i = 0; i<8; i++) {
           // cout<<i;
            string position = "00000000";
            for(auto &pawn : positions) {
                if(pawn.rows == i) position[pawn.columns] = pawn.colour;
            }
            CURLcode res;
            CURL* curl;
            curl = curl_easy_init();
            curl_easy_setopt(curl, CURLOPT_URL, this->address.c_str());
            stringstream ss;
            ss<<"row"<<i<<'='<<position;
            const char* param = ss.str().c_str();
            //cout<<param<<endl;
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(param));
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, param);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

        }
        CURL* curl;
        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, this->flushAddress.c_str());
        CURLcode res;
        res = curl_easy_perform(curl);
        cout<<res<<endl;
        curl_easy_cleanup(curl);


    }
};

vector<Boundary> readHSVBounds(string path) {
    ifstream file;
    file.open(path, fstream::in);

    vector<Boundary> v;

    while(true) {
        string s;
        string name;
        file>>name;
        if(file.eof()) break;
        getline(file, s);
        int min[3];
        int max[3];

        sscanf(s.c_str(), " [%d, %d, %d] [%d, %d, %d]",
               &min[0], &min[1], &min[2], &max[0], &max[1], &max[2]);

        Boundary scalars;
        scalars.min = Scalar(min[0], min[1], min[2]);
        scalars.max = Scalar(max[0], max[1], max[2]);
        scalars.name = name;
        v.push_back(scalars);
    }

    return v;
}

vector<Line> getBars(vector<Point2f> corners, int count, bool orientation) { //true = top-to-bottom, false = left-to-right
    if(corners.size() != 4) {
        vector<Line> grid;
        return grid;
    }

    double difX = abs(orientation ? corners[0].x - corners[2].x : corners[2].x - corners[3].x);
    double difY = abs(orientation ? corners[0].y - corners[2].y : corners[2].y - corners[3].y);

    vector<Line> grid;

    for (int j = 0; j < count + 1; j++) {
        Line newLine;
        newLine.from = orientation ? corners[0] : corners[1];
        newLine.to = orientation ? corners[1] : corners[3];
        newLine.from.x += j * (difX/count) * (orientation ? 1 :-1);
        newLine.from.y += j * (difY/count);
        newLine.to.x += j * (difX/count) * (orientation ? 1 :-1);
        newLine.to.y += j * (difY/count);
        grid.push_back(newLine);
    }


    return grid;
}

vector<Point2f> findItems(Mat& src, Mat& dst, Boundary& boundary, Mat& kernel) {


    inRange(src, boundary.min, boundary.max, dst);
    erode(dst, dst, kernel, Point(-1,-1), 1);
    dilate(dst, dst, kernel, Point(-1,-1), 1);



    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;

    Canny(dst, dst, 100, 200);

    findContours(dst, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

    vector<int> indices(contours.size());
    iota(indices.begin(), indices.end(), 0);

    sort(indices.begin(), indices.end(), [&contours](int l, int r) {
        return contours[l].size() > contours[r].size();
    });

    int N = (int)contours.size();

    vector<Point2f> points;

    for (int i = 0; i < N; ++i) {
        auto point2f = Point2f(0, 0);
        for (auto point : contours[indices[i]]) {
            point2f.x += point.x;
            point2f.y += point.y;
        }

        point2f.x /= contours[indices[i]].size();
        point2f.y /= contours[indices[i]].size();

        points.push_back(point2f);
    }

    return points;
}

int main(int argc, char** argv)
{
    cout<<"Address to connect to?";
    string address;
    cin>>address;
    if(address == ".") {
        address = "localhost:3000/";
    }
    Broadcast broadcast(address);
    broadcast.start();
////    while(true) {
//        vector<Pawn> v;
//        Pawn p;
//        p.colour = '1';
//        p.rows = 1;
//        p.columns = 2;
//        v.push_back(p);
//        std::cout<<"Broadcasting..."<<endl;
//        broadcast.update(v);
////    }
//    cin.ignore();
    int devNo;
    cout<<"Device number?";
    cin>>devNo;
    cv::VideoCapture cap(devNo);
    if(!cap.isOpened()){
        cout<<"Cannot connect to video";
        return -1;
    }

    auto bounds = readHSVBounds("bounds.hsv");


    Mat display;
    Mat hsv;
    namedWindow("Display", WINDOW_AUTOSIZE);
    moveWindow("Display", 100, 600);
    auto kernel = getStructuringElement(CV_SHAPE_RECT, Size(5, 5));
    Boundary p1, p2, edges;
    for (auto color : bounds) {
        cout << color.name;
        if (color.name == "EdgeMarkers") edges = color;
        if (color.name == "Player1") p1 = color;
        if (color.name == "Player2") p2 = color;
    }

//    Broadcast broadcast(address);
//    broadcast.start();
    while(true) {
        cap >> display;
        cvtColor(display, hsv, COLOR_BGR2HSV);
        Mat tmp;
        auto boardEdges = findItems(hsv, tmp, edges, kernel);

        if(boardEdges.size() == 4) {

            vector<int> coordinateIndices(4);
            iota(coordinateIndices.begin(), coordinateIndices.end(), 0);
            sort(coordinateIndices.begin(), coordinateIndices.end(), [&boardEdges](int l, int r) {
                return (boardEdges[l].x < boardEdges[r].x);
            });

            vector<Point2f> sortedPoints(4);
            for (int i = 0; i < 4; ++i) {
                sortedPoints[i] = boardEdges[coordinateIndices[i]];
            }

            for(int i = 0; i<sortedPoints.size(); i++) {
                putText(display, to_string(i), sortedPoints[i], FONT_HERSHEY_COMPLEX,5, Scalar(255,0,0), 2);
            }


            vector<Line> vertical = getBars(sortedPoints, 8, true);
            vector<Line> horizontal = getBars(sortedPoints, 8, false);

            for(int i = 0; i < vertical.size() && i < horizontal.size(); i++) {
                arrowedLine(display, vertical[i].from, vertical[i].to, Scalar(0, 0, 255), (i+2)/2);
                arrowedLine(display, horizontal[i].from, horizontal[i].to, Scalar(0,255,0), (i+2)/2);
            }

            Board board(vertical, horizontal, 8);

            auto p1Pawns = findItems(hsv, tmp, p1, kernel);
            auto p2Pawns = findItems(hsv, tmp, p2, kernel);
            pair<unsigned short, unsigned short> coords;
            Pawn p;
            vector<Pawn> wrappedPawns;
            for(auto const &pawn : p1Pawns) {
                circle(display, pawn, 2, Scalar(128,0,128),10);
                coords = board.locate(pawn);
                cout<<coords.first<<" "<<coords.second<<endl;
                p.rows = coords.first;
                p.columns = coords.second;
                p.colour = '1';
                if(coords.first < 8 && coords.second < 8)
                    wrappedPawns.push_back(p);
            }
            for(auto const &pawn : p2Pawns) {
                circle(display, pawn, 2, Scalar(0,128,128),10);
                coords = board.locate(pawn);
                cout<<coords.first<<" "<<coords.second<<endl;
                p.rows = coords.first;
                p.columns = coords.second;
                p.colour = '2';
                if(coords.first < 8 && coords.second < 8)
                    wrappedPawns.push_back(p);

            }

            broadcast.update(wrappedPawns);


        } else {
            stringstream ss;
            ss<<"Found "<<boardEdges.size()<<" boardEdges";
            putText(display, ss.str(),Point(100, 100), FONT_HERSHEY_COMPLEX, 1, Scalar(0,0,255), 2);
        }


//        for (auto color : bounds) {
//            Mat tmp;
//
//            auto boardEdges = findItems(hsv, tmp, color, kernel);
//
//            if(!boardEdges.empty()) circle(display, boardEdges[0], 10, Scalar(255,0,0), 3);
//        }

        imshow("Display", display);
        if(waitKey(30) == '\n') break;
    }


//    while(true)
//    {
//        ComputeHsvRanges(p1Channels);
//        inRange(frame, bounds[0], bounds[1], pinkArea);
//        imshow("Detection", pinkArea);
//
//        Mat kernel = getStructuringElement(CV_SHAPE_RECT, Size(5, 05));
//
//        erode(pinkArea, pinkArea, kernel, Point(-1,-1), 3);
//        dilate(pinkArea, pinkArea, kernel, Point(-1,-1), 3);
//
//
//        vector<vector<Point> > contours;
//        vector<Vec4i> hierarchy;
//
//        Mat canny_output;
//        Canny(pinkArea, canny_output, 100, 200);
//
//        findContours(canny_output, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
//
//        vector<int> indices(contours.size());
//        iota(indices.begin(), indices.end(), 0);
//
//        sort(indices.begin(), indices.end(), [&contours](int lhs, int rhs) {
//            return contours[lhs].size() > contours[rhs].size();
//        });
//
//        int N = min(2, (int)contours.size());
//
//        Mat detected = Mat::zeros(pinkArea.size(), CV_8UC3);
//
//        for(int i = 0; i < N; i++) {
//            drawContours(detected, contours, indices[i], Scalar(128,128,128), 2, 8, hierarchy, 0, Point());
//        }
//
//        imshow("Contours", detected);
//
//
//
//
//
//        if(cv::waitKey(30) >= 0) break;
//
//
//    }
    return 0;
}

