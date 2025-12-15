# 📅 Calendar Manager Pro – Enhanced Edition

A feature-rich, native **Windows Calendar Application** built in **pure C using the Win32 API**.  
This enhanced edition introduces modern event management features such as **search**, **filtering**, **color-coding**, and **robust data protection**, all in a lightweight desktop app.

![Version](https://img.shields.io/badge/version-3.0-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)
![Language](https://img.shields.io/badge/language-C-orange)
![License](https://img.shields.io/badge/license-MIT-green)

---

## 🌟 Enhanced Features

### 🚀 Core Functionality
- ✅ **Advanced Event Management** – Add, **Edit**, and Delete events seamlessly
- ✅ **Smart Search** – Real-time text search across event descriptions
- ✅ **Dynamic Filtering** – Filter events by **Category** or **Priority**
- ✅ **Color-Coded View** – Events are visually distinct based on priority and category
- ✅ **Data Safety** – Automatic saving with a dedicated **Backup** system

### 📅 Event Properties
- ⏰ **Date & Time** – Support for specific time ranges and All-Day events
- 📍 **Location** – Track where your events are happening
- ⚡ **Priorities** – Critical, High, Medium, Low
- 🏷️ **Categories** – Work, Personal, Birthday, Meeting, Appointment, Reminder, Holiday, Other
- 🔔 **Reminders** – Set custom reminder times (minutes before event)

### 🖥️ User Interface
- **Modern Font Rendering** – Uses Segoe UI for a cleaner look
- **Interactive Calendar** – Click dates to filter the specific day's schedule
- **Detailed List View** – Sortable columns with visual priority indicators
- **Statistics Dashboard** – Comprehensive breakdown of schedule data
- **Debug Console** – Built-in debug tools for developer troubleshooting

---

## 🚀 Getting Started

### Prerequisites
**Windows Operating System** with a C compiler:
- **MinGW-w64** (Recommended)
- **MSYS2**
- **TDM-GCC**

### Installation

1. **Clone or Download the Repository**
```bash
git clone https://github.com/Harshvardhan770/DSA-CP
cd DSA-CP
````

2. **Compile the Application**

```bash
gcc -o calendar_win32.exe calendar_win32.c -mwindows -lcomctl32 -lgdi32
```

3. **Run the Application**

```bash
.\calendar_win32.exe
```

---

## 📖 Usage Guide

### ➕ Adding & Editing Events

1. **Add** – Click **➕ Add Event** or press `Ctrl + N`
2. **Edit** – Select an event and click **✏️ Edit Event** or press `Ctrl + E`
3. Fill in the following details:

   * **Description** (Required)
   * **Location**
   * **Time** – Toggle *All Day* or set Start/End times
   * **Priority & Category**
   * **Reminder** – Enable and set reminder minutes
4. Click **Save / Update**

### 🔍 Searching & Filtering

* **Text Search** – Type in the search box (real-time results)
* **Category Filter** – Filter by Work, Personal, etc.
* **Priority Filter** – Show Critical or High priority events
* **Date Filter** – Click a date on the calendar
* **Reset Filters** – Click **📋 View All** or press `F5`

### 💾 Data Management

* **Export CSV** – Generate spreadsheet-compatible files
* **Backup** – Create timestamped backups
  Example:

  ```
  calendar_backup_20251216_120000.dat
  ```

---

## ⌨️ Keyboard Shortcuts

| Shortcut   | Action             |
| ---------- | ------------------ |
| `Ctrl + N` | New Event          |
| `Ctrl + E` | Edit Event         |
| `Ctrl + F` | Focus Search       |
| `Delete`   | Delete Event       |
| `F5`       | Refresh / View All |

---

## 🗂️ Data Structure

### Binary Storage (`calendar.dat`)

The application uses a custom binary format for speed and efficiency.
**Note:** Older versions of `calendar.dat` containing recurrence data are not compatible with v3.0.

### CSV Export Format

```csv
ID,Date,Time,Description,Location,Priority,Category,Reminder
1,15/12/2025,09:00-10:00,"Team Meeting","Office",High,Meeting,15 min
2,25/12/2025,All Day,"Christmas","Home",Low,Holiday,0 min
```

---

## 📊 Visual Indicators

### Priority Colors

* 🔴 **Critical** – Light Red
* 🟠 **High** – Light Orange
* 🟡 **Medium** – Light Yellow
* 🟢 **Low** – Light Green

### Category Colors

* 💼 **Work** – Light Blue
* 👤 **Personal** – Light Green
* 🎂 **Birthday** – Light Pink
* 📅 **Other** – Light Grey

---

## 🔧 Troubleshooting

### Events Not Showing?

1. Click **📋 View All** to clear date filters
2. Ensure the search box is empty
3. Open the **🐛 Debug Console** to verify event data

### “Invalid Time” Error

* End time must be **after** start time
* Hours must be `0–23`, minutes `0–59`

### Compilation Error: `undefined reference`

Ensure required libraries are linked:

```bash
-mwindows -lcomctl32 -lgdi32
```

---

## 🔮 Future Roadmap

* [ ] Restore Recurrence (Daily / Weekly / Monthly)
* [ ] Drag-and-drop support
* [ ] Dark Mode
* [ ] System Tray integration for reminders

---

## 📝 License

This project is licensed under the **MIT License**.

```
MIT License  
Copyright (c) 2025 Calendar Manager Pro Enhanced
```

```
```
