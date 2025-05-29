#include "Act.h"

#include <ctime>
#include <iostream>

Act::Act(const std::string &name, std::string timeStr, int length,
           bool isRigid)
    : name(name),
      startStr(timeStr),
      length(length),
      actLength(0),
      frozenLength(0),
      rigid(isRigid),
      fixed(true),
      frozen(false) {
  setStartTime(timeStr);
}
Act::Act(const std::string &name, int length, bool isRigid)
    : name(name),
      startStr("09:00"),
      length(length),
      actLength(0),
      frozenLength(0),
      rigid(isRigid),
      fixed(false),
      frozen(false) {
  setStartTime("09:00");  // Initialize startInt properly
}
int Act::timeStringToMinutes(const std::string &timeStr) {
    size_t colonPos = timeStr.find(':');
    if (colonPos == std::string::npos || colonPos == 0 ||
        colonPos == timeStr.length() - 1) {
      throw std::invalid_argument("Invalid time format. Expected 'HH:MM'.");
    }

    int hours = std::stoi(timeStr.substr(0, colonPos));
    int minutes = std::stoi(timeStr.substr(colonPos + 1));

    if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59) {
      throw std::invalid_argument(
          "Invalid time. Hours must be 0-23 and minutes 0-59.");
    }

    return hours * 60 + minutes;
  }
std::string Act::minutesToTime(int totalMinutes) {
    totalMinutes %= 1440;
    int hours = totalMinutes / 60;
    int minutes = totalMinutes % 60;
    return (hours < 10 ? "0" : "") + std::to_string(hours) + ":" +
           (minutes < 10 ? "0" : "") + std::to_string(minutes);
  }
void Act::setStartTime(std::string timeStr) {
    startInt = timeStringToMinutes(timeStr);
    startStr = timeStr;
  }
void Act::setStartTime(int Minutes) {
    startInt = Minutes;
    startStr = minutesToTime(Minutes);
  }
void Act::setCurrentTime() {
    std::time_t now = std::time(nullptr);
    std::tm *localTime = std::localtime(&now);

    int hours = localTime->tm_hour;
    int minutes = localTime->tm_min;

    int totalMinutes = hours * 60 + minutes;

    startInt = totalMinutes;
    startStr = minutesToTime(totalMinutes);
    fixed = true;
  }
  void Act::setActLen(double ratio) {
    if (rigid) {
      actLength = length;
    } else if (frozen) {
      actLength = frozenLength;
    } else {
      actLength = static_cast<int>((length * ratio));
    }
  }
  void Act::setActLen(int fixedLen) { actLength = fixedLen; }
  void Act::setFrozenLen(int frozenLen) { frozenLength = frozenLen; }
  void Act::setRigid() { rigid = !rigid; }
  void Act::setRigid(bool isRigid) { rigid = isRigid; }
  void Act::setFixed() { fixed = !fixed; }
  void Act::toggleFrozen() { frozen = !frozen; }
  void Act::setName(const std::string& newName) { name = newName; }
  void Act::setLength(int newLength) { length = newLength; }
  int Act::getLength() const { return length; }
  int Act::getActLength() const { return static_cast<int>(actLength); }
  int Act::getStartInt() const { return startInt; }
  std::string Act::getStartStr() const {return startStr;}
  bool Act::isRigid() const { return rigid; }
  bool Act::isFixed() const { return fixed; }
  bool Act::isFrozen() const { return frozen; }
  std::string Act::getName() const { return name; }
  void Act::displayTask() {
    std::cout << "Task: " << name << ", Start Time: " << startStr
              << ", Length: " << length << " minutes"
              << ", ActLen: " << actLength << " minutes" << std::endl;
  }
