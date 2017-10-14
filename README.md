## Chess


### Summary
An app for processing live images of a game of ~~chess~~ **draughts** and streaming the positions to a remote server using color-based object detection.


### Dependencies
The program is based on the wonderful OpenCV library for C++, which provides crucial CV algorithms such as erosion, dillatation, Canny or filtering.
A C++ CURL with a thin make-do wrapper is used for server communication (as socketio for c++ proved difficult to implement).
In the absence of a real webcam, the app works perfectly with DroidCam.
Naturally, to see any results, the chess_client git repo (which contains the server and the browser-based client with the output) has to be installed.







