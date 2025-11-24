# 📅 Calendar Manager Pro

A feature-rich, native Windows calendar application built in pure C using Win32 API. Manage your events efficiently with an intuitive GUI, persistent storage, and advanced features like recurring events, priorities, and reminders.

![Version](https://img.shields.io/badge/version-3.0-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)
![Language](https://img.shields.io/badge/language-C-orange)
![License](https://img.shields.io/badge/license-MIT-green)

---

## 🌟 Features

### Core Functionality
- ✅ **Event Management**: Add, edit, delete events with ease
- ✅ **Interactive Calendar**: Click dates to filter and view events
- ✅ **Persistent Storage**: All data saved automatically in binary format
- ✅ **Export to CSV**: Export your events to spreadsheet format
- ✅ **Statistics Dashboard**: View event breakdown by priority and category

### Event Properties
- 📅 **Date & Time**: Support for both timed and all-day events
- 📍 **Location**: Add location information to events
- ⚡ **Priorities**: Critical, High, Medium, Low
- 🏷️ **Categories**: Work, Personal, Birthday, Meeting, Appointment, Reminder, Holiday, Other
- 🔄 **Recurrence**: Daily, Weekly, Monthly, Yearly recurring events
- ⏰ **Reminders**: Set custom reminder times (minutes before event)

### User Interface
- 🎨 **Native Windows GUI**: Clean, professional interface using Win32 API
- 📋 **Sortable List View**: View events in a detailed table format
- 🗓️ **Month Calendar Widget**: Visual date selection
- 📊 **Status Bar**: Real-time feedback on operations
- 💬 **Confirmation Dialogs**: Safe operations with user confirmation

---

## 🚀 Getting Started

### Prerequisites

**Windows Operating System** with one of the following:
- **MinGW-w64** (Recommended)
- **MSYS2** with GCC
- **TDM-GCC**
- **Cygwin** with GCC

### Installation

1. **Clone or Download** the repository:
```bash
git clone https://github.com/Harshvardhan770/DSA-CP
cd DSA-CP
```

2. **Compile** the application:
```bash
gcc -o calendar.exe calendar_win32.c -mwindows -lcomctl32 -lgdi32
```

3. **Run** the application:
```bash
.\calendar.exe
```

Or simply **double-click** `calendar.exe` after compilation.

---

## 📖 Usage Guide

### Adding an Event

1. Click the **"➕ Add Event"** button
2. Fill in the event details:
   - **Description** (required): Brief description of the event
   - **Location** (optional): Where the event takes place
   - **All Day Event**: Check if event is all-day
   - **Start Time**: Event start time (HH:MM format)
   - **End Time**: Event end time (HH:MM format)
   - **Priority**: Select priority level
   - **Category**: Choose event category
   - **Recurrence**: Set if event repeats
   - **Reminder**: Optionally set reminder minutes before event
3. Click **"Save Event"**

### Viewing Events

- **View All Events**: Click "📋 View All" to see all events
- **View Today's Events**: Click "📅 View Today" to see today's schedule
- **Filter by Date**: Click any date on the calendar to view events for that day
- **Scroll List**: Use the list view to browse through events

### Deleting Events

1. **Select** an event from the list
2. Click **"🗑️ Delete Event"**
3. **Confirm** deletion in the dialog

### Exporting Data

1. Click **"💾 Export CSV"**
2. Choose save location and filename
3. Open the CSV file in Excel, Google Sheets, or any spreadsheet application

### Viewing Statistics

1. Click **"📊 Statistics"**
2. View breakdown of:
   - Total events
   - All-day vs timed events
   - Recurring events
   - Events with reminders
   - Priority distribution
   - Category distribution

---

## 📁 File Structure

```
DSA-CP/
├── calendar_win32.c        # Main application source code
├── calendar.exe            # Compiled executable (after build)
├── calendar.dat            # Binary data file (auto-created)
├── README.md               # This file
└── LICENSE                 # License file
```

---

## 🗂️ Data Storage

### Binary Format (`calendar.dat`)

The application stores events in a binary file format:

```
[Magic Number: 0xCAFEBABE]
[Next ID Counter]
[Event Count]
[Event 1 Data]
[Event 2 Data]
...
```

**Features:**
- ✅ Automatic saving after every operation
- ✅ Compact binary format for efficiency
- ✅ Data integrity validation with magic number
- ✅ Persistent across application restarts

### CSV Export Format

Exported CSV files contain:
```csv
ID,Date,Time,Description,Location,Priority,Category,Recurrence
1,15/11/2024,09:00-10:00,"Team Meeting","Conference Room",High,Meeting,None
2,20/11/2024,All Day,"Holiday","",Low,Holiday,None
```

---

## 🎯 Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Tab` | Navigate between fields |
| `Enter` | Save event (in Add Event dialog) |
| `Escape` | Cancel (in Add Event dialog) |
| `Click Date` | Filter events by date |

---

## 🛠️ Technical Details

### Technologies Used
- **Language**: Pure C (C99 standard)
- **GUI Framework**: Win32 API
- **Controls**: Common Controls Library (comctl32)
- **Graphics**: GDI (gdi32)

### System Requirements
- **OS**: Windows 7 or later
- **RAM**: 50 MB minimum
- **Disk Space**: 5 MB
- **Display**: 1024x768 or higher recommended

### Architecture
```
┌─────────────────────────────────────┐
│         Main Window                 │
│  ┌──────────┐  ┌─────────────────┐ │
│  │ Calendar │  │   Event List    │ │
│  │  Widget  │  │   (ListView)    │ │
│  └──────────┘  └─────────────────┘ │
│  ┌────────────────────────────────┐│
│  │    Action Buttons              ││
│  └────────────────────────────────┘│
│  ┌────────────────────────────────┐│
│  │      Status Bar                ││
│  └────────────────────────────────┘│
└─────────────────────────────────────┘
```

---

## 🔧 Compilation Options

### Basic Compilation
```bash
gcc -o calendar.exe calendar_win32.c -mwindows -lcomctl32 -lgdi32
```

### With Optimization
```bash
gcc -O2 -o calendar.exe calendar_win32.c -mwindows -lcomctl32 -lgdi32
```

### Debug Build
```bash
gcc -g -o calendar_debug.exe calendar_win32.c -lcomctl32 -lgdi32
```

### Static Linking (Portable)
```bash
gcc -static -o calendar.exe calendar_win32.c -mwindows -lcomctl32 -lgdi32
```

---

## 📊 Event Priority Levels

| Priority | Icon | Use Case |
|----------|------|----------|
| **Critical** | 🔴 | Urgent, cannot be missed |
| **High** | 🟠 | Important, high priority |
| **Medium** | 🟡 | Normal priority |
| **Low** | 🟢 | Optional, low priority |

---

## 🏷️ Event Categories

| Category | Icon | Description |
|----------|------|-------------|
| **Work** | 💼 | Work-related events |
| **Personal** | 👤 | Personal appointments |
| **Birthday** | 🎂 | Birthday celebrations |
| **Meeting** | 🤝 | Team/client meetings |
| **Appointment** | 📋 | Doctor, dentist, etc. |
| **Reminder** | ⏰ | Task reminders |
| **Holiday** | 🎉 | Holidays and vacations |
| **Other** | 📌 | Miscellaneous events |

---

## 🔄 Recurrence Types

- **None**: One-time event
- **Daily**: Repeats every day
- **Weekly**: Repeats every week
- **Monthly**: Repeats on the same day each month
- **Yearly**: Repeats annually

---

## 🐛 Troubleshooting

### Application Won't Start
- Ensure you have Windows 7 or later
- Try running as Administrator
- Check if `comctl32.dll` is present in System32

### Events Not Saving
- Check write permissions in application directory
- Ensure sufficient disk space
- Try running as Administrator

### Compilation Errors

**Error: `comctl32` not found**
```bash
# Install MinGW-w64 properly or use:
gcc -o calendar.exe calendar_win32.c -mwindows -lcomctl32 -lgdi32 -L/mingw64/lib
```

**Error: Undefined reference to `WinMain`**
```bash
# Add -mwindows flag:
gcc -o calendar.exe calendar_win32.c -mwindows -lcomctl32 -lgdi32
```

### Data File Corrupted
- Delete `calendar.dat` to start fresh
- Application will create new data file on next run
- Consider keeping CSV exports as backups

---

## 💡 Tips & Best Practices

1. **Regular Backups**: Export to CSV periodically
2. **Use Categories**: Organize events by category for better filtering
3. **Set Priorities**: Mark important events as High or Critical
4. **Use Locations**: Add locations for better context
5. **Recurring Events**: Use for regular meetings and appointments
6. **Reminders**: Set reminders for critical events

---

## 🔮 Future Enhancements

Potential features for future versions:
- [ ] Search functionality
- [ ] Event editing capability
- [ ] Custom color themes
- [ ] Week/Month view
- [ ] Print calendar
- [ ] Import from CSV/ICS
- [ ] Multi-user support
- [ ] Cloud sync integration
- [ ] Mobile companion app

---

## 📝 License

This project is licensed under the MIT License - see below for details:

```
MIT License

Copyright (c) 2024 Calendar Manager Pro

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## 🤝 Contributing

Contributions are welcome! To contribute:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## 📧 Support & Contact

- **Issues**: Report bugs via GitHub Issues
- **Questions**: Open a discussion on GitHub
  
---

## 🙏 Acknowledgments

- Windows API documentation by Microsoft
- MinGW-w64 project for GCC Windows support
- Community contributors and testers

---

## 📚 Additional Resources

### Useful Links
- [Win32 API Documentation](https://docs.microsoft.com/en-us/windows/win32/)
- [MinGW-w64 Downloads](https://www.mingw-w64.org/)
- [C Programming Guide](https://en.cppreference.com/w/c)

### Related Projects
- [LibreOffice Calc](https://www.libreoffice.org/) - For viewing exported CSVs
- [Windows Calendar](https://www.microsoft.com/en-us/windows) - Native Windows calendar

---


## ⭐ Star History

If you find this project useful, please consider giving it a star on GitHub!

---
