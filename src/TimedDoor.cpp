// Copyright 2021 GHA Test Team
#include "TimedDoor.h"

#include <chrono>
#include <stdexcept>
#include <thread>

namespace {

class AlarmDispatch : public TimerClient {
 public:
  explicit AlarmDispatch(TimedDoor& timedDoor) : door(timedDoor) {}

  void Timeout() override { door.throwState(); }

 private:
  TimedDoor& door;
};

}  // namespace

DoorTimerAdapter::DoorTimerAdapter(TimedDoor& timedDoor) : door(timedDoor) {}

void DoorTimerAdapter::Timeout() {
  Timer alarmClock;
  AlarmDispatch dispatch(door);
  alarmClock.tregister(door.getTimeOut(), &dispatch);
}

TimedDoor::TimedDoor(int timeoutSeconds)
    : adapter(new DoorTimerAdapter(*this)),
      iTimeout(timeoutSeconds),
      isOpened(false) {}

bool TimedDoor::isDoorOpened() { return isOpened; }

void TimedDoor::unlock() {
  isOpened = true;
  adapter->Timeout();
}

void TimedDoor::lock() { isOpened = false; }

int TimedDoor::getTimeOut() const { return iTimeout; }

void TimedDoor::throwState() {
  if (isOpened) {
    throw std::runtime_error("Door has been open for too long");
  }
}

void Timer::sleep(int delaySeconds) {
  std::this_thread::sleep_for(std::chrono::seconds(delaySeconds));
}

void Timer::tregister(int delaySeconds, TimerClient* listener) {
  client = listener;
  sleep(delaySeconds);
  if (client != nullptr) {
    client->Timeout();
  }
}
