#ifndef MMA8653_H_
#define MMA8653_H_

class MMA8653 {
public:
    void setup();
    bool getXYZ(int& X, int& Y, int& Z);
};

#endif
