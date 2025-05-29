#include "TaskManager.h"
#include "Config.h"
#include "UndoManager.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

TaskManager::TaskManager(int dl) : dayLength(dl), config(nullptr), undoManager(std::make_unique<UndoManager>()) {}

TaskManager::TaskManager(Config* cfg) : config(cfg), undoManager(std::make_unique<UndoManager>()) {
  if (config) {
    // Get day length from config (convert hours to minutes)
    double hours = config->getDouble("default-day-length", 7.0);
    dayLength = static_cast<int>(hours * 60);
  } else {
    dayLength = 7 * 60; // Default fallback
  }
}

void TaskManager::addTask(const std::string &name, std::string start,
                          int length,
                          bool isRigid) {  // fixed
  Act newTask(name, start, length, isRigid);
  tasks.push_back(newTask);
}

void TaskManager::addTask(const std::string &name, int length,
                          bool isRigid) {  // flexible
  Act newTask(name, length, isRigid);
  tasks.push_back(newTask);
}

void TaskManager::insertTask(int index, const std::string &name, std::string start,
                            int length, bool isRigid) {  // fixed
  Act newTask(name, start, length, isRigid);
  insertTaskAt(index, newTask);
}

void TaskManager::insertTask(int index, const std::string &name, int length,
                            bool isRigid) {  // flexible
  Act newTask(name, length, isRigid);
  insertTaskAt(index, newTask);
}

void TaskManager::insertTaskAt(size_t index, Act &newTask) {
  if (index > tasks.size()) {
    std::cout << "Index out of bounds. Task not added." << std::endl;
    return;
  }
  tasks.insert(tasks.begin() + index, newTask);
}

void TaskManager::beginAt(size_t index) {
  if (index > tasks.size()) {
    std::cout << "Index out of bounds. Cant begin timer." << std::endl;
    return;
  }
  tasks[index].setCurrentTime();
}

void TaskManager::calcStartTimes() {
  if (tasks.empty()) return;

  for (auto i = 0; i < tasks.size(); i++) {
    if (!tasks[i].isFixed()) {
      if (i == 0) {
        // First task starts at default time if not fixed
        tasks[i].setStartTime("09:00");
      } else {
        tasks[i].setStartTime(tasks[i - 1].getStartInt() +
                              tasks[i - 1].getActLength());
      }
    }
  }
}

void TaskManager::displayAllTasks() {
  for (auto &task : tasks) {
    task.displayTask();
  }
}

void TaskManager::calcActLen() {
  bool hasWarnings = false;
  calcActLen(hasWarnings); // Call the enhanced version and ignore warnings
}

std::vector<std::string> TaskManager::calcActLen(bool& hasWarnings) {
  std::vector<std::string> warnings;
  hasWarnings = false;

  for (int i = 0; i < tasks.size() - 1; i++) {
    if (tasks[i + 1].isFixed()) {
      int calculatedActLen = tasks[i + 1].getStartInt() - tasks[i].getStartInt();

      // Check for negative ActLength (time conflict)
      if (calculatedActLen < 0) {
        std::string warning = "Time conflict: Task '" + tasks[i].getName() +
                             "' (starts " + tasks[i].getStartStr() +
                             ") conflicts with '" + tasks[i + 1].getName() +
                             "' (starts " + tasks[i + 1].getStartStr() +
                             "). ActLength set to 0.";
        warnings.push_back(warning);
        hasWarnings = true;

        // Set ActLength to 0 to prevent negative values
        tasks[i].setActLen(0);
      } else {
        tasks[i].setActLen(calculatedActLen);
      }

      tasks[i].setFrozenLen(tasks[i].getActLength());
      tasks[i].toggleFrozen();
    }
  }

  int totalRigid = 0;
  int totalFlexible = 0;

  for (auto &task : tasks) {
    if (task.isRigid()) {
      totalRigid += task.getLength();
    } else if (task.isFrozen()) {
      totalRigid += task.getActLength();
    } else {
      totalFlexible += task.getLength();
    }
  }

  int remainLen = dayLength - totalRigid;
  double ratio = 1.0;
  if (totalFlexible > 0) {
    ratio = static_cast<double>(remainLen) / totalFlexible;
  }

  for (auto &task : tasks) {
    task.setActLen(ratio);
  }

  return warnings;
}

