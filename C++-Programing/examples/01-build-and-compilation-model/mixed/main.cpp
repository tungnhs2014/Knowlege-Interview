#include "sensor_c.h"

#include <iostream>

int main() {
    std::cout << "c-sensor=" << sensor_read() << '\n';
}
