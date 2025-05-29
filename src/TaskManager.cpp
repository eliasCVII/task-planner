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
#include <algorithm>
#include <unistd.h>
#include <chrono>
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

// File discovery and selection methods
std::vector<std::string> TaskManager::findJsonFiles() const {
  std::vector<std::string> jsonFiles;
  std::string dataDir = getConfiguredDataDir();
  std::string extension = config ? config->getString("file-extension", ".json") : ".json";

  try {
    if (!std::filesystem::exists(dataDir)) {
      return jsonFiles; // Return empty vector if directory doesn't exist
    }

    for (const auto& entry : std::filesystem::directory_iterator(dataDir)) {
      if (entry.is_regular_file()) {
        std::string filename = entry.path().filename().string();
        std::string filepath = entry.path().string();

        // Check if file has the correct extension
        if (filename.size() >= extension.size() &&
            filename.substr(filename.size() - extension.size()) == extension) {

          // Validate that it's a proper task file
          if (isValidTaskFile(filepath)) {
            jsonFiles.push_back(filepath);
          }
        }
      }
    }

    // Sort files by modification time (newest first)
    std::sort(jsonFiles.begin(), jsonFiles.end(), [](const std::string& a, const std::string& b) {
      try {
        auto timeA = std::filesystem::last_write_time(a);
        auto timeB = std::filesystem::last_write_time(b);
        return timeA > timeB;
      } catch (const std::exception&) {
        return false;
      }
    });

  } catch (const std::exception& e) {
    std::cerr << "Error scanning directory " << dataDir << ": " << e.what() << std::endl;
  }

  return jsonFiles;
}

// Helper function to check if a command is available
bool TaskManager::isCommandAvailable(const std::string& command) const {
  std::string checkCmd = "command -v " + command + " >/dev/null 2>&1";
  return system(checkCmd.c_str()) == 0;
}

// Helper function for simple numbered file selection
std::string TaskManager::selectFileSimple() const {
  std::vector<std::string> jsonFiles = findJsonFiles();

  if (jsonFiles.empty()) {
    std::cout << "No JSON files found in data directory." << std::endl;
    return "";
  }

  std::string dataDir = getConfiguredDataDir();

  // Display files with numbers
  std::cout << "\nAvailable task files:" << std::endl;
  std::cout << "=====================" << std::endl;
  for (size_t i = 0; i < jsonFiles.size(); ++i) {
    std::string displayPath = jsonFiles[i];
    // Show relative path if file is in data directory
    if (displayPath.find(dataDir) == 0) {
      displayPath = displayPath.substr(dataDir.length());
      if (!displayPath.empty() && displayPath.front() == '/') {
        displayPath = displayPath.substr(1);
      }
    }

    // Show file modification time
    try {
      auto ftime = std::filesystem::last_write_time(jsonFiles[i]);
      auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
      auto cftime = std::chrono::system_clock::to_time_t(sctp);
      auto tm = *std::localtime(&cftime);

      std::cout << std::setw(2) << (i + 1) << ". " << displayPath
                << " (modified: " << std::put_time(&tm, "%Y-%m-%d %H:%M") << ")" << std::endl;
    } catch (const std::exception&) {
      std::cout << std::setw(2) << (i + 1) << ". " << displayPath << std::endl;
    }
  }

  std::cout << " 0. Cancel" << std::endl;
  std::cout << "\nEnter your choice (0-" << jsonFiles.size() << "): ";

  std::string input;
  std::getline(std::cin, input);

  try {
    int choice = std::stoi(input);
    if (choice == 0) {
      return ""; // User cancelled
    }
    if (choice >= 1 && choice <= static_cast<int>(jsonFiles.size())) {
      return jsonFiles[choice - 1];
    }
  } catch (const std::exception&) {
    // Invalid input
  }

  std::cout << "Invalid selection." << std::endl;
  return "";
}

