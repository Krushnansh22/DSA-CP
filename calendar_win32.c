#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma comment(lib, "comctl32.lib")

// ========== CONSTANTS ==========
#define MAX_DESC 200
#define MAX_LOC 100
#define DATA_FILE "calendar.dat"

#define ID_CALENDAR 1001
#define ID_ADD_BTN 1002
#define ID_DELETE_BTN 1003
#define ID_EDIT_BTN 1004
#define ID_LIST 1005
#define ID_VIEW_ALL 1006
#define ID_VIEW_TODAY 1007
#define ID_EXPORT 1008
#define ID_STATS 1009
#define ID_TAB 1010

// Dialog controls
#define IDC_DESC 2001
#define IDC_LOC 2002
#define IDC_HOUR 2003
#define IDC_MIN 2004
#define IDC_END_HOUR 2005
#define IDC_END_MIN 2006
#define IDC_PRIORITY 2007
#define IDC_CATEGORY 2008
#define IDC_ALL_DAY 2009
#define IDC_RECUR 2010
#define IDC_REMINDER 2011
#define IDC_REMINDER_MIN 2012
#define IDC_SAVE 2013
#define IDC_CANCEL 2014

// ========== DATA STRUCTURES ==========
typedef enum {
    PRIORITY_LOW = 0, PRIORITY_MEDIUM, PRIORITY_HIGH, PRIORITY_CRITICAL
} Priority;

typedef enum {
    CAT_WORK, CAT_PERSONAL, CAT_BIRTHDAY, CAT_MEETING,
    CAT_APPOINTMENT, CAT_REMINDER, CAT_HOLIDAY, CAT_OTHER
} Category;

typedef enum {
    RECUR_NONE, RECUR_DAILY, RECUR_WEEKLY, RECUR_MONTHLY, RECUR_YEARLY
} RecurrenceType;

typedef struct {
    int day, month, year;
} Date;

typedef struct {
    int hour, minute;
} Time;

typedef struct Event {
    int id;
    Date date;
    Time start_time, end_time;
    char description[MAX_DESC];
    char location[MAX_LOC];
    Priority priority;
    Category category;
    int is_all_day;
    RecurrenceType recurrence;
    int reminder_minutes;
    int deleted;
    struct Event *next;
} Event;

// ========== GLOBAL VARIABLES ==========
Event *event_list = NULL;
int next_id = 1;
HWND hwndMain, hwndCalendar, hwndListView, hwndStatus, hwndTab;
HWND hwndAddDialog = NULL;
HINSTANCE hInst;
Date g_selected_date;

// ========== UTILITY FUNCTIONS ==========
int is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int days_in_month(int month, int year) {
    int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (month == 2 && is_leap_year(year)) return 29;
    return days[month-1];
}

int compare_dates(Date d1, Date d2) {
    if (d1.year != d2.year) return d1.year - d2.year;
    if (d1.month != d2.month) return d1.month - d2.month;
    return d1.day - d2.day;
}

void get_today(Date *d) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    d->day = st.wDay;
    d->month = st.wMonth;
    d->year = st.wYear;
}

const char* priority_to_string(Priority p) {
    switch(p) {
        case PRIORITY_LOW: return "Low";
        case PRIORITY_MEDIUM: return "Medium";
        case PRIORITY_HIGH: return "High";
        case PRIORITY_CRITICAL: return "Critical";
        default: return "Unknown";
    }
}

const char* category_to_string(Category c) {
    const char *cats[] = {"Work", "Personal", "Birthday", "Meeting",
                          "Appointment", "Reminder", "Holiday", "Other"};
    return cats[c];
}

const char* recurrence_to_string(RecurrenceType r) {
    switch(r) {
        case RECUR_NONE: return "None";
        case RECUR_DAILY: return "Daily";
        case RECUR_WEEKLY: return "Weekly";
        case RECUR_MONTHLY: return "Monthly";
        case RECUR_YEARLY: return "Yearly";
        default: return "None";
    }
}

