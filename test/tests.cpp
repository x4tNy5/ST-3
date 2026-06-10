// Copyright 2021 GHA Test Team

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>

#include "TimedDoor.h"

namespace {

class MockDoorInterface : public Door {
 public:
  MOCK_METHOD(void, lock, (), (override));
  MOCK_METHOD(void, unlock, (), (override));
  MOCK_METHOD(bool, isDoorOpened, (), (override));
};

class MockTimerListener : public TimerClient {
 public:
  MOCK_METHOD(void, Timeout, (), (override));
};

class TimedDoorTestHarness : public ::testing::Test {
 protected:
  void SetUp() override {
    doorUnderTest = std::make_unique<TimedDoor>(instantDelay);
  }

  void TearDown() override { doorUnderTest.reset(); }

  static constexpr int instantDelay = 0;
  std::unique_ptr<TimedDoor> doorUnderTest;
};

class TimerRegistrationHarness : public ::testing::Test {
 protected:
  void SetUp() override { listener = std::make_unique<MockTimerListener>(); }

  void TearDown() override { listener.reset(); }

  std::unique_ptr<MockTimerListener> listener;
};

void OpenDoorIgnoringAlarm(TimedDoor& door) {
  try {
    door.unlock();
  } catch (const std::runtime_error&) {
  }
}

}  // namespace

TEST_F(TimedDoorTestHarness, NewDoorIsClosedByDefault) {
  EXPECT_FALSE(doorUnderTest->isDoorOpened());
}

TEST_F(TimedDoorTestHarness, LockKeepsDoorClosed) {
  doorUnderTest->lock();
  EXPECT_FALSE(doorUnderTest->isDoorOpened());
}

TEST_F(TimedDoorTestHarness, UnlockLeavesDoorOpenEvenAfterAlarm) {
  OpenDoorIgnoringAlarm(*doorUnderTest);
  EXPECT_TRUE(doorUnderTest->isDoorOpened());
}

TEST_F(TimedDoorTestHarness, RemembersConfiguredTimeoutValue) {
  TimedDoor delayedDoor(7);
  EXPECT_EQ(delayedDoor.getTimeOut(), 7);
}

TEST_F(TimedDoorTestHarness, ClosingAfterUnlockPreventsStateException) {
  OpenDoorIgnoringAlarm(*doorUnderTest);
  doorUnderTest->lock();
  EXPECT_NO_THROW(doorUnderTest->throwState());
}

TEST_F(TimedDoorTestHarness, OpenDoorTriggersRuntimeErrorOnStateCheck) {
  OpenDoorIgnoringAlarm(*doorUnderTest);
  EXPECT_THROW(doorUnderTest->throwState(), std::runtime_error);
}

TEST_F(TimedDoorTestHarness, ClosedDoorDoesNotRaiseOnStateCheck) {
  doorUnderTest->lock();
  EXPECT_NO_THROW(doorUnderTest->throwState());
}

TEST(DoorTimerAdapterSuite, ForwardsTimeoutToDoorStateValidation) {
  TimedDoor sampleDoor(0);
  DoorTimerAdapter bridge(sampleDoor);

  OpenDoorIgnoringAlarm(sampleDoor);
  EXPECT_THROW(bridge.Timeout(), std::runtime_error);

  sampleDoor.lock();
  EXPECT_NO_THROW(bridge.Timeout());
}

TEST_F(TimerRegistrationHarness, NotifiesListenerAfterDelayElapses) {
  EXPECT_CALL(*listener, Timeout()).Times(1);

  Timer countdown;
  countdown.tregister(0, listener.get());
}

TEST_F(TimerRegistrationHarness, SkipsCallbackWhenListenerMissing) {
  Timer countdown;
  EXPECT_NO_THROW(countdown.tregister(0, nullptr));
}

TEST(MockDoorInterfaceSuite, LockAndUnlockFollowExpectedSequence) {
  MockDoorInterface doorMock;

  {
    ::testing::InSequence callOrder;
    EXPECT_CALL(doorMock, unlock()).Times(1);
    EXPECT_CALL(doorMock, isDoorOpened()).WillOnce(::testing::Return(true));
    EXPECT_CALL(doorMock, lock()).Times(1);
    EXPECT_CALL(doorMock, isDoorOpened()).WillOnce(::testing::Return(false));
  }

  doorMock.unlock();
  EXPECT_TRUE(doorMock.isDoorOpened());
  doorMock.lock();
  EXPECT_FALSE(doorMock.isDoorOpened());
}

TEST(MockTimerListenerSuite, TimeoutInvocationIsObservable) {
  MockTimerListener listenerMock;

  EXPECT_CALL(listenerMock, Timeout()).Times(1);
  listenerMock.Timeout();
}

TEST(TimedDoorAlarmSuite, RaisesAfterTimeoutWhenDoorStaysOpen) {
  TimedDoor delayedDoor(1);
  std::atomic<bool> alarmRaised{false};

  std::thread waitingThread([&]() {
    try {
      delayedDoor.unlock();
    } catch (const std::runtime_error&) {
      alarmRaised = true;
    }
  });

  waitingThread.join();
  EXPECT_TRUE(alarmRaised.load());
  EXPECT_TRUE(delayedDoor.isDoorOpened());
}

TEST(TimedDoorAlarmSuite, SkipsAlarmWhenDoorClosedBeforeTimeout) {
  TimedDoor delayedDoor(1);
  std::atomic<bool> alarmRaised{false};

  std::thread waitingThread([&]() {
    try {
      delayedDoor.unlock();
    } catch (const std::runtime_error&) {
      alarmRaised = true;
    }
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  delayedDoor.lock();
  waitingThread.join();

  EXPECT_FALSE(alarmRaised.load());
}
