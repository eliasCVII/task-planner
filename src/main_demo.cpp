#include "TaskManager.h"
#include <iostream>

int main() {
  const int day = 7;
  const int dayLength = day * 60;
  TaskManager manager(dayLength);

  manager.addTask("A", "10:00", 60, false);
  manager.addTask("B", "13:30", 10, false);
  manager.addTask("C", 60, false);
  manager.addTask("D", 15, false);
  manager.calcActLen();
  manager.calcStartTimes();
  manager.displayAllTasks();

  std::cout << "\n";
  //manager.getTask(0).displayTask();

  return 0;
}