// ========== EVENT MANAGEMENT ==========
Event* create_event(Date date, Time start, Time end, const char *desc,
                   const char *loc, Priority pri, Category cat,
                   int all_day, RecurrenceType rec, int reminder) {
    Event *e = (Event*)malloc(sizeof(Event));
    if (!e) return NULL;
    
    e->id = next_id++;
    e->date = date;
    e->start_time = start;
    e->end_time = end;
    strncpy(e->description, desc, MAX_DESC-1);
    e->description[MAX_DESC-1] = '\0';
    strncpy(e->location, loc, MAX_LOC-1);
    e->location[MAX_LOC-1] = '\0';
    e->priority = pri;
    e->category = cat;
    e->is_all_day = all_day;
    e->recurrence = rec;
    e->reminder_minutes = reminder;
    e->deleted = 0;
    e->next = NULL;
    
    return e;
}

void add_event_to_list(Event *e) {
    if (!event_list) {
        event_list = e;
        return;
    }
    Event *curr = event_list;
    while (curr->next) curr = curr->next;
    curr->next = e;
}

Event* find_event_by_id(int id) {
    Event *e = event_list;
    while (e) {
        if (e->id == id && !e->deleted) return e;
        e = e->next;
    }
    return NULL;
}

void delete_event(int id) {
    Event *e = find_event_by_id(id);
    if (e) e->deleted = 1;
}

// ========== FILE OPERATIONS ==========
void save_events() {
    FILE *fp = fopen(DATA_FILE, "wb");
    if (!fp) return;
    
    int magic = 0xCAFEBABE;
    fwrite(&magic, sizeof(int), 1, fp);
    fwrite(&next_id, sizeof(int), 1, fp);
    
    int count = 0;
    Event *e = event_list;
    while (e) {
        if (!e->deleted) count++;
        e = e->next;
    }
    fwrite(&count, sizeof(int), 1, fp);
    
    e = event_list;
    while (e) {
        if (!e->deleted) {
            fwrite(e, sizeof(Event) - sizeof(Event*), 1, fp);
        }
        e = e->next;
    }
    fclose(fp);
}

void load_events() {
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) return;
    
    int magic, count;
    fread(&magic, sizeof(int), 1, fp);
    if (magic != 0xCAFEBABE) {
        fclose(fp);
        return;
    }
    
    fread(&next_id, sizeof(int), 1, fp);
    fread(&count, sizeof(int), 1, fp);
    
    for (int i = 0; i < count; i++) {
        Event *e = (Event*)malloc(sizeof(Event));
        if (!e) break;
        fread(e, sizeof(Event) - sizeof(Event*), 1, fp);
        e->next = NULL;
        add_event_to_list(e);
    }
    fclose(fp);
}

void export_to_csv(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return;
    
    fprintf(fp, "ID,Date,Time,Description,Location,Priority,Category,Recurrence\n");
    
    Event *e = event_list;
    while (e) {
        if (!e->deleted) {
            fprintf(fp, "%d,%02d/%02d/%d,", e->id, e->date.day, e->date.month, e->date.year);
            if (e->is_all_day) {
                fprintf(fp, "All Day,");
            } else {
                fprintf(fp, "%02d:%02d-%02d:%02d,", 
                       e->start_time.hour, e->start_time.minute,
                       e->end_time.hour, e->end_time.minute);
            }
            fprintf(fp, "\"%s\",\"%s\",%s,%s,%s\n",
                   e->description, e->location,
                   priority_to_string(e->priority),
                   category_to_string(e->category),
                   recurrence_to_string(e->recurrence));
        }
        e = e->next;
    }
    fclose(fp);
}

