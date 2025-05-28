#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <filesystem>

#include "TaskManager.h"
#include "Config.h"

using namespace ftxui;

// Helper function to get current time in minutes since midnight
int getCurrentTimeInMinutes() {
  auto now = std::time(nullptr);
  auto tm = *std::localtime(&now);
  return tm.tm_hour * 60 + tm.tm_min;
}

// Helper function to convert minutes to HH:MM format
std::string minutesToTimeString(int minutes) {
  int hours = minutes / 60;
  int mins = minutes % 60;
  std::ostringstream oss;
  oss << std::setfill('0') << std::setw(2) << hours << ":"
      << std::setfill('0') << std::setw(2) << mins;
  return oss.str();
}

// Helper function to find current task
std::string getCurrentTask(TaskManager& manager) {
  int currentTime = getCurrentTimeInMinutes();
  auto tasks = manager.getTasks();

  for (size_t i = 0; i < tasks.size(); i++) {
    int taskStart = tasks[i].getStartInt();
    int taskEnd = taskStart + tasks[i].getActLength();

    if (currentTime >= taskStart && currentTime < taskEnd) {
      int remainingMinutes = taskEnd - currentTime;
      return tasks[i].getName() + " (ends at " + minutesToTimeString(taskEnd) +
             ", " + std::to_string(remainingMinutes) + " min remaining)";
    }
  }

  return "No active task at current time (" + minutesToTimeString(currentTime) + ")";
}

// Helper function to find next task
std::string getNextTask(TaskManager& manager) {
  int currentTime = getCurrentTimeInMinutes();
  auto tasks = manager.getTasks();

  // Find the next task that starts after current time
  for (size_t i = 0; i < tasks.size(); i++) {
    int taskStart = tasks[i].getStartInt();

    if (taskStart > currentTime) {
      int minutesUntil = taskStart - currentTime;
      return tasks[i].getName() + " (starts at " + minutesToTimeString(taskStart) +
             ", in " + std::to_string(minutesUntil) + " minutes)";
    }
  }

  return "No upcoming tasks today";
}

// Helper function to print usage information
void printUsage(const char* programName) {
  std::cout << "Usage: " << programName << " [command] [date]\n";
  std::cout << "       " << programName << " [date]           - Interactive mode for specific date\n";
  std::cout << "\nCommands:\n";
  std::cout << "  now    - Show current active task\n";
  std::cout << "  next   - Show next upcoming task\n";
  std::cout << "  list   - Show all tasks for today\n";
  std::cout << "  (no args) - Launch interactive task manager (loads last session)\n";
  std::cout << "\nDate parameter (YYYY-MM-DD format):\n";
  std::cout << "  " << programName << " 2024-01-15         - Interactive mode for specific date\n";
  std::cout << "  " << programName << " now 2024-01-15     - Show current task for specific date\n";
  std::cout << "  " << programName << " list 2024-01-15    - List tasks for specific date\n";
  std::cout << "\nSession Management:\n";
  std::cout << "  Interactive mode remembers the last opened file\n";
  std::cout << "  Use date parameter to override and work on specific dates\n";
  std::cout << "\nConfiguration:\n";
  std::cout << "  Settings are loaded from plan.conf (see CONFIG.md for details)\n";
  std::cout << "  Default data directory: data/ (configurable via data-dir setting)\n";
  std::cout << "  Default day length: 7.0 hours (configurable via default-day-length setting)\n";
  std::cout << "\nData files format: tasks_YYYY-MM-DD.json\n";
}

// Helper function to list all tasks
void listAllTasks(TaskManager& manager) {
  auto tasks = manager.getTasks();
  std::cout << "Today's Tasks:\n";
  std::cout << "=============\n";

  for (size_t i = 0; i < tasks.size(); i++) {
    std::string startTime = minutesToTimeString(tasks[i].getStartInt());
    std::string endTime = minutesToTimeString(tasks[i].getStartInt() + tasks[i].getActLength());
    std::string status = tasks[i].isFixed() ? "[FIXED]" : "[FLEX]";

    std::cout << (i + 1) << ". " << tasks[i].getName()
              << " " << status
              << " (" << startTime << " - " << endTime
              << ", " << tasks[i].getActLength() << " min)\n";
  }
}

