# 📅 Console Calendar Manager

A feature-rich, console-based calendar management application built in C. This project demonstrates advanced data structures (linked lists), file I/O operations, and a beautiful command-line interface with color-coded output.

## ✨ Features

### 📝 Event Management
- **Add Events** with date, time, description, priority, and category
- **Edit Events** - Modify description, priority, and category
- **Delete Events** with confirmation prompts
- **All-Day Events** support
- **Unique ID System** for easy event tracking

### 🔍 Advanced Search & Filtering
- **Today's Events** - Quick view of current day's schedule
- **Upcoming Events** - View events for next N days
- **Search by Keyword** - Case-insensitive search
- **Filter by Month** - View all events in a specific month
- **Filter by Category** - View events by type (Work, Personal, Birthday, etc.)
- **Filter by Priority** - View HIGH, MEDIUM, or LOW priority events
- **Date Range Search** - Find events between two dates

### 🎨 Priority System
- **HIGH Priority** (Red) - Urgent events
- **MEDIUM Priority** (Yellow) - Important events
- **LOW Priority** (Green) - Regular events
- Automatic sorting by priority

### 🏷️ Category System
Seven built-in categories:
- 🔵 Work
- 💜 Personal
- 🔷 Birthday
- 🟡 Meeting
- 🟢 Appointment
- 🔴 Reminder
- ⚪ Other

### 📊 Analytics & Export
- **Statistics Dashboard** - View distribution by priority and category
- **CSV Export** - Export calendar to Excel-compatible format
- **Persistent Storage** - Binary file storage for fast loading

### 🎨 Beautiful Interface
- Color-coded priorities and categories
- Clear visual separators
- Intuitive menu navigation
- Professional formatting

## 🚀 Getting Started

### Prerequisites

- GCC Compiler (MinGW for Windows)
- Terminal with ANSI color support (recommended: Windows Terminal, Git Bash, or modern Linux/Mac terminal)

### Installation

1. **Clone the repository**
```bash
   git clone https://github.com/Harshvardhan770/DSA-CP.git
   cd DSA-CP
```

2. **Compile the project**
```bash
   gcc -o calendar.exe calendar_win32.c -mwindows -lcomctl32 -lgdi32
```

3. **Run the application**
   
   **Linux/Mac/Git Bash:**
```bash
   ./calendar
```
   
   **Windows (CMD/PowerShell):**
```cmd
   calendar.exe
```

## 📖 Usage

### Main Menu Options
```
1.  Add New Event          - Create a new calendar event
2.  View All Events        - Display all scheduled events
3.  View Today's Events    - Show today's schedule
4.  View Events by Month   - Filter by specific month
5.  View Events by Category - Filter by event type
6.  View Events by Priority - Filter by priority level
7.  View Upcoming Events   - See events for next N days
8.  Search by Keyword      - Find events by description
9.  Search by Date Range   - Find events between dates
10. Edit Event             - Modify existing event
11. Delete Event           - Remove an event
12. View Statistics        - See calendar analytics
13. Export to CSV          - Export data to CSV file
14. Save & Exit            - Save and close application
```

### Adding an Event

1. Select option `1` from the menu
2. Enter date (validated for leap years and month days)
3. Choose all-day or timed event
4. Select priority level (HIGH/MEDIUM/LOW)
5. Select category (Work/Personal/Birthday/etc.)
6. Enter event description

### Example
```
Enter Day (1-31): 25
Enter Month (1-12): 12
Enter Year: 2025

Is this an all-day event? (1=Yes, 0=No): 0
Enter Hour (24-hour format) (0-23): 14
Enter Minute (0-59): 30

Select Priority:
  1. HIGH
  2. MEDIUM
  3. LOW
Enter priority (1-3): 1

Select Category:
  1. Work
  2. Personal
  3. Birthday
  4. Meeting
  5. Appointment
  6. Reminder
  7. Other
Enter category (1-7): 4

Enter Event Description (max 99 chars): Team Sprint Planning Meeting

[OK] Event added successfully! ID: 1
```

## 🗂️ Project Structure
```
DSA-CP/
├── calendar.h          # Header file with structures and function declarations
├── calendar.c          # Core implementation (linked list operations, file I/O)
├── main.c              # Main program with menu and user interface
├── calendar_data.bin   # Binary data file (auto-generated)
└── README.md           # This file
```

## 🔧 Technical Details

### Data Structures

**Event Structure:**
```c
typedef struct Event {
    int id;                      // Unique identifier
    Date event_date;             // Event date
    Time event_time;             // Event time
    char description[100];       // Event description
    Priority priority;           // HIGH, MEDIUM, LOW
    Category category;           // Event category
    int is_all_day;             // All-day event flag
    struct Event *next;         // Pointer to next event (linked list)
} Event;
```

### Key Algorithms

- **Sorted Insertion**: Events automatically sorted by date, time, and priority
- **Binary File I/O**: Efficient storage and retrieval
- **Leap Year Validation**: Accurate date validation
- **Case-Insensitive Search**: Flexible keyword matching

## 🎨 Terminal Compatibility

### Recommended Terminals

| Platform | Terminal | Colors | Unicode |
|----------|----------|--------|---------|
| Windows | Windows Terminal | ✅ | ✅ |
| Windows | PowerShell 7 | ✅ | ✅ |
| Windows | Git Bash | ✅ | ⚠️ Limited |
| Linux | GNOME Terminal | ✅ | ✅ |
| Linux | Konsole | ✅ | ✅ |
| macOS | Terminal.app | ✅ | ✅ |
| macOS | iTerm2 | ✅ | ✅ |

## 📊 Sample Output
```
=============================================
   WELCOME TO CALENDAR MANAGER PRO 2.0
=============================================

=== TODAY'S EVENTS (23/11/2025) ===

ID: 1 | 14:30 | HIGH | Meeting
Description: Team Sprint Planning Meeting

ID: 2 | All Day | MEDIUM | Birthday
Description: Mom's Birthday

================================
```

## 🛠️ Features Under Development

- [ ] Recurring events (daily, weekly, monthly)
- [ ] Event reminders/notifications
- [ ] Multi-user support
- [ ] Import from .ics files
- [ ] GUI version

## 🤝 Contributing

Contributions are welcome! Feel free to:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 👨‍💻 Author

**Harshvardhan**
- GitHub: [@Harshvardhan770](https://github.com/Harshvardhan770)