// ========== GUI FUNCTIONS ==========
void update_list_view(Date *filter_date) {
    ListView_DeleteAllItems(hwndListView);
    
    Event *e = event_list;
    int idx = 0;
    int total = 0;
    
    while (e) {
        if (!e->deleted) {
            if (!filter_date || compare_dates(e->date, *filter_date) == 0) {
                LVITEM lvi = {0};
                lvi.mask = LVIF_TEXT | LVIF_PARAM;
                lvi.iItem = idx;
                lvi.lParam = (LPARAM)e->id;
                
                char buffer[50];
                sprintf(buffer, "%d", e->id);
                lvi.pszText = buffer;
                ListView_InsertItem(hwndListView, &lvi);
                
                sprintf(buffer, "%02d/%02d/%d", e->date.day, e->date.month, e->date.year);
                ListView_SetItemText(hwndListView, idx, 1, buffer);
                
                if (e->is_all_day) {
                    strcpy(buffer, "All Day");
                } else {
                    sprintf(buffer, "%02d:%02d-%02d:%02d", 
                           e->start_time.hour, e->start_time.minute,
                           e->end_time.hour, e->end_time.minute);
                }
                ListView_SetItemText(hwndListView, idx, 2, buffer);
                
                ListView_SetItemText(hwndListView, idx, 3, e->description);
                ListView_SetItemText(hwndListView, idx, 4, (char*)priority_to_string(e->priority));
                ListView_SetItemText(hwndListView, idx, 5, (char*)category_to_string(e->category));
                
                idx++;
            }
            total++;
        }
        e = e->next;
    }
    
    char status[100];
    sprintf(status, "Total Events: %d | Showing: %d", total, idx);
    SetWindowText(hwndStatus, status);
}

void mark_calendar_dates() {
    // Note: Windows MonthCalendar doesn't support marking individual days easily
    // This would require MCN_GETDAYSTATE notification handler
}