// Helper function to convert hours (decimal) to minutes
int hoursToMinutes(double hours) {
  return static_cast<int>(hours * 60);
}

// Helper function to validate hours input
bool isValidHours(const std::string& str) {
  if (str.empty()) return false;
  try {
    double hours = std::stod(str);
    return hours > 0 && hours <= 24; // Reasonable range
  } catch (...) {
    return false;
  }
}

// Helper function to trim whitespace
std::string trim(const std::string& str) {
  size_t start = str.find_first_not_of(" \t\n\r");
  if (start == std::string::npos) return "";
  size_t end = str.find_last_not_of(" \t\n\r");
  return str.substr(start, end - start + 1);
}

// Helper function to validate time format (HH:MM)
bool isValidTimeFormat(const std::string& time) {
  std::string trimmed = trim(time);
  if (trimmed.empty()) return true; // Empty is valid for flexible tasks
  std::regex time_regex("^([0-1]?[0-9]|2[0-3]):[0-5][0-9]$");
  return std::regex_match(trimmed, time_regex);
}

// Helper function to validate numeric input
bool isValidNumber(const std::string& str) {
  std::string trimmed = trim(str);
  if (trimmed.empty()) return false;
  for (char c : trimmed) {
    if (!std::isdigit(c)) return false;
  }
  return true;
}

// Helper function to get current attribute value as string
std::string getCurrentAttributeValue(const std::vector<Act>& tasks, int task_idx, int col_idx) {
  if (task_idx < 0 || task_idx >= tasks.size()) return "";

  const auto& task = tasks[task_idx];
  switch (col_idx) {
    case 0: return task.isFixed() ? "Yes" : "No";
    case 1: return task.isRigid() ? "Yes" : "No";
    case 2: return task.getName();
    case 3: return task.getStartStr();
    case 4: return std::to_string(task.getLength());
    case 5: return std::to_string(task.getActLength());
    default: return "";
  }
}

// Helper function to check if column is editable
bool isColumnEditable(int col_idx) {
  // ActLength (5) is calculated, not directly editable
  // Fixed (0), Rigid (1), Name (2), Start (3), Length (4) are all editable
  return col_idx >= 0 && col_idx <= 4;
}

// Helper function to apply edit to task
bool applyEdit(TaskManager& manager, int task_idx, int col_idx, const std::string& value, std::string& error_msg) {
  if (task_idx < 0 || task_idx >= manager.taskSize()) {
    error_msg = "Invalid task index";
    return false;
  }

  auto& task = manager.getTaskRef(task_idx);
  std::string trimmed_value = trim(value);

  try {
    switch (col_idx) {
      case 0: // Fixed (fixed-time)
        if (trimmed_value == "Yes" || trimmed_value == "yes" || trimmed_value == "Y" || trimmed_value == "y" || trimmed_value == "1" || trimmed_value == "true") {
          if (!task.isFixed()) {
            task.setFixed(); // Toggle to make it fixed
          }
        } else if (trimmed_value == "No" || trimmed_value == "no" || trimmed_value == "N" || trimmed_value == "n" || trimmed_value == "0" || trimmed_value == "false") {
          if (task.isFixed()) {
            task.setFixed(); // Toggle to make it flexible
          }
        } else {
          error_msg = "Invalid fixed value. Use Yes/No, Y/N, 1/0, or true/false";
          return false;
        }
        break;

      case 1: // Rigid
        if (trimmed_value == "Yes" || trimmed_value == "yes" || trimmed_value == "Y" || trimmed_value == "y" || trimmed_value == "1" || trimmed_value == "true") {
          task.setRigid(true);
        } else if (trimmed_value == "No" || trimmed_value == "no" || trimmed_value == "N" || trimmed_value == "n" || trimmed_value == "0" || trimmed_value == "false") {
          task.setRigid(false);
        } else {
          error_msg = "Invalid rigid value. Use Yes/No, Y/N, 1/0, or true/false";
          return false;
        }
        break;

      case 2: // Name
        if (trimmed_value.empty()) {
          error_msg = "Task name cannot be empty";
          return false;
        }
        task.setName(trimmed_value);
        break;

      case 3: // Start Time
        if (!trimmed_value.empty() && !isValidTimeFormat(trimmed_value)) {
          error_msg = "Invalid time format. Use HH:MM";
          return false;
        }
        if (trimmed_value.empty()) {
          // Make task flexible
          if (task.isFixed()) {
            task.setFixed();
          }
        } else {
          // Set start time and make task fixed
          task.setStartTime(trimmed_value);
          if (!task.isFixed()) {
            task.setFixed();
          }
        }
        break;

      case 4: // Length
        if (!isValidNumber(trimmed_value) || std::stoi(trimmed_value) <= 0) {
          error_msg = "Length must be a positive number";
          return false;
        }
        task.setLength(std::stoi(trimmed_value));
        break;

      default:
        error_msg = "Column not editable";
        return false;
    }

    // Recalculate task properties after any edit
    bool hasWarnings = false;
    auto warnings = manager.calcActLen(hasWarnings);
    manager.calcStartTimes();

    // Display warnings if any
    if (hasWarnings && !warnings.empty()) {
      error_msg = warnings[0]; // Show first warning
      return false;
    }

    return true;

  } catch (const std::exception& e) {
    error_msg = "Error applying edit: " + std::string(e.what());
    return false;
  }
}

