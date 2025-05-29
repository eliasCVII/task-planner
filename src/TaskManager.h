#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <vector>
#include <string>
#include <memory>
#include "Act.h"

// Forward declarations
class Config;
class UndoManager;
class UndoableCommand;

class TaskManager {
 private:
  std::vector<Act> tasks;
  int dayLength;
  Config* config;  // Pointer to configuration
  std::unique_ptr<UndoManager> undoManager;  // Undo/redo functionality

 public:
  TaskManager(int dl);
  TaskManager(Config* cfg);  // Constructor with config
  void addTask(const std::string &name, std::string start, int length, bool isRigid);
  void addTask(const std::string &name, int length, bool isRigid);
  void insertTask(int index, const std::string &name, std::string start, int length, bool isRigid);
  void insertTask(int index, const std::string &name, int length, bool isRigid);
  void insertTaskAt(size_t index, Act &newTask);
  void beginAt(size_t index);
  void calcStartTimes();
  void displayAllTasks();
  void calcActLen();
  std::vector<std::string> calcActLen(bool& hasWarnings); // Returns warnings if any
  Act getTask(int index);
  std::vector<Act> getTasks();
  int taskSize();
  void updateTask(int index, const std::string& name, const std::string& startTime, int length, bool isRigid);
  Act& getTaskRef(int index);
  bool deleteTask(int index); // Returns true if deletion was successful
  bool moveTask(int fromIndex, int toIndex); // Move task from one position to another
  bool moveTaskUp(int index); // Move task up by one position (to earlier time)
  bool moveTaskDown(int index); // Move task down by one position (to later time)
  int getDayLength() const; // Get current day length in minutes
  void setDayLength(int minutes); // Set day length in minutes
  double getDayLengthHours() const; // Get day length in hours (for display)

  // Persistence methods
  bool saveToFile(const std::string& filename) const;
  bool loadFromFile(const std::string& filename);
  std::string getDateBasedFilename() const;
  std::string getDateBasedFilename(const std::string& date) const;
  void clearTasks();

  // Config-aware methods
  std::string getConfiguredDataDir() const;
  std::string getConfiguredFilename() const;
  std::string getConfiguredFilename(const std::string& date) const;

  // Undo/Redo functionality
  void executeCommand(std::unique_ptr<UndoableCommand> command);
  bool canUndo() const;
  bool canRedo() const;
  void undo();
  void redo();
  std::string getLastUndoDescription() const;
  std::string getLastRedoDescription() const;
  size_t getUndoStackSize() const;
  size_t getRedoStackSize() const;
};

#endif  // TASKMANAGER_H