// ========== ADD EVENT DIALOG PROCEDURE ==========
LRESULT CALLBACK AddEventDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_ALL_DAY: {
                    BOOL checked = IsDlgButtonChecked(hwnd, IDC_ALL_DAY);
                    EnableWindow(GetDlgItem(hwnd, IDC_HOUR), !checked);
                    EnableWindow(GetDlgItem(hwnd, IDC_MIN), !checked);
                    EnableWindow(GetDlgItem(hwnd, IDC_END_HOUR), !checked);
                    EnableWindow(GetDlgItem(hwnd, IDC_END_MIN), !checked);
                    return 0;
                }
                
                case IDC_REMINDER: {
                    BOOL checked = IsDlgButtonChecked(hwnd, IDC_REMINDER);
                    EnableWindow(GetDlgItem(hwnd, IDC_REMINDER_MIN), checked);
                    return 0;
                }
                
                case IDC_SAVE: {
                    char desc[MAX_DESC], loc[MAX_LOC];
                    GetDlgItemText(hwnd, IDC_DESC, desc, MAX_DESC);
                    GetDlgItemText(hwnd, IDC_LOC, loc, MAX_LOC);
                    
                    if (strlen(desc) == 0) {
                        MessageBox(hwnd, "Description cannot be empty!", "Error", MB_OK | MB_ICONERROR);
                        return 0;
                    }
                    
                    Time start = {
                        GetDlgItemInt(hwnd, IDC_HOUR, NULL, FALSE),
                        GetDlgItemInt(hwnd, IDC_MIN, NULL, FALSE)
                    };
                    Time end = {
                        GetDlgItemInt(hwnd, IDC_END_HOUR, NULL, FALSE),
                        GetDlgItemInt(hwnd, IDC_END_MIN, NULL, FALSE)
                    };
                    
                    int all_day = IsDlgButtonChecked(hwnd, IDC_ALL_DAY);
                    Priority pri = (Priority)SendDlgItemMessage(hwnd, IDC_PRIORITY, CB_GETCURSEL, 0, 0);
                    Category cat = (Category)SendDlgItemMessage(hwnd, IDC_CATEGORY, CB_GETCURSEL, 0, 0);
                    RecurrenceType rec = (RecurrenceType)SendDlgItemMessage(hwnd, IDC_RECUR, CB_GETCURSEL, 0, 0);
                    
                    int reminder = 0;
                    if (IsDlgButtonChecked(hwnd, IDC_REMINDER)) {
                        reminder = GetDlgItemInt(hwnd, IDC_REMINDER_MIN, NULL, FALSE);
                    }
                    
                    Event *e = create_event(g_selected_date, start, end, desc, loc, pri, cat, all_day, rec, reminder);
                    if (e) {
                        add_event_to_list(e);
                        save_events();
                        update_list_view(NULL);
                        SetWindowText(hwndStatus, "Event added successfully!");
                        DestroyWindow(hwnd);
                        hwndAddDialog = NULL;
                    }
                    return 0;
                }
                
                case IDC_CANCEL: {
                    DestroyWindow(hwnd);
                    hwndAddDialog = NULL;
                    return 0;
                }
            }
            break;
        }
        
        case WM_CLOSE: {
            DestroyWindow(hwnd);
            hwndAddDialog = NULL;
            return 0;
        }
        
        case WM_DESTROY: {
            hwndAddDialog = NULL;
            return 0;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void show_add_event_dialog(HWND parent) {
    if (hwndAddDialog) {
        SetForegroundWindow(hwndAddDialog);
        return;
    }
    
    // Register dialog window class
    static int registered = 0;
    if (!registered) {
        WNDCLASSEX wc = {0};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = AddEventDlgProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = "AddEventDialog";
        RegisterClassEx(&wc);
        registered = 1;
    }
    
    // Get selected date from calendar
    SYSTEMTIME st;
    MonthCal_GetCurSel(hwndCalendar, &st);
    g_selected_date.day = st.wDay;
    g_selected_date.month = st.wMonth;
    g_selected_date.year = st.wYear;
    
    // Create dialog window
    hwndAddDialog = CreateWindowEx(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        "AddEventDialog",
        "Add New Event",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 580,
        parent, NULL, hInst, NULL
    );
    
    int y = 15;
    
    // Description
    CreateWindow("STATIC", "Description:", WS_CHILD | WS_VISIBLE,
                15, y, 100, 20, hwndAddDialog, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                  125, y, 330, 25, hwndAddDialog, (HMENU)IDC_DESC, hInst, NULL);
    y += 35;
    
    // Location
    CreateWindow("STATIC", "Location:", WS_CHILD | WS_VISIBLE,
                15, y, 100, 20, hwndAddDialog, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                  125, y, 330, 25, hwndAddDialog, (HMENU)IDC_LOC, hInst, NULL);
    y += 40;
    
    // All Day checkbox
    CreateWindow("BUTTON", "All Day Event", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                15, y, 150, 25, hwndAddDialog, (HMENU)IDC_ALL_DAY, hInst, NULL);
    y += 35;
    
    // Start Time
    CreateWindow("STATIC", "Start Time (HH:MM):", WS_CHILD | WS_VISIBLE,
                15, y, 110, 20, hwndAddDialog, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "9", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER,
                  135, y, 50, 25, hwndAddDialog, (HMENU)IDC_HOUR, hInst, NULL);
    CreateWindow("STATIC", ":", WS_CHILD | WS_VISIBLE,
                190, y+3, 10, 20, hwndAddDialog, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "0", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER,
                  205, y, 50, 25, hwndAddDialog, (HMENU)IDC_MIN, hInst, NULL);
    y += 35;
    
    // End Time
    CreateWindow("STATIC", "End Time (HH:MM):", WS_CHILD | WS_VISIBLE,
                15, y, 110, 20, hwndAddDialog, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "10", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER,
                  135, y, 50, 25, hwndAddDialog, (HMENU)IDC_END_HOUR, hInst, NULL);
    CreateWindow("STATIC", ":", WS_CHILD | WS_VISIBLE,
                190, y+3, 10, 20, hwndAddDialog, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "0", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER,
                  205, y, 50, 25, hwndAddDialog, (HMENU)IDC_END_MIN, hInst, NULL);
    y += 40;
    
    // Priority
    CreateWindow("STATIC", "Priority:", WS_CHILD | WS_VISIBLE,
                15, y, 100, 20, hwndAddDialog, NULL, hInst, NULL);
    HWND hwndPriority = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
                                     125, y, 150, 200, hwndAddDialog, (HMENU)IDC_PRIORITY, hInst, NULL);
    SendMessage(hwndPriority, CB_ADDSTRING, 0, (LPARAM)"Low");
    SendMessage(hwndPriority, CB_ADDSTRING, 0, (LPARAM)"Medium");
    SendMessage(hwndPriority, CB_ADDSTRING, 0, (LPARAM)"High");
    SendMessage(hwndPriority, CB_ADDSTRING, 0, (LPARAM)"Critical");
    SendMessage(hwndPriority, CB_SETCURSEL, 1, 0);
    y += 35;
    
    // Category
    CreateWindow("STATIC", "Category:", WS_CHILD | WS_VISIBLE,
                15, y, 100, 20, hwndAddDialog, NULL, hInst, NULL);
    HWND hwndCategory = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
                                     125, y, 150, 200, hwndAddDialog, (HMENU)IDC_CATEGORY, hInst, NULL);
    SendMessage(hwndCategory, CB_ADDSTRING, 0, (LPARAM)"Work");
    SendMessage(hwndCategory, CB_ADDSTRING, 0, (LPARAM)"Personal");
    SendMessage(hwndCategory, CB_ADDSTRING, 0, (LPARAM)"Birthday");
    SendMessage(hwndCategory, CB_ADDSTRING, 0, (LPARAM)"Meeting");
    SendMessage(hwndCategory, CB_ADDSTRING, 0, (LPARAM)"Appointment");
    SendMessage(hwndCategory, CB_ADDSTRING, 0, (LPARAM)"Reminder");
    SendMessage(hwndCategory, CB_ADDSTRING, 0, (LPARAM)"Holiday");
    SendMessage(hwndCategory, CB_ADDSTRING, 0, (LPARAM)"Other");
    SendMessage(hwndCategory, CB_SETCURSEL, 0, 0);
    y += 35;
    
    // Recurrence
    CreateWindow("STATIC", "Recurrence:", WS_CHILD | WS_VISIBLE,
                15, y, 100, 20, hwndAddDialog, NULL, hInst, NULL);
    HWND hwndRecur = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
                                  125, y, 150, 200, hwndAddDialog, (HMENU)IDC_RECUR, hInst, NULL);
    SendMessage(hwndRecur, CB_ADDSTRING, 0, (LPARAM)"None");
    SendMessage(hwndRecur, CB_ADDSTRING, 0, (LPARAM)"Daily");
    SendMessage(hwndRecur, CB_ADDSTRING, 0, (LPARAM)"Weekly");
    SendMessage(hwndRecur, CB_ADDSTRING, 0, (LPARAM)"Monthly");
    SendMessage(hwndRecur, CB_ADDSTRING, 0, (LPARAM)"Yearly");
    SendMessage(hwndRecur, CB_SETCURSEL, 0, 0);
    y += 40;
    
    // Reminder
    CreateWindow("BUTTON", "Set Reminder", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                15, y, 120, 25, hwndAddDialog, (HMENU)IDC_REMINDER, hInst, NULL);
    HWND hwndReminderMin = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "15", 
                                          WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER,
                                          145, y, 60, 25, hwndAddDialog, (HMENU)IDC_REMINDER_MIN, hInst, NULL);
    EnableWindow(hwndReminderMin, FALSE);
    CreateWindow("STATIC", "minutes before", WS_CHILD | WS_VISIBLE,
                210, y+3, 100, 20, hwndAddDialog, NULL, hInst, NULL);
    y += 50;
    
    // Buttons
    CreateWindow("BUTTON", "Save Event", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
                100, y, 120, 35, hwndAddDialog, (HMENU)IDC_SAVE, hInst, NULL);
    CreateWindow("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                240, y, 120, 35, hwndAddDialog, (HMENU)IDC_CANCEL, hInst, NULL);
    
    // Set focus to description field
    SetFocus(GetDlgItem(hwndAddDialog, IDC_DESC));
}