Act TaskManager::getTask(int index) {
  if (index < 0 || index >= tasks.size()) {
    throw std::out_of_range("Index out of range");
  }
  return tasks[index];
}

std::vector<Act> TaskManager::getTasks() { return tasks; }

int TaskManager::taskSize(){
  return tasks.size();
}

void TaskManager::updateTask(int index, const std::string& name, const std::string& startTime, int length, bool isRigid) {
  if (index < 0 || index >= tasks.size()) {
    std::cout << "Index out of bounds. Task not updated." << std::endl;
    return;
  }

  tasks[index].setName(name);
  tasks[index].setLength(length);
  tasks[index].setRigid(isRigid);

  // Handle start time - if empty, make it flexible, otherwise set it and make it fixed
  if (startTime.empty()) {
    tasks[index].setFixed(); // Toggle to make it flexible if it was fixed
    if (tasks[index].isFixed()) {
      tasks[index].setFixed(); // Toggle again to ensure it's flexible
    }
  } else {
    tasks[index].setStartTime(startTime);
    if (!tasks[index].isFixed()) {
      tasks[index].setFixed(); // Toggle to make it fixed if it was flexible
    }
  }
}

Act& TaskManager::getTaskRef(int index) {
  if (index < 0 || index >= tasks.size()) {
    throw std::out_of_range("Index out of range");
  }
  return tasks[index];
}

bool TaskManager::deleteTask(int index) {
  if (index < 0 || index >= tasks.size()) {
    return false; // Invalid index
  }
  tasks.erase(tasks.begin() + index);
  return true;
}

bool TaskManager::moveTask(int fromIndex, int toIndex) {
  if (fromIndex < 0 || fromIndex >= tasks.size() ||
      toIndex < 0 || toIndex >= tasks.size() ||
      fromIndex == toIndex) {
    return false; // Invalid indices or no movement needed
  }

  // Use std::swap for adjacent moves (more reliable)
  if (abs(toIndex - fromIndex) == 1) {
    std::swap(tasks[fromIndex], tasks[toIndex]);
    return true;
  }

  // For non-adjacent moves, use the traditional approach
  // Store the task to move
  Act taskToMove = tasks[fromIndex];

  // Remove the task from its current position
  tasks.erase(tasks.begin() + fromIndex);

  // Adjust toIndex if necessary (since we removed an element)
  if (toIndex > fromIndex) {
    toIndex--;
  }

  // Insert the task at the new position
  tasks.insert(tasks.begin() + toIndex, taskToMove);

  return true;
}

bool TaskManager::moveTaskUp(int index) {
  if (index <= 0 || index >= tasks.size()) {
    return false; // Can't move first task up or invalid index
  }
  return moveTask(index, index - 1);
}

bool TaskManager::moveTaskDown(int index) {
  if (index < 0 || index >= tasks.size() - 1) {
    return false; // Can't move last task down or invalid index
  }
  return moveTask(index, index + 1);
}

int TaskManager::getDayLength() const {
  return dayLength;
}

void TaskManager::setDayLength(int minutes) {
  dayLength = minutes;
}

double TaskManager::getDayLengthHours() const {
  return static_cast<double>(dayLength) / 60.0;
}

// Persistence methods
bool TaskManager::saveToFile(const std::string& filename) const {
  try {
    // Create data directory if it doesn't exist
    std::filesystem::path filepath(filename);
    std::filesystem::create_directories(filepath.parent_path());

    json j;

    // Get current date for metadata
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream date_stream;
    date_stream << std::put_time(&tm, "%Y-%m-%d");

    j["date"] = date_stream.str();
    j["dayLength"] = dayLength;

    // Serialize tasks
    json tasks_array = json::array();
    for (const auto& task : tasks) {
      json task_obj;
      task_obj["name"] = task.getName();
      task_obj["startTime"] = task.getStartStr();
      task_obj["length"] = task.getLength();
      task_obj["rigid"] = task.isRigid();
      task_obj["fixed"] = task.isFixed();
      tasks_array.push_back(task_obj);
    }
    j["tasks"] = tasks_array;

    // Write to file
    std::ofstream file(filename);
    if (!file.is_open()) {
      std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
      return false;
    }

    file << j.dump(2); // Pretty print with 2-space indentation
    file.close();

    return true;
  } catch (const std::exception& e) {
    std::cerr << "Error saving to file " << filename << ": " << e.what() << std::endl;
    return false;
  }
}

