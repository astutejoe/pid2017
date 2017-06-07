all:
	g++ main.cpp `pkg-config --cflags --libs gtkmm-3.0` -lopencv_imgproc -lopencv_imgcodecs -lopencv_core -o main
