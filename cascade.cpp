#include <iostream>
#include <opencv2/objdetect.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>

using namespace std;
using namespace cv;

void DetectAndDraw(cv::Mat &frame);

CascadeClassifier pawnCascade;

int main(int argc, char** argv)
{
    cv::VideoCapture cap(1);
    if(!cap.isOpened()){
        cout<<"Cannot connect to video";
        return -1;
    }

    if( !pawnCascade.load("/home/nopony/projects-dev/chess/data/cascade.xml")) {
        cout<<"Cannot load cascade";
        return -1;
    }


    cv::Mat frame;
    cv::namedWindow("Detection",1);
    while(true)
    {
        cap >> frame; // get a new frame from camera

        DetectAndDraw(frame);

        imshow("Detection", frame);
        if(cv::waitKey(30) >= 0) break;
    }
    return 0;
}

void DetectAndDraw(cv::Mat &frame) {
    std::vector<Rect> pawns;
    Mat frame_gray;
    cvtColor( frame, frame_gray, COLOR_BGR2GRAY );
    equalizeHist( frame_gray, frame_gray );
    pawnCascade.detectMultiScale(frame_gray, pawns, 1.1, 2, 0|CASCADE_SCALE_IMAGE, Size(30, 30));
    Scalar pink = Scalar(242,109,191);
    for (const auto &pawn : pawns) {
        rectangle(frame, pawn, pink);
    }

}