std::string TaskManager::selectFileWithFzf() const {
  std::vector<std::string> jsonFiles = findJsonFiles();

  if (jsonFiles.empty()) {
    return ""; // No files found
  }

  // Check for available file selection tools in order of preference
  if (isCommandAvailable("fzf")) {
    return selectFileWithFzfTool();
  } else if (isCommandAvailable("fd")) {
    return selectFileWithFdTool();
  } else if (isCommandAvailable("find")) {
    return selectFileWithFindTool();
  } else {
    // Fallback to simple numbered selection
    std::cerr << "Note: fzf, fd, or find not found. Using simple selection interface." << std::endl;
    return selectFileSimple();
  }
}

// Implementation for fzf-based file selection
std::string TaskManager::selectFileWithFzfTool() const {
  std::vector<std::string> jsonFiles = findJsonFiles();

  // Create a temporary file with the list of files
  std::string tempFile = "/tmp/plan_files_" + std::to_string(getpid()) + ".txt";
  std::ofstream temp(tempFile);
  if (!temp.is_open()) {
    std::cerr << "Error: Could not create temporary file for fzf" << std::endl;
    return "";
  }

  // Write file paths to temp file, showing relative paths for better display
  std::string dataDir = getConfiguredDataDir();
  for (const auto& file : jsonFiles) {
    std::string displayPath = file;
    // Show relative path if file is in data directory
    if (file.find(dataDir) == 0) {
      displayPath = file.substr(dataDir.length());
      if (!displayPath.empty() && displayPath.front() == '/') {
        displayPath = displayPath.substr(1);
      }
    }
    temp << displayPath << " (" << file << ")" << std::endl;
  }
  temp.close();

  // Run fzf to select a file
  std::string fzfCommand = "fzf --height=40% --reverse --prompt='Select task file: ' --preview='head -20 {}' < " + tempFile;

  FILE* pipe = popen(fzfCommand.c_str(), "r");
  if (!pipe) {
    std::cerr << "Error: Could not run fzf." << std::endl;
    std::filesystem::remove(tempFile);
    return "";
  }

  char buffer[1024];
  std::string result = "";
  if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    result = buffer;
    // Remove trailing newline
    if (!result.empty() && result.back() == '\n') {
      result.pop_back();
    }
  }

  int exitCode = pclose(pipe);
  std::filesystem::remove(tempFile);

  if (exitCode != 0 || result.empty()) {
    return ""; // User cancelled or fzf failed
  }

  // Extract the actual file path from the result (it's in parentheses)
  size_t start = result.find('(');
  size_t end = result.find(')', start);
  if (start != std::string::npos && end != std::string::npos) {
    return result.substr(start + 1, end - start - 1);
  }

  return "";
}

// Implementation for fd-based file selection
std::string TaskManager::selectFileWithFdTool() const {
  std::string dataDir = getConfiguredDataDir();
  std::string extension = config ? config->getString("file-extension", ".json") : ".json";

  // Use fd to find JSON files and pipe to fzf if available, otherwise use simple selection
  std::string fdCommand = "fd -e json . " + dataDir;

  if (isCommandAvailable("fzf")) {
    fdCommand += " | fzf --height=40% --reverse --prompt='Select task file: ' --preview='head -20 {}'";
  } else {
    // Use fd with simple numbered selection
    FILE* pipe = popen(fdCommand.c_str(), "r");
    if (!pipe) {
      std::cerr << "Error: Could not run fd command." << std::endl;
      return "";
    }

    std::vector<std::string> files;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      std::string file = buffer;
      if (!file.empty() && file.back() == '\n') {
        file.pop_back();
      }
      if (isValidTaskFile(file)) {
        files.push_back(file);
      }
    }
    pclose(pipe);

    if (files.empty()) {
      std::cout << "No valid JSON task files found." << std::endl;
      return "";
    }

    // Display files with numbers
    std::cout << "\nAvailable task files (found with fd):" << std::endl;
    std::cout << "=====================================" << std::endl;
    for (size_t i = 0; i < files.size(); ++i) {
      std::string displayPath = files[i];
      if (displayPath.find(dataDir) == 0) {
        displayPath = displayPath.substr(dataDir.length());
        if (!displayPath.empty() && displayPath.front() == '/') {
          displayPath = displayPath.substr(1);
        }
      }
      std::cout << std::setw(2) << (i + 1) << ". " << displayPath << std::endl;
    }
    std::cout << " 0. Cancel" << std::endl;
    std::cout << "\nEnter your choice (0-" << files.size() << "): ";

    std::string input;
    std::getline(std::cin, input);

    try {
      int choice = std::stoi(input);
      if (choice == 0) {
        return "";
      }
      if (choice >= 1 && choice <= static_cast<int>(files.size())) {
        return files[choice - 1];
      }
    } catch (const std::exception&) {
      // Invalid input
    }

    std::cout << "Invalid selection." << std::endl;
    return "";
  }

  // If fzf is available, run the combined command
  FILE* pipe = popen(fdCommand.c_str(), "r");
  if (!pipe) {
    std::cerr << "Error: Could not run fd + fzf command." << std::endl;
    return "";
  }

  char buffer[1024];
  std::string result = "";
  if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    result = buffer;
    if (!result.empty() && result.back() == '\n') {
      result.pop_back();
    }
  }

  int exitCode = pclose(pipe);
  if (exitCode != 0 || result.empty()) {
    return "";
  }

  return result;
}

