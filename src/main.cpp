// Copyright 2021 GHA Test Team
#include "TimedDoor.h"

#include <iostream>
#include <stdexcept>

int main() {
  TimedDoor tDoor(5);
  tDoor.lock();

  try {
    tDoor.unlock();
  } catch (const std::runtime_error& error) {
    std::cerr << "Alarm: " << error.what() << std::endl;
  }

  return 0;
}