int main(int argc, char* argv[]) {
  // Load configuration
  Config config("plan.conf");
  config.loadFromFile();

  // Initialize TaskManager with config
  TaskManager manager(&config);

  // Determine which file to load based on priority:
  // 1. Command line date parameter (if provided)
  // 2. Last opened file (for interactive mode with no args)
  // 3. Today's date file
  std::string targetDate = "";
  std::string dataFilename;
  bool useLastOpened = false;
  bool isInteractiveWithDate = false;

  // Check for date parameter in different positions
  if (argc == 2) {
    // Could be: ./proj 2024-01-15 (interactive with date) or ./proj command
    std::string arg = argv[1];
    if (arg.length() == 10 && arg[4] == '-' && arg[7] == '-') {
      // It's a date for interactive mode
      targetDate = arg;
      dataFilename = manager.getConfiguredFilename(targetDate);
      isInteractiveWithDate = true;
    }
    // If not a date, it will be handled as a command below
  } else if (argc > 2) {
    // Could be: ./proj command 2024-01-15
    targetDate = argv[argc - 1];
    // Simple date validation (YYYY-MM-DD format)
    if (targetDate.length() == 10 && targetDate[4] == '-' && targetDate[7] == '-') {
      dataFilename = manager.getConfiguredFilename(targetDate);
    } else {
      targetDate = "";
    }
  }

  // If no command line date and this is interactive mode (no args), try last opened file
  if (targetDate.empty() && argc == 1) {
    std::string lastFile = config.getLastOpenedFile();
    if (!lastFile.empty() && std::filesystem::exists(lastFile)) {
      dataFilename = lastFile;
      useLastOpened = true;
    }
  }

  // If no valid date or last file, use today's date
  if (targetDate.empty() && !useLastOpened) {
    dataFilename = manager.getConfiguredFilename();
  }

  // Try to load data from file
  bool dataLoaded = manager.loadFromFile(dataFilename);

  // Show which file was loaded (for CLI commands and when status messages are enabled)
  if (argc > 1 || config.getBool("status-messages", true)) {
    if (dataLoaded) {
      if (useLastOpened) {
        std::cout << "Loaded last session: " << dataFilename << std::endl;
      } else if (!targetDate.empty()) {
        if (isInteractiveWithDate) {
          std::cout << "Interactive mode for " << targetDate << ": " << dataFilename << std::endl;
        } else {
          std::cout << "Loaded data for " << targetDate << ": " << dataFilename << std::endl;
        }
      }
    }
  }

  // If no data file exists, create default tasks for demonstration
  if (!dataLoaded) {
    manager.addTask("A", "10:00", 60, false);
    manager.addTask("B", "13:30", 10, false);
    manager.addTask("C", 60, false);
    manager.addTask("D", 15, false);
  }

  // Calculate initial task properties
  manager.calcActLen();
  manager.calcStartTimes();

  // Handle command-line arguments
  if (argc > 1 && !isInteractiveWithDate) {
    std::string command = argv[1];

    if (command == "now") {
      std::cout << getCurrentTask(manager) << std::endl;
      return 0;
    } else if (command == "next") {
      std::cout << getNextTask(manager) << std::endl;
      return 0;
    } else if (command == "list") {
      listAllTasks(manager);
      return 0;
    } else if (command == "help" || command == "--help" || command == "-h") {
      printUsage(argv[0]);
      return 0;
    } else {
      std::cerr << "Unknown command: " << command << std::endl;
      printUsage(argv[0]);
      return 1;
    }
  }

  // If no arguments, launch interactive mode

  // Navigation state
  int selected_task = -1; // -1 for dayLength row, 0+ for actual tasks
  int selected_column = 0; // 0=Fixed, 1=Rigid, 2=Name, 3=Start, 4=Length, 5=ActLength
  const int NUM_COLUMNS = 6;
  const std::vector<std::string> COLUMN_NAMES = {"Fixed", "Rigid", "Name", "Start", "Length", "ActLength"};

  // Editing state
  bool edit_mode = false;
  std::string edit_buffer = "";

  // Visual mode state
  bool visual_mode = false;
  int visual_selected_task = -1;

  // Deletion state for 'dd' command
  bool first_d_pressed = false;

  // Status messages
  std::string status_message = "";
  bool show_success = false;

  // Create dayLength table renderer
  auto dayLength_renderer = Renderer([&] {
    // Create dayLength table data
    std::vector<std::vector<std::string>> dayLength_data;

    std::ostringstream dayLengthStr;
    dayLengthStr << std::fixed << std::setprecision(1) << manager.getDayLengthHours() << " hours";

    std::string hours_value = dayLengthStr.str();

    // If we're editing the dayLength cell, show the edit buffer instead
    if (edit_mode && selected_task == -1) {
      hours_value = edit_buffer + " hours";
    }

    dayLength_data.push_back({"Day Length", hours_value});

    // Create dayLength table
    auto dayLength_table = Table(dayLength_data);
    dayLength_table.SelectAll().Border(LIGHT);
    dayLength_table.SelectColumn(0).DecorateCells(bold);

    // Highlight if dayLength is selected
    if (selected_task == -1) {
      if (edit_mode) {
        // In edit mode - bright highlight
        dayLength_table.SelectCell(1, 0).Decorate(bgcolor(Color::Red));
        dayLength_table.SelectCell(1, 0).DecorateCells(color(Color::White) | bold);
      } else {
        // Navigation mode - subtle highlight
        dayLength_table.SelectCell(1, 0).Decorate(bgcolor(Color::Cyan));
        dayLength_table.SelectCell(1, 0).DecorateCells(color(Color::Black) | bold);
      }
    }

    return dayLength_table.Render();
  });

  // Create tasks table renderer
  auto tasks_table_renderer = Renderer([&] {
    // Create table data
    std::vector<std::vector<std::string>> table_data;

    // Add header row
    table_data.push_back({"Fixed", "Rigid", "Name", "Start", "Length", "ActLength"});

    // Add task rows
    auto tasks = manager.getTasks();
    for (int i = 0; i < tasks.size(); i++) {
      const auto& task = tasks[i];
      std::vector<std::string> row = {
        task.isFixed() ? "Yes" : "No",
        task.isRigid() ? "Yes" : "No",
        task.getName(),
        task.getStartStr(),
        std::to_string(task.getLength()),
        std::to_string(task.getActLength())
      };

      // If we're editing this cell, show the edit buffer instead
      if (edit_mode && i == selected_task && selected_column < row.size()) {
        row[selected_column] = edit_buffer;
      }

      table_data.push_back(row);
    }

    // Create table
    auto table = Table(table_data);
    table.SelectAll().Border(LIGHT);
    table.SelectRow(0).Decorate(bold);
    table.SelectRow(0).SeparatorVertical(LIGHT);
    table.SelectColumn(2).DecorateCells(color(Color::Yellow));
    table.SelectColumn(3).DecorateCells(color(Color::Green));

    // Highlight selected row (add 1 to account for header)
    if (selected_task >= 0 && selected_task < tasks.size()) {
      table.SelectRow(selected_task + 1).Decorate(bgcolor(Color::Blue));
      table.SelectRow(selected_task + 1).DecorateCells(color(Color::White));
    }

    // Highlight selected column
    if (selected_task >= 0 && selected_column >= 0 && selected_column < NUM_COLUMNS) {
      table.SelectColumn(selected_column).Decorate(bgcolor(Color::DarkBlue));
    }

    // Special highlighting for the currently selected cell and visual selection
    if (selected_task >= 0 && selected_task < tasks.size() &&
        selected_column >= 0 && selected_column < NUM_COLUMNS) {
      if (edit_mode) {
        // In edit mode - bright highlight
        table.SelectCell(selected_column, selected_task + 1).Decorate(bgcolor(Color::Red));
        table.SelectCell(selected_column, selected_task + 1).DecorateCells(color(Color::White) | bold);
      } else if (visual_mode && visual_selected_task >= 0 && visual_selected_task < tasks.size()) {
        // Visual mode - highlight the selected task row
        for (int col = 0; col < NUM_COLUMNS; col++) {
          table.SelectCell(col, visual_selected_task + 1).Decorate(bgcolor(Color::Yellow));
          table.SelectCell(col, visual_selected_task + 1).DecorateCells(color(Color::Black) | bold);
        }
        // Also show current cursor position with different color
        table.SelectCell(selected_column, selected_task + 1).Decorate(bgcolor(Color::Cyan));
        table.SelectCell(selected_column, selected_task + 1).DecorateCells(color(Color::Black) | bold);
      } else {
        // Navigation mode - subtle highlight
        table.SelectCell(selected_column, selected_task + 1).Decorate(bgcolor(Color::Cyan));
        table.SelectCell(selected_column, selected_task + 1).DecorateCells(color(Color::Black) | bold);
      }
    }

    return table.Render();
  });

  // Create combined renderer
  auto table_renderer = Renderer([&] {
    auto tasks = manager.getTasks();

    // Create status line
    std::string mode_indicator = edit_mode ? "[EDIT]" : (visual_mode ? "[VISUAL]" : "[NAV]");
    std::string current_cell = "";
    if (selected_task == -1) {
      // DayLength table
      current_cell = "Day Length - Available working hours (editable)";
    } else if (selected_task >= 0 && selected_task < tasks.size() &&
               selected_column >= 0 && selected_column < COLUMN_NAMES.size()) {
      current_cell = "Task " + std::to_string(selected_task + 1) + " - " + COLUMN_NAMES[selected_column];
      if (!isColumnEditable(selected_column)) {
        current_cell += " (read-only)";
      } else if (selected_column == 0) {
        current_cell += " (fixed-time: start time won't change)";
      } else if (selected_column == 1) {
        current_cell += " (rigid: fixed length)";
      }
    }

    return vbox({
      text("Interactive Task Manager") | bold | hcenter,
      separator(),
      dayLength_renderer->Render(),
      separator(),
      tasks_table_renderer->Render() | flex,
      separator(),
      hbox({
        text(mode_indicator) | (edit_mode ? color(Color::Red) : color(Color::Green)) | bold,
        text(" | "),
        text(current_cell) | dim,
      }),
      text("hjkl: Navigate | Enter: Edit/Toggle | Tab: Next field | v: Visual | Esc: Exit | i/o: Insert | dd/D: Delete | q: Quit") | dim | hcenter,
    }) | border;
  });

  // Main renderer is just the table
  auto main_renderer = table_renderer;

  auto screen = ScreenInteractive::TerminalOutput();

  // Add vim-like navigation and command handling
  main_renderer |= CatchEvent([&](Event event) {
    // Handle quit
    if (event == Event::Character('q')) {
      if (edit_mode) {
        // If in edit mode, close it instead of quitting
        edit_mode = false;
        edit_buffer = "";
        status_message = "";
        show_success = false;
        return true;
      } else {
        screen.ExitLoopClosure()();
        return true;
      }
    }

    // Handle escape - exit edit mode or visual mode
    if (event == Event::Escape) {
      if (edit_mode) {
        edit_mode = false;
        edit_buffer = "";
        status_message = "";
        show_success = false;
        return true;
      } else if (visual_mode) {
        visual_mode = false;
        visual_selected_task = -1;
        status_message = "";
        show_success = false;
        return true;
      }
      return false;
    }

    // Handle edit mode events
    if (edit_mode) {
      if (event == Event::Return) {
        // Apply the edit
        std::string error_msg;
        bool edit_success = false;

        if (selected_task == -1) {
          // Handle dayLength editing
          if (isValidHours(edit_buffer)) {
            double hours = std::stod(edit_buffer);
            int minutes = hoursToMinutes(hours);
            manager.setDayLength(minutes);

            // Recalculate task properties after dayLength change
            bool hasWarnings = false;
            auto warnings = manager.calcActLen(hasWarnings);
            manager.calcStartTimes();

            if (hasWarnings && !warnings.empty()) {
              error_msg = warnings[0];
              edit_success = false;
            } else {
              edit_success = true;
            }
          } else {
            error_msg = "Invalid hours value. Enter a positive number (e.g., 7.5)";
            edit_success = false;
          }
        } else {
          edit_success = applyEdit(manager, selected_task, selected_column, edit_buffer, error_msg);
        }

        if (edit_success) {
          // Sequential field navigation: Name -> Start -> Length -> exit
          if (selected_column == 2) { // Name field
            selected_column = 3; // Move to Start field
            edit_buffer = "";
            status_message = "Editing " + COLUMN_NAMES[selected_column] + " - Press Enter for next field, Esc to cancel";
            show_success = false;
            // Stay in edit mode
          } else if (selected_column == 3) { // Start field
            selected_column = 4; // Move to Length field
            edit_buffer = "";
            status_message = "Editing " + COLUMN_NAMES[selected_column] + " - Press Enter to finish, Esc to cancel";
            show_success = false;
            // Stay in edit mode
          } else {
            // For Length field (4) and boolean fields (0,1), exit edit mode
            status_message = "Edit applied successfully";
            show_success = true;
            edit_mode = false;
            edit_buffer = "";
          }
        } else {
          status_message = error_msg;
          show_success = false;
        }
        return true;
      } else if (event == Event::Tab) {
        // Apply current edit first
        std::string error_msg;
        if (applyEdit(manager, selected_task, selected_column, edit_buffer, error_msg)) {
          // Tab cycles through editable columns of the current task
          int original_column = selected_column;
          do {
            selected_column = (selected_column + 1) % NUM_COLUMNS;
          } while (!isColumnEditable(selected_column) && selected_column != original_column);

          // Start editing the new column
          edit_buffer = "";
          status_message = "Editing " + COLUMN_NAMES[selected_column] + " - Press Enter to apply, Tab for next field, Esc to cancel";
          show_success = false;
        } else {
          status_message = error_msg;
          show_success = false;
        }
        return true;
      } else if (event == Event::Backspace) {
        if (!edit_buffer.empty()) {
          edit_buffer.pop_back();
        }
        return true;
      } else if (event.is_character()) {
        edit_buffer += event.character();
        return true;
      }
      return false;
    }

    // Only handle navigation and commands when not in edit mode
    if (!edit_mode) {
      // Handle visual mode task movement
      if (visual_mode) {
        if (event == Event::Character('j') || event == Event::ArrowDown) {
          // Move selected task down
          if (visual_selected_task < manager.taskSize() - 1) {
            // Check if moving a fixed task up (to earlier position) - make it flexible
            auto& task = manager.getTaskRef(visual_selected_task);
            if (task.isFixed() && visual_selected_task < selected_task) {
              task.setFixed(); // Remove fixed status
              status_message = "Fixed task made flexible for movement";
              show_success = false;
            }

            if (manager.moveTask(visual_selected_task, visual_selected_task + 1)) {
              visual_selected_task++;
              selected_task = visual_selected_task; // Keep cursor on moved task

              // Recalculate task properties after movement
              bool hasWarnings = false;
              auto warnings = manager.calcActLen(hasWarnings);
              manager.calcStartTimes();

              // Display warnings if any
              if (hasWarnings && !warnings.empty()) {
                status_message = "Warning: " + warnings[0];
                show_success = false;
              } else if (status_message.empty()) {
                status_message = "Task moved down";
                show_success = true;
              }
            }
          }
          return true;
        }

        if (event == Event::Character('k') || event == Event::ArrowUp) {
          // Move selected task up
          if (visual_selected_task > 0) {
            // Check if moving a fixed task up (to earlier position) - make it flexible
            auto& task = manager.getTaskRef(visual_selected_task);
            if (task.isFixed()) {
              task.setFixed(); // Remove fixed status
              status_message = "Fixed task made flexible for movement";
              show_success = false;
            }

            if (manager.moveTask(visual_selected_task, visual_selected_task - 1)) {
              visual_selected_task--;
              selected_task = visual_selected_task; // Keep cursor on moved task

              // Recalculate task properties after movement
              bool hasWarnings = false;
              auto warnings = manager.calcActLen(hasWarnings);
              manager.calcStartTimes();

              // Display warnings if any
              if (hasWarnings && !warnings.empty()) {
                status_message = "Warning: " + warnings[0];
                show_success = false;
              } else if (status_message.empty()) {
                status_message = "Task moved up";
                show_success = true;
              }
            }
          }
          return true;
        }

        // Handle Enter in visual mode - complete action and exit visual mode
        if (event == Event::Return) {
          visual_mode = false;
          visual_selected_task = -1;
          status_message = "Visual mode completed";
          show_success = true;
          return true;
        }
      } else {
        // Normal navigation mode
        if (event == Event::Character('j') || event == Event::ArrowDown) {
          if (selected_task == -1) {
            // From dayLength row to first task
            if (manager.taskSize() > 0) {
              selected_task = 0;
            }
          } else if (selected_task < manager.taskSize() - 1) {
            selected_task++;
          }
          return true;
        }

        if (event == Event::Character('k') || event == Event::ArrowUp) {
          if (selected_task == 0) {
            // From first task to dayLength row
            selected_task = -1;
          } else if (selected_task > 0) {
            selected_task--;
          }
          return true;
        }
      }

      if (event == Event::Character('h') || event == Event::ArrowLeft) {
        if (selected_column > 0) {
          selected_column--;
        }
        return true;
      }

      if (event == Event::Character('l') || event == Event::ArrowRight) {
        if (selected_column < NUM_COLUMNS - 1) {
          selected_column++;
        }
        return true;
      }

      // Handle enter to start editing or toggle boolean values (not in visual mode)
      if (event == Event::Return && !visual_mode) {
        if (selected_task == -1) {
          // Editing dayLength
          edit_mode = true;
          edit_buffer = "";
          status_message = "Editing Day Length - Enter hours (e.g., 7.5), Press Enter to apply, Esc to cancel";
          show_success = false;
          return true;
        } else if (isColumnEditable(selected_column) && selected_task >= 0 && selected_task < manager.taskSize()) {
          // For boolean columns (Fixed and Rigid), toggle directly
          if (selected_column == 0 || selected_column == 1) {
            auto& task = manager.getTaskRef(selected_task);
            if (selected_column == 0) { // Fixed (fixed-time)
              task.setFixed(); // Toggle fixed state
              status_message = "Fixed-time toggled to " + std::string(task.isFixed() ? "Yes" : "No");
            } else if (selected_column == 1) { // Rigid
              task.setRigid(!task.isRigid()); // Toggle rigid state
              status_message = "Rigid toggled to " + std::string(task.isRigid() ? "Yes" : "No");
            }
            show_success = true;
            // Recalculate task properties after toggle
            bool hasWarnings = false;
            auto warnings = manager.calcActLen(hasWarnings);
            manager.calcStartTimes();

            // Display warnings if any
            if (hasWarnings && !warnings.empty()) {
              status_message = "Warning: " + warnings[0];
              show_success = false;
            }
          } else {
            // For other columns, enter edit mode
            edit_mode = true;
            edit_buffer = ""; // Start with empty field for fresh input
            status_message = "Editing " + COLUMN_NAMES[selected_column] + " - Press Enter to apply, Esc to cancel";
            show_success = false;
          }
        } else {
          status_message = "This column is not editable";
          show_success = false;
        }
        return true;
      }

      // Handle vim-like insertion commands - inline insertion (not in visual mode)
      if (event == Event::Character('i') && !visual_mode) {
        // Insert before current task
        int insert_position = selected_task;
        manager.insertTask(insert_position, "", 0, false); // Empty task with default values

        // Recalculate task properties
        bool hasWarnings = false;
        auto warnings = manager.calcActLen(hasWarnings);
        manager.calcStartTimes();

        // Position cursor on the new task's Name field and enter edit mode
        selected_task = insert_position;
        selected_column = 2; // Name column
        edit_mode = true;
        edit_buffer = "";

        // Show warnings if any, otherwise show normal message
        if (hasWarnings && !warnings.empty()) {
          status_message = "Warning: " + warnings[0];
          show_success = false;
        } else {
          status_message = "New task inserted - Editing Name - Press Enter to apply, Tab for next field, Esc to cancel";
          show_success = false;
        }
        return true;
      }

      if (event == Event::Character('o') && !visual_mode) {
        // Insert after current task
        int insert_position = selected_task + 1;
        manager.insertTask(insert_position, "", 0, false); // Empty task with default values

        // Recalculate task properties
        bool hasWarnings = false;
        auto warnings = manager.calcActLen(hasWarnings);
        manager.calcStartTimes();

        // Position cursor on the new task's Name field and enter edit mode
        selected_task = insert_position;
        selected_column = 2; // Name column
        edit_mode = true;
        edit_buffer = "";

        // Show warnings if any, otherwise show normal message
        if (hasWarnings && !warnings.empty()) {
          status_message = "Warning: " + warnings[0];
          show_success = false;
        } else {
          status_message = "New task inserted - Editing Name - Press Enter to apply, Tab for next field, Esc to cancel";
          show_success = false;
        }
        return true;
      }

      // Handle visual mode entry
      if (event == Event::Character('v')) {
        if (!visual_mode) {
          // Enter visual mode
          visual_mode = true;
          visual_selected_task = selected_task;
          status_message = "Visual mode - Use j/k to move task, Enter/Esc to exit";
          show_success = false;
        }
        return true;
      }

      // Handle vim-like deletion commands (not in visual mode)
      if (event == Event::Character('d') && !visual_mode) {
        if (first_d_pressed) {
          // Second 'd' pressed - execute deletion
          if (manager.taskSize() > 0 && selected_task >= 0 && selected_task < manager.taskSize()) {
            std::string deleted_task_name = manager.getTask(selected_task).getName();
            if (manager.deleteTask(selected_task)) {
              // Recalculate task properties after deletion
              bool hasWarnings = false;
              auto warnings = manager.calcActLen(hasWarnings);
              manager.calcStartTimes();

              // Adjust cursor position
              if (selected_task >= manager.taskSize()) {
                selected_task = manager.taskSize() - 1; // Move to last task if we deleted the last one
              }
              if (selected_task < 0) {
                selected_task = 0; // Ensure we don't go negative
              }

              status_message = "Task '" + deleted_task_name + "' deleted successfully";
              show_success = true;
            } else {
              status_message = "Failed to delete task";
              show_success = false;
            }
          } else {
            status_message = "No task to delete";
            show_success = false;
          }
          first_d_pressed = false;
        } else {
          // First 'd' pressed
          first_d_pressed = true;
          status_message = "Press 'd' again to delete task";
          show_success = false;
        }
        return true;
      }

      if (event == Event::Character('D') && !visual_mode) {
        // Capital D - immediate deletion
        if (manager.taskSize() > 0 && selected_task >= 0 && selected_task < manager.taskSize()) {
          std::string deleted_task_name = manager.getTask(selected_task).getName();
          if (manager.deleteTask(selected_task)) {
            // Recalculate task properties after deletion
            bool hasWarnings = false;
            auto warnings = manager.calcActLen(hasWarnings);
            manager.calcStartTimes();

            // Adjust cursor position
            if (selected_task >= manager.taskSize()) {
              selected_task = manager.taskSize() - 1; // Move to last task if we deleted the last one
            }
            if (selected_task < 0) {
              selected_task = 0; // Ensure we don't go negative
            }

            status_message = "Task '" + deleted_task_name + "' deleted successfully";
            show_success = true;
          } else {
            status_message = "Failed to delete task";
            show_success = false;
          }
        } else {
          status_message = "No task to delete";
          show_success = false;
        }
        first_d_pressed = false; // Reset dd state
        return true;
      }

      // Reset first_d_pressed if any other key is pressed
      if (first_d_pressed && event != Event::Character('d')) {
        first_d_pressed = false;
        status_message = "";
        show_success = false;
      }
    }

    return false;
  });

  screen.Loop(main_renderer);

  // Auto-save data when exiting interactive mode (if enabled in config)
  if (config.getBool("auto-save", true)) {
    if (!manager.saveToFile(dataFilename)) {
      std::cerr << "Warning: Failed to save data to " << dataFilename << std::endl;
    } else if (config.getBool("status-messages", true)) {
      std::cout << "Data saved to " << dataFilename << std::endl;
    }

    // Remember this file as the last opened for next session
    config.setLastOpenedFile(dataFilename);
    config.saveSessionState();
  }

  return 0;
}
