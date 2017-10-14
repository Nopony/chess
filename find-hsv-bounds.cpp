#include <iostream>
#include <opencv2/objdetect.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <fstream>

using namespace std;
using namespace cv;

cv::Mat frame;

bool clicked = false;



//int HMin = 0;
//int HMax = 255;
//int H
void TrackbarCallBack(int trackbarValue, void* args) {
//    auto hsvValPtr = static_cast<float*>(args);
//
//    *hsvValPtr = (float)trackbarValue;
}

void CallBackFunc(int event, int x, int y, int flags, void* channelVectors)
{
    if ( event == EVENT_LBUTTONDOWN ) {
        auto pChannels = static_cast<vector<uchar>*>(channelVectors);
        Vec3b color = frame.at<Vec3b>(y, x);
        cout<<"H: "<<(int)color[0]<<" S: "<<(int)color[1]<<" V: "<<(int)color[2]<<endl;
        for(int i=0; i<3; i++) {
            pChannels[i].push_back(color[i]);
        }
        clicked = true;
    }
}

void ComputeHsvRanges(vector<uchar>* channels, Vec3i* bounds) {

    if(channels[0].empty()) return;

    for(int i=0; i<3; i++) {
        sort(channels[i].begin(), channels[i].end());
    }

    size_t last = channels[0].size() - 1;

    bounds[0] = Vec3i(channels[0][0], channels[1][0], channels[2][0]);
    bounds[1] = Vec3i(channels[0][last], channels[1][last], channels[2][last]);

}

int main(int argc, char** argv)
{
    cout<<"Device number?";
    unsigned short int device_number;
    cin>>device_number;
    cv::VideoCapture cap(device_number);

    if(!cap.isOpened()){
        cout<<"Cannot connect to video";
        return -1;
    }

    auto targetFile = "./bounds.hsv";
    ofstream of;
    of.open(targetFile, fstream::app);
    if(!of.is_open()) {
        cout<<"FAILED TO OPEN FILE";
        return -1;
    }

    Vec3i HSVBounds[2];
    HSVBounds[0] = Vec3i(0,0,0);
    HSVBounds[1] = Vec3i(255,255,255);

    cv::namedWindow("HSV",1);
    cv::namedWindow("Filtered", 1);
    cv::namedWindow("Morphed", 1);
    cv::namedWindow("Trackbars");

    cv::moveWindow("HSV", 0, 500);
    cv::moveWindow("Filtered", 650, 100);
    cv::moveWindow("Morphed", 1250, 100);
    cv::moveWindow("Trackbars", 0, 100);

    bool more = true;

    while(more) {
        cout<<"Range name?";
        string rangeName;
        cin >> rangeName;



        vector<uchar> channels[3];

        setMouseCallback("HSV", CallBackFunc, channels);

        int none = 0;
        int full = 255;

        createTrackbar("HMin", "Trackbars", &HSVBounds[0][0], 255, TrackbarCallBack);
        createTrackbar("HMax", "Trackbars", &HSVBounds[1][0], 255, TrackbarCallBack);
        createTrackbar("SMin", "Trackbars", &HSVBounds[0][1], 255, TrackbarCallBack);
        createTrackbar("SMax", "Trackbars", &HSVBounds[1][1], 255, TrackbarCallBack);
        createTrackbar("VMin", "Trackbars", &HSVBounds[0][2], 255, TrackbarCallBack);
        createTrackbar("VMax", "Trackbars", &HSVBounds[1][2], 255, TrackbarCallBack);

        bool hsvRangesComputed = false;

        while(true) {

            cap >> frame; // get a new frame from camera

            Mat filtered;

            cvtColor(frame, frame, COLOR_BGR2HSV);

            if(clicked)
            {
                ComputeHsvRanges(channels, HSVBounds);
                setTrackbarPos("HMin", "Trackbars", HSVBounds[0][0]);
                setTrackbarPos("HMax", "Trackbars", HSVBounds[1][0]);
                setTrackbarPos("SMin", "Trackbars", HSVBounds[0][1]);
                setTrackbarPos("SMax", "Trackbars", HSVBounds[1][1]);
                setTrackbarPos("VMin", "Trackbars", HSVBounds[0][2]);
                setTrackbarPos("VMax", "Trackbars", HSVBounds[1][2]);
            }

            clicked = false;
            inRange(frame, HSVBounds[0], HSVBounds[1], filtered);

            Mat morphed;

            Mat kernel = getStructuringElement(CV_SHAPE_RECT, Size(5, 5));

            erode(filtered, morphed, kernel, Point(-1, -1), 2);
            dilate(morphed, morphed, kernel, Point(-1, -1), 2);


//            vector<vector<Point> > contours;
//            vector<Vec4i> hierarchy;
//
//            Mat canny_output;
//            Canny(filtered, canny_output, 100, 200);
//
//            findContours(canny_output, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
//
//            vector<int> indices(contours.size());
//            iota(indices.begin(), indices.end(), 0);
//
//            sort(indices.begin(), indices.end(), [&contours](int lhs, int rhs) {
//                return contours[lhs].size() > contours[rhs].size();
//            });
//
//            int N = min(2, (int)contours.size());
//
//            Mat detected = Mat::zeros(filtered.size(), CV_8UC3);
//
//            for(int i = 0; i < N; i++) {
//                drawContours(detected, contours, indices[i], Scalar(128, 128, 128), 2, 8, hierarchy, 0, Point());
//            }
//
//
//            imshow("Contours", detected);

            imshow("HSV", frame);
            imshow("Filtered", filtered);
            imshow("Morphed", morphed);

            if (cv::waitKey(30) == '\n') {
                of<<rangeName<<' ';

                of<<HSVBounds[0]<<' ';
                of<<HSVBounds[1];

                of<<endl;

                cout<<"Another range? [Y/n]";

                char c;
                cin>>c;
                if(c == 'n') {
                    more = false;
                }
                if(c == 'Y') {
                    more = true;
                }
                break;
            };
        }


        return 0;
    }






}