bool TaskManager::loadFromFile(const std::string& filename) {
  try {
    // Check if file exists
    if (!std::filesystem::exists(filename)) {
      std::cout << "Data file not found: " << filename << ". Starting with empty task list." << std::endl;
      return false;
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
      std::cerr << "Error: Could not open file for reading: " << filename << std::endl;
      return false;
    }

    json j;
    file >> j;
    file.close();

    // Validate JSON structure
    if (!j.contains("dayLength") || !j.contains("tasks")) {
      std::cerr << "Error: Invalid file format in " << filename << std::endl;
      return false;
    }

    // Load dayLength
    dayLength = j["dayLength"].get<int>();

    // Clear existing tasks
    tasks.clear();

    // Load tasks
    for (const auto& task_obj : j["tasks"]) {
      if (!task_obj.contains("name") || !task_obj.contains("length") ||
          !task_obj.contains("rigid") || !task_obj.contains("fixed")) {
        std::cerr << "Warning: Skipping invalid task in " << filename << std::endl;
        continue;
      }

      std::string name = task_obj["name"].get<std::string>();
      int length = task_obj["length"].get<int>();
      bool rigid = task_obj["rigid"].get<bool>();
      bool fixed = task_obj["fixed"].get<bool>();

      if (fixed && task_obj.contains("startTime")) {
        std::string startTime = task_obj["startTime"].get<std::string>();
        addTask(name, startTime, length, rigid);
      } else {
        addTask(name, length, rigid);
      }
    }

    return true;
  } catch (const std::exception& e) {
    std::cerr << "Error loading from file " << filename << ": " << e.what() << std::endl;
    return false;
  }
}

std::string TaskManager::getDateBasedFilename() const {
  auto now = std::time(nullptr);
  auto tm = *std::localtime(&now);
  std::ostringstream filename;
  filename << "data/tasks_" << std::put_time(&tm, "%Y-%m-%d") << ".json";
  return filename.str();
}

std::string TaskManager::getDateBasedFilename(const std::string& date) const {
  return "data/tasks_" + date + ".json";
}

void TaskManager::clearTasks() {
  tasks.clear();
}

// Config-aware methods
std::string TaskManager::getConfiguredDataDir() const {
  if (config) {
    return config->getString("data-dir", "data");
  }
  return "data";
}

std::string TaskManager::getConfiguredFilename() const {
  auto now = std::time(nullptr);
  auto tm = *std::localtime(&now);
  std::ostringstream filename;

  std::string dataDir = getConfiguredDataDir();
  std::string extension = config ? config->getString("file-extension", ".json") : ".json";

  filename << dataDir << "/tasks_" << std::put_time(&tm, "%Y-%m-%d") << extension;
  return filename.str();
}

std::string TaskManager::getConfiguredFilename(const std::string& date) const {
  std::string dataDir = getConfiguredDataDir();
  std::string extension = config ? config->getString("file-extension", ".json") : ".json";

  return dataDir + "/tasks_" + date + extension;
}

// Undo/Redo functionality implementation
void TaskManager::executeCommand(std::unique_ptr<UndoableCommand> command) {
  if (undoManager) {
    undoManager->executeCommand(std::move(command));
  }
}

bool TaskManager::canUndo() const {
  return undoManager && undoManager->canUndo();
}

bool TaskManager::canRedo() const {
  return undoManager && undoManager->canRedo();
}

void TaskManager::undo() {
  if (undoManager) {
    undoManager->undo();
    // Recalculate task properties after undo
    calcActLen();
    calcStartTimes();
  }
}

void TaskManager::redo() {
  if (undoManager) {
    undoManager->redo();
    // Recalculate task properties after redo
    calcActLen();
    calcStartTimes();
  }
}

std::string TaskManager::getLastUndoDescription() const {
  if (undoManager) {
    return undoManager->getLastUndoDescription();
  }
  return "";
}

std::string TaskManager::getLastRedoDescription() const {
  if (undoManager) {
    return undoManager->getLastRedoDescription();
  }
  return "";
}

size_t TaskManager::getUndoStackSize() const {
  if (undoManager) {
    return undoManager->getUndoStackSize();
  }
  return 0;
}

size_t TaskManager::getRedoStackSize() const {
  if (undoManager) {
    return undoManager->getRedoStackSize();
  }
  return 0;
}