// Implementation for find-based file selection
std::string TaskManager::selectFileWithFindTool() const {
  std::string dataDir = getConfiguredDataDir();
  std::string extension = config ? config->getString("file-extension", ".json") : ".json";

  // Use find to locate JSON files
  std::string findCommand = "find " + dataDir + " -name '*" + extension + "' -type f";

  FILE* pipe = popen(findCommand.c_str(), "r");
  if (!pipe) {
    std::cerr << "Error: Could not run find command." << std::endl;
    return "";
  }

  std::vector<std::string> files;
  char buffer[1024];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    std::string file = buffer;
    if (!file.empty() && file.back() == '\n') {
      file.pop_back();
    }
    if (isValidTaskFile(file)) {
      files.push_back(file);
    }
  }
  pclose(pipe);

  if (files.empty()) {
    std::cout << "No valid JSON task files found." << std::endl;
    return "";
  }

  // Sort files by modification time (newest first)
  std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
    try {
      auto timeA = std::filesystem::last_write_time(a);
      auto timeB = std::filesystem::last_write_time(b);
      return timeA > timeB;
    } catch (const std::exception&) {
      return false;
    }
  });

  // Display files with numbers
  std::cout << "\nAvailable task files (found with find):" << std::endl;
  std::cout << "=======================================" << std::endl;
  for (size_t i = 0; i < files.size(); ++i) {
    std::string displayPath = files[i];
    if (displayPath.find(dataDir) == 0) {
      displayPath = displayPath.substr(dataDir.length());
      if (!displayPath.empty() && displayPath.front() == '/') {
        displayPath = displayPath.substr(1);
      }
    }

    // Show file modification time
    try {
      auto ftime = std::filesystem::last_write_time(files[i]);
      auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
      auto cftime = std::chrono::system_clock::to_time_t(sctp);
      auto tm = *std::localtime(&cftime);

      std::cout << std::setw(2) << (i + 1) << ". " << displayPath
                << " (modified: " << std::put_time(&tm, "%Y-%m-%d %H:%M") << ")" << std::endl;
    } catch (const std::exception&) {
      std::cout << std::setw(2) << (i + 1) << ". " << displayPath << std::endl;
    }
  }
  std::cout << " 0. Cancel" << std::endl;
  std::cout << "\nEnter your choice (0-" << files.size() << "): ";

  std::string input;
  std::getline(std::cin, input);

  try {
    int choice = std::stoi(input);
    if (choice == 0) {
      return "";
    }
    if (choice >= 1 && choice <= static_cast<int>(files.size())) {
      return files[choice - 1];
    }
  } catch (const std::exception&) {
    // Invalid input
  }

  std::cout << "Invalid selection." << std::endl;
  return "";
}

bool TaskManager::isValidTaskFile(const std::string& filename) const {
  try {
    if (!std::filesystem::exists(filename)) {
      return false;
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
      return false;
    }

    json j;
    file >> j;
    file.close();

    // Check for required fields that make it a valid task file
    return j.contains("dayLength") && j.contains("tasks") && j["tasks"].is_array();

  } catch (const std::exception&) {
    return false;
  }
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