// ========== MAIN WINDOW PROCEDURE ==========
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Create calendar control
            hwndCalendar = CreateWindowEx(0, MONTHCAL_CLASS, "",
                                         WS_CHILD | WS_VISIBLE | WS_BORDER,
                                         20, 20, 300, 300,
                                         hwnd, (HMENU)ID_CALENDAR, hInst, NULL);
            
            // Create list view
            hwndListView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
                                         WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                                         340, 20, 820, 520,
                                         hwnd, (HMENU)ID_LIST, hInst, NULL);
            
            // Setup list view columns
            LVCOLUMN lvc = {0};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            
            lvc.pszText = "ID";
            lvc.cx = 50;
            ListView_InsertColumn(hwndListView, 0, &lvc);
            
            lvc.pszText = "Date";
            lvc.cx = 100;
            ListView_InsertColumn(hwndListView, 1, &lvc);
            
            lvc.pszText = "Time";
            lvc.cx = 120;
            ListView_InsertColumn(hwndListView, 2, &lvc);
            
            lvc.pszText = "Description";
            lvc.cx = 280;
            ListView_InsertColumn(hwndListView, 3, &lvc);
            
            lvc.pszText = "Priority";
            lvc.cx = 80;
            ListView_InsertColumn(hwndListView, 4, &lvc);
            
            lvc.pszText = "Category";
            lvc.cx = 100;
            ListView_InsertColumn(hwndListView, 5, &lvc);
            
            // Set extended list view styles
            ListView_SetExtendedListViewStyle(hwndListView, 
                LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            
            // Create buttons
            CreateWindow("BUTTON", "Add Event", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        20, 340, 140, 35, hwnd, (HMENU)ID_ADD_BTN, hInst, NULL);
            
            CreateWindow("BUTTON", "Delete Event", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        170, 340, 140, 35, hwnd, (HMENU)ID_DELETE_BTN, hInst, NULL);
            
            CreateWindow("BUTTON", "View Today", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        20, 385, 140, 35, hwnd, (HMENU)ID_VIEW_TODAY, hInst, NULL);
            
            CreateWindow("BUTTON", "View All", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        170, 385, 140, 35, hwnd, (HMENU)ID_VIEW_ALL, hInst, NULL);
            
            CreateWindow("BUTTON", "Export CSV", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        20, 430, 140, 35, hwnd, (HMENU)ID_EXPORT, hInst, NULL);
            
            CreateWindow("BUTTON", "Statistics", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                        170, 430, 140, 35, hwnd, (HMENU)ID_STATS, hInst, NULL);
            
            // Create status bar
            hwndStatus = CreateWindowEx(0, STATUSCLASSNAME, "Ready - Calendar Manager Pro",
                                       WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                                       0, 0, 0, 0,
                                       hwnd, NULL, hInst, NULL);
            
            // Load events
            load_events();
            update_list_view(NULL);
            
            return 0;
        }
        
        case WM_NOTIFY: {
            LPNMHDR nmhdr = (LPNMHDR)lParam;
            
            if (nmhdr->idFrom == ID_CALENDAR && nmhdr->code == MCN_SELECT) {
                LPNMSELCHANGE pSelChange = (LPNMSELCHANGE)lParam;
                Date selected = {
                    pSelChange->stSelStart.wDay,
                    pSelChange->stSelStart.wMonth,
                    pSelChange->stSelStart.wYear
                };
                update_list_view(&selected);
                
                char status[100];
                sprintf(status, "Showing events for %02d/%02d/%d", 
                       selected.day, selected.month, selected.year);
                SetWindowText(hwndStatus, status);
            }
            
            return 0;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_ADD_BTN: {
                    show_add_event_dialog(hwnd);
                    break;
                }
                
                case ID_DELETE_BTN: {
                    int idx = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
                    if (idx != -1) {
                        LVITEM lvi = {0};
                        lvi.mask = LVIF_PARAM;
                        lvi.iItem = idx;
                        ListView_GetItem(hwndListView, &lvi);
                        
                        int id = (int)lvi.lParam;
                        Event *e = find_event_by_id(id);
                        
                        if (e) {
                            char msg[400];
                            sprintf(msg, "Are you sure you want to delete this event?\n\n"
                                        "Description: %s\nDate: %02d/%02d/%d\n"
                                        "Priority: %s\nCategory: %s",
                                   e->description, e->date.day, e->date.month, e->date.year,
                                   priority_to_string(e->priority),
                                   category_to_string(e->category));
                            
                            int result = MessageBox(hwnd, msg, "Confirm Delete",
                                                   MB_YESNO | MB_ICONQUESTION);
                            
                            if (result == IDYES) {
                                delete_event(id);
                                save_events();
                                update_list_view(NULL);
                                SetWindowText(hwndStatus, "Event deleted successfully!");
                            }
                        }
                    } else {
                        MessageBox(hwnd, "Please select an event from the list to delete.",
                                 "No Selection", MB_OK | MB_ICONINFORMATION);
                    }
                    break;
                }
                
                case ID_VIEW_TODAY: {
                    Date today;
                    get_today(&today);
                    
                    SYSTEMTIME st = {0};
                    st.wYear = today.year;
                    st.wMonth = today.month;
                    st.wDay = today.day;
                    MonthCal_SetCurSel(hwndCalendar, &st);
                    
                    update_list_view(&today);
                    SetWindowText(hwndStatus, "Showing today's events");
                    break;
                }
                
                case ID_VIEW_ALL: {
                    update_list_view(NULL);
                    SetWindowText(hwndStatus, "Showing all events");
                    break;
                }
                
                case ID_EXPORT: {
                    char filename[MAX_PATH] = "events.csv";
                    OPENFILENAME ofn = {0};
                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFilter = "CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0";
                    ofn.lpstrFile = filename;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
                    ofn.lpstrDefExt = "csv";
                    ofn.lpstrTitle = "Export Events to CSV";
                    
                    if (GetSaveFileName(&ofn)) {
                        export_to_csv(filename);
                        char msg[500];
                        sprintf(msg, "Events successfully exported to:\n%s\n\n"
                                    "You can now open this file in Excel or any spreadsheet application.",
                               filename);
                        MessageBox(hwnd, msg, "Export Successful", MB_OK | MB_ICONINFORMATION);
                        
                        char status[400];
                        sprintf(status, "Exported to: %s", filename);
                        SetWindowText(hwndStatus, status);
                    }
                    break;
                }
                
                case ID_STATS: {
                    // Count statistics
                    int total = 0, priorities[4] = {0}, categories[8] = {0};
                    int all_day = 0, recurring = 0, with_reminder = 0;
                    Event *e = event_list;
                    while (e) {
                        if (!e->deleted) {
                            total++;
                            priorities[e->priority]++;
                            categories[e->category]++;
                            if (e->is_all_day) all_day++;
                            if (e->recurrence != RECUR_NONE) recurring++;
                            if (e->reminder_minutes > 0) with_reminder++;
                        }
                        e = e->next;
                    }
                    
                    char stats[2000];
                    sprintf(stats, 
                           "═══════════════════════════════════════════\n"
                           "        CALENDAR STATISTICS"
                           "═══════════════════════════════════════════\n\n"
                           "   OVERVIEW:\n"
                           "   Total Events: %d\n"
                           "   All-Day Events: %d\n"
                           "   Recurring Events: %d\n"
                           "   Events with Reminders: %d\n\n"
                           "PRIORITY DISTRIBUTION:\n"
                           "   Critical: %d\n"
                           "   High: %d\n"
                           "   Medium: %d\n"
                           "   Low: %d\n\n"
                           "CATEGORY DISTRIBUTION:\n"
                           "   Work: %d\n"
                           "   Personal: %d\n"
                           "   Birthday: %d\n"
                           "   Meeting: %d\n"
                           "   Appointment: %d\n"
                           "   Reminder: %d\n"
                           "   Holiday: %d\n"
                           "   Other: %d\n"
                           "═══════════════════════════════════════════",
                           total, all_day, recurring, with_reminder,
                           priorities[PRIORITY_CRITICAL],
                           priorities[PRIORITY_HIGH],
                           priorities[PRIORITY_MEDIUM],
                           priorities[PRIORITY_LOW],
                           categories[CAT_WORK],
                           categories[CAT_PERSONAL],
                           categories[CAT_BIRTHDAY],
                           categories[CAT_MEETING],
                           categories[CAT_APPOINTMENT],
                           categories[CAT_REMINDER],
                           categories[CAT_HOLIDAY],
                           categories[CAT_OTHER]);
                    
                    MessageBox(hwnd, stats, "Calendar Statistics", MB_OK | MB_ICONINFORMATION);
                    break;
                }
            }
            return 0;
        }
        
        case WM_SIZE: {
            // Resize status bar
            SendMessage(hwndStatus, WM_SIZE, 0, 0);
            return 0;
        }
        
        case WM_CLOSE: {
            int result = MessageBox(hwnd, 
                                   "Are you sure you want to exit?\n\n"
                                   "All events will be saved automatically.",
                                   "Exit Calendar Manager",
                                   MB_YESNO | MB_ICONQUESTION);
            if (result == IDYES) {
                DestroyWindow(hwnd);
            }
            return 0;
        }
        
        case WM_DESTROY: {
            save_events();
            
            // Free memory
            Event *e = event_list;
            while (e) {
                Event *next = e->next;
                free(e);
                e = next;
            }
            
            PostQuitMessage(0);
            return 0;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ========== MAIN FUNCTION ==========
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_DATE_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Register main window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "CalendarManager";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONERROR | MB_OK);
        return 0;
    }
    
    // Create main window
    hwndMain = CreateWindowEx(
        0,
        "CalendarManager",
        "  Calendar Manager Pro - Windows Edition",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 650,
        NULL, NULL, hInstance, NULL
    );
    
    if (!hwndMain) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONERROR | MB_OK);
        return 0;
    }
    
    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);
    
    // Show welcome message
    char welcome[500];
    Date today;
    get_today(&today);
    
    int event_count = 0;
    Event *e = event_list;
    while (e) {
        if (!e->deleted) event_count++;
        e = e->next;
    }
    
    sprintf(welcome, 
           "Welcome to Calendar Manager Pro!\n\n"
           "Today: %02d/%02d/%d\n"
           "Total Events: %d\n\n"
           "Features:\n"
           "• Add, edit, and delete events\n"
           "• Set priorities and categories\n"
           "• Recurring events support\n"
           "• Export to CSV\n"
           "• Automatic save\n\n"
           "Click on calendar dates to filter events!\n"
           "All changes are saved automatically.",
           today.day, today.month, today.year, event_count);
    
    MessageBox(hwndMain, welcome, "Welcome!", MB_OK | MB_ICONINFORMATION);
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}