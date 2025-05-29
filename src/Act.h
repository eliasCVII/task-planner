#ifndef TASK_H
#define TASK_H

#include <string>

class Act {
 private:
  std::string name;

  int startInt;
  std::string startStr;

  int length;
  int actLength;
  int frozenLength;

  bool rigid;
  bool fixed;
  bool frozen;

 public:
  Act(const std::string &name, std::string timeStr, int length, bool isRigid);
  Act(const std::string &name, int length, bool isRigid);
  int timeStringToMinutes(const std::string &timeStr);
  std::string minutesToTime(int totalMinutes);
  void setStartTime(std::string timeStr);
  void setStartTime(int Minutes);
  void setCurrentTime();
  void setActLen(double ratio);
  void setActLen(int fixedLen);
  void setFrozenLen(int frozenLen);
  void setRigid();
  void setRigid(bool isRigid);
  void setFixed();
  void toggleFrozen();
  void setName(const std::string& newName);
  void setLength(int newLength);
  int getLength() const;
  int getActLength() const;
  int getStartInt() const;
  std::string getStartStr() const;
  bool isRigid() const;
  bool isFixed() const;
  bool isFrozen() const;
  std::string getName() const;
  void displayTask();
};

#endif  // TASK_H
