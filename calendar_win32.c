// calendar_win32.c - Calendar Application with Windows GUI
// Compile: gcc -o calendar.exe calendar_win32.c -mwindows -lcomctl32 -lgdi32

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
HINSTANCE hInst;

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
    
    fprintf(fp, "ID,Date,Time,Description,Location,Priority,Category\n");
    
    Event *e = event_list;
    while (e) {
        if (!e->deleted) {
            fprintf(fp, "%d,%02d/%02d/%d,%02d:%02d,\"%s\",\"%s\",%s,%s\n",
                   e->id, e->date.day, e->date.month, e->date.year,
                   e->start_time.hour, e->start_time.minute,
                   e->description, e->location,
                   priority_to_string(e->priority),
                   category_to_string(e->category));
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
    SYSTEMTIME st;
    MonthCal_GetCurSel(hwndCalendar, &st);
    
    Event *e = event_list;
    while (e) {
        if (!e->deleted && e->date.year == st.wYear && e->date.month == st.wMonth) {
            SYSTEMTIME bold_st = {0};
            bold_st.wYear = e->date.year;
            bold_st.wMonth = e->date.month;
            bold_st.wDay = e->date.day;
            MonthCal_SetDayState(hwndCalendar, 1, &bold_st);
        }
        e = e->next;
    }
}

// ========== ADD EVENT DIALOG ==========
INT_PTR CALLBACK AddEventDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static Date selected_date;
    
    switch (message) {
        case WM_INITDIALOG: {
            selected_date = *(Date*)lParam;
            
            // Set window title
            SetWindowText(hwndDlg, "Add New Event");
            
            // Set default values
            SetDlgItemInt(hwndDlg, IDC_HOUR, 9, FALSE);
            SetDlgItemInt(hwndDlg, IDC_MIN, 0, FALSE);
            SetDlgItemInt(hwndDlg, IDC_END_HOUR, 10, FALSE);
            SetDlgItemInt(hwndDlg, IDC_END_MIN, 0, FALSE);
            SetDlgItemInt(hwndDlg, IDC_REMINDER_MIN, 15, FALSE);
            
            // Populate combo boxes
            SendDlgItemMessage(hwndDlg, IDC_PRIORITY, CB_ADDSTRING, 0, (LPARAM)"Low");
            SendDlgItemMessage(hwndDlg, IDC_PRIORITY, CB_ADDSTRING, 0, (LPARAM)"Medium");
            SendDlgItemMessage(hwndDlg, IDC_PRIORITY, CB_ADDSTRING, 0, (LPARAM)"High");
            SendDlgItemMessage(hwndDlg, IDC_PRIORITY, CB_ADDSTRING, 0, (LPARAM)"Critical");
            SendDlgItemMessage(hwndDlg, IDC_PRIORITY, CB_SETCURSEL, 1, 0);
            
            SendDlgItemMessage(hwndDlg, IDC_CATEGORY, CB_ADDSTRING, 0, (LPARAM)"Work");
            SendDlgItemMessage(hwndDlg, IDC_CATEGORY, CB_ADDSTRING, 0, (LPARAM)"Personal");
            SendDlgItemMessage(hwndDlg, IDC_CATEGORY, CB_ADDSTRING, 0, (LPARAM)"Birthday");
            SendDlgItemMessage(hwndDlg, IDC_CATEGORY, CB_ADDSTRING, 0, (LPARAM)"Meeting");
            SendDlgItemMessage(hwndDlg, IDC_CATEGORY, CB_ADDSTRING, 0, (LPARAM)"Appointment");
            SendDlgItemMessage(hwndDlg, IDC_CATEGORY, CB_ADDSTRING, 0, (LPARAM)"Reminder");
            SendDlgItemMessage(hwndDlg, IDC_CATEGORY, CB_ADDSTRING, 0, (LPARAM)"Holiday");
            SendDlgItemMessage(hwndDlg, IDC_CATEGORY, CB_ADDSTRING, 0, (LPARAM)"Other");
            SendDlgItemMessage(hwndDlg, IDC_CATEGORY, CB_SETCURSEL, 0, 0);
            
            SendDlgItemMessage(hwndDlg, IDC_RECUR, CB_ADDSTRING, 0, (LPARAM)"None");
            SendDlgItemMessage(hwndDlg, IDC_RECUR, CB_ADDSTRING, 0, (LPARAM)"Daily");
            SendDlgItemMessage(hwndDlg, IDC_RECUR, CB_ADDSTRING, 0, (LPARAM)"Weekly");
            SendDlgItemMessage(hwndDlg, IDC_RECUR, CB_ADDSTRING, 0, (LPARAM)"Monthly");
            SendDlgItemMessage(hwndDlg, IDC_RECUR, CB_ADDSTRING, 0, (LPARAM)"Yearly");
            SendDlgItemMessage(hwndDlg, IDC_RECUR, CB_SETCURSEL, 0, 0);
            
            return TRUE;
        }
        
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_ALL_DAY: {
                    BOOL enabled = !IsDlgButtonChecked(hwndDlg, IDC_ALL_DAY);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_HOUR), enabled);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_MIN), enabled);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_END_HOUR), enabled);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_END_MIN), enabled);
                    return TRUE;
                }
                
                case IDC_REMINDER: {
                    BOOL enabled = IsDlgButtonChecked(hwndDlg, IDC_REMINDER);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_REMINDER_MIN), enabled);
                    return TRUE;
                }
                
                case IDC_SAVE: {
                    char desc[MAX_DESC], loc[MAX_LOC];
                    GetDlgItemText(hwndDlg, IDC_DESC, desc, MAX_DESC);
                    GetDlgItemText(hwndDlg, IDC_LOC, loc, MAX_LOC);
                    
                    if (strlen(desc) == 0) {
                        MessageBox(hwndDlg, "Description cannot be empty!", "Error", MB_OK | MB_ICONERROR);
                        return TRUE;
                    }
                    
                    Time start = {
                        GetDlgItemInt(hwndDlg, IDC_HOUR, NULL, FALSE),
                        GetDlgItemInt(hwndDlg, IDC_MIN, NULL, FALSE)
                    };
                    Time end = {
                        GetDlgItemInt(hwndDlg, IDC_END_HOUR, NULL, FALSE),
                        GetDlgItemInt(hwndDlg, IDC_END_MIN, NULL, FALSE)
                    };
                    
                    int all_day = IsDlgButtonChecked(hwndDlg, IDC_ALL_DAY);
                    Priority pri = (Priority)SendDlgItemMessage(hwndDlg, IDC_PRIORITY, CB_GETCURSEL, 0, 0);
                    Category cat = (Category)SendDlgItemMessage(hwndDlg, IDC_CATEGORY, CB_GETCURSEL, 0, 0);
                    RecurrenceType rec = (RecurrenceType)SendDlgItemMessage(hwndDlg, IDC_RECUR, CB_GETCURSEL, 0, 0);
                    
                    int reminder = 0;
                    if (IsDlgButtonChecked(hwndDlg, IDC_REMINDER)) {
                        reminder = GetDlgItemInt(hwndDlg, IDC_REMINDER_MIN, NULL, FALSE);
                    }
                    
                    Event *e = create_event(selected_date, start, end, desc, loc, pri, cat, all_day, rec, reminder);
                    if (e) {
                        add_event_to_list(e);
                        save_events();
                        EndDialog(hwndDlg, IDOK);
                    }
                    return TRUE;
                }
                
                case IDC_CANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

void show_add_event_dialog(HWND hwnd, Date *date) {
    // Create dialog dynamically
    HWND hwndDlg = CreateWindowEx(0, "#32770", "Add Event",
                                  WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 450, 520,
                                  hwnd, NULL, hInst, NULL);
    
    int y = 10;
    
    // Description
    CreateWindow("STATIC", "Description:", WS_CHILD | WS_VISIBLE,
                10, y, 100, 20, hwndDlg, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                  120, y, 300, 25, hwndDlg, (HMENU)IDC_DESC, hInst, NULL);
    y += 35;
    
    // Location
    CreateWindow("STATIC", "Location:", WS_CHILD | WS_VISIBLE,
                10, y, 100, 20, hwndDlg, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                  120, y, 300, 25, hwndDlg, (HMENU)IDC_LOC, hInst, NULL);
    y += 35;
    
    // All Day
    CreateWindow("BUTTON", "All Day Event", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                10, y, 150, 25, hwndDlg, (HMENU)IDC_ALL_DAY, hInst, NULL);
    y += 35;
    
    // Start Time
    CreateWindow("STATIC", "Start Time (HH:MM):", WS_CHILD | WS_VISIBLE,
                10, y, 110, 20, hwndDlg, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "9", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                  130, y, 50, 25, hwndDlg, (HMENU)IDC_HOUR, hInst, NULL);
    CreateWindow("STATIC", ":", WS_CHILD | WS_VISIBLE,
                185, y, 10, 20, hwndDlg, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                  200, y, 50, 25, hwndDlg, (HMENU)IDC_MIN, hInst, NULL);
    y += 35;
    
    // End Time
    CreateWindow("STATIC", "End Time (HH:MM):", WS_CHILD | WS_VISIBLE,
                10, y, 110, 20, hwndDlg, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "10", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                  130, y, 50, 25, hwndDlg, (HMENU)IDC_END_HOUR, hInst, NULL);
    CreateWindow("STATIC", ":", WS_CHILD | WS_VISIBLE,
                185, y, 10, 20, hwndDlg, NULL, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                  200, y, 50, 25, hwndDlg, (HMENU)IDC_END_MIN, hInst, NULL);
    y += 35;
    
    // Priority
    CreateWindow("STATIC", "Priority:", WS_CHILD | WS_VISIBLE,
                10, y, 100, 20, hwndDlg, NULL, hInst, NULL);
    HWND hwndPriority = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                     120, y, 150, 200, hwndDlg, (HMENU)IDC_PRIORITY, hInst, NULL);
    SendMessage(hwndPriority, CB_ADDSTRING, 0, (LPARAM)"Low");
    SendMessage(hwndPriority, CB_ADDSTRING, 0, (LPARAM)"Medium");
    SendMessage(hwndPriority, CB_ADDSTRING, 0, (LPARAM)"High");
    SendMessage(hwndPriority, CB_ADDSTRING, 0, (LPARAM)"Critical");
    SendMessage(hwndPriority, CB_SETCURSEL, 1, 0);
    y += 35;
    
    // Category
    CreateWindow("STATIC", "Category:", WS_CHILD | WS_VISIBLE,
                10, y, 100, 20, hwndDlg, NULL, hInst, NULL);
    HWND hwndCategory = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                     120, y, 150, 200, hwndDlg, (HMENU)IDC_CATEGORY, hInst, NULL);
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
                10, y, 100, 20, hwndDlg, NULL, hInst, NULL);
    HWND hwndRecur = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                  120, y, 150, 200, hwndDlg, (HMENU)IDC_RECUR, hInst, NULL);
    SendMessage(hwndRecur, CB_ADDSTRING, 0, (LPARAM)"None");
    SendMessage(hwndRecur, CB_ADDSTRING, 0, (LPARAM)"Daily");
    SendMessage(hwndRecur, CB_ADDSTRING, 0, (LPARAM)"Weekly");
    SendMessage(hwndRecur, CB_ADDSTRING, 0, (LPARAM)"Monthly");
    SendMessage(hwndRecur, CB_ADDSTRING, 0, (LPARAM)"Yearly");
    SendMessage(hwndRecur, CB_SETCURSEL, 0, 0);
    y += 35;
    
    // Reminder
    CreateWindow("BUTTON", "Set Reminder (minutes before):", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                10, y, 200, 25, hwndDlg, (HMENU)IDC_REMINDER, hInst, NULL);
    CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "15", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                  220, y, 60, 25, hwndDlg, (HMENU)IDC_REMINDER_MIN, hInst, NULL);
    EnableWindow(GetDlgItem(hwndDlg, IDC_REMINDER_MIN), FALSE);
    y += 40;
    
    // Buttons
    CreateWindow("BUTTON", "Save", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                150, y, 80, 30, hwndDlg, (HMENU)IDC_SAVE, hInst, NULL);
    CreateWindow("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                240, y, 80, 30, hwndDlg, (HMENU)IDC_CANCEL, hInst, NULL);
    
    ShowWindow(hwndDlg, SW_SHOW);
    
    // Message loop for dialog
    MSG msg;
    BOOL ret;
    while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
        if (ret == -1) {
            break;
        } else if (!IsWindow(hwndDlg) || !IsDialogMessage(hwndDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (!IsWindow(hwndDlg)) break;
    }
}

// Continued in message handler...

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Create tab control
            hwndTab = CreateWindowEx(0, WC_TABCONTROL, "",
                                    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                    10, 10, 1160, 600,
                                    hwnd, (HMENU)ID_TAB, hInst, NULL);
            
            // Add tabs
            TCITEM tie = {0};
            tie.mask = TCIF_TEXT;
            tie.pszText = "Calendar";
            TabCtrl_InsertItem(hwndTab, 0, &tie);
            tie.pszText = "All Events";
            TabCtrl_InsertItem(hwndTab, 1, &tie);
            
            // Create calendar control
            hwndCalendar = CreateWindowEx(0, MONTHCAL_CLASS, "",
                                         WS_CHILD | WS_VISIBLE | WS_BORDER | MCS_DAYSTATE,
                                         30, 60, 300, 300,
                                         hwnd, (HMENU)ID_CALENDAR, hInst, NULL);
            
            // Create list view
            hwndListView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
                                         WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                                         360, 60, 790, 520,
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
            lvc.cx = 250;
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
            CreateWindow("BUTTON", "Add Event", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        30, 380, 120, 35, hwnd, (HMENU)ID_ADD_BTN, hInst, NULL);
            
            CreateWindow("BUTTON", "Delete Event", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        160, 380, 120, 35, hwnd, (HMENU)ID_DELETE_BTN, hInst, NULL);
            
            CreateWindow("BUTTON", "View Today", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        30, 425, 120, 35, hwnd, (HMENU)ID_VIEW_TODAY, hInst, NULL);
            
            CreateWindow("BUTTON", "View All", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        160, 425, 120, 35, hwnd, (HMENU)ID_VIEW_ALL, hInst, NULL);
            
            CreateWindow("BUTTON", "Export CSV", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        30, 470, 120, 35, hwnd, (HMENU)ID_EXPORT, hInst, NULL);
            
            CreateWindow("BUTTON", "Statistics", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        160, 470, 120, 35, hwnd, (HMENU)ID_STATS, hInst, NULL);
            
            // Create status bar
            hwndStatus = CreateWindowEx(0, STATUSCLASSNAME, "Ready",
                                       WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                                       0, 0, 0, 0,
                                       hwnd, NULL, hInst, NULL);
            
            // Load events
            load_events();
            update_list_view(NULL);
            mark_calendar_dates();
            
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
            }
            
            if (nmhdr->idFrom == ID_CALENDAR && nmhdr->code == MCN_VIEWCHANGE) {
                mark_calendar_dates();
            }
            
            return 0;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_ADD_BTN: {
                    SYSTEMTIME st;
                    MonthCal_GetCurSel(hwndCalendar, &st);
                    Date selected = {st.wDay, st.wMonth, st.wYear};
                    
                    show_add_event_dialog(hwnd, &selected);
                    update_list_view(NULL);
                    mark_calendar_dates();
                    SetWindowText(hwndStatus, "Event added successfully!");
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
                            char msg[300];
                            sprintf(msg, "Delete event: %s?", e->description);
                            int result = MessageBox(hwnd, msg, "Confirm Delete",
                                                   MB_YESNO | MB_ICONQUESTION);
                            
                            if (result == IDYES) {
                                delete_event(id);
                                save_events();
                                update_list_view(NULL);
                                mark_calendar_dates();
                                SetWindowText(hwndStatus, "Event deleted successfully!");
                            }
                        }
                    } else {
                        MessageBox(hwnd, "Please select an event to delete",
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
                    ofn.Flags = OFN_OVERWRITEPROMPT;
                    ofn.lpstrDefExt = "csv";
                    
                    if (GetSaveFileName(&ofn)) {
                        export_to_csv(filename);
                        char msg[400];
                        sprintf(msg, "Events exported to %s", filename);
                        MessageBox(hwnd, msg, "Export Successful", MB_OK | MB_ICONINFORMATION);
                        SetWindowText(hwndStatus, msg);
                    }
                    break;
                }
                
                case ID_STATS: {
                    // Count statistics
                    int total = 0, priorities[4] = {0}, categories[8] = {0};
                    Event *e = event_list;
                    while (e) {
                        if (!e->deleted) {
                            total++;
                            priorities[e->priority]++;
                            categories[e->category]++;
                        }
                        e = e->next;
                    }
                    
                    char stats[1000];
                    sprintf(stats, "CALENDAR STATISTICS\n\n"
                                  "Total Events: %d\n\n"
                                  "PRIORITY DISTRIBUTION:\n"
                                  "  Critical: %d\n"
                                  "  High: %d\n"
                                  "  Medium: %d\n"
                                  "  Low: %d\n\n"
                                  "CATEGORY DISTRIBUTION:\n"
                                  "  Work: %d\n"
                                  "  Personal: %d\n"
                                  "  Birthday: %d\n"
                                  "  Meeting: %d\n"
                                  "  Appointment: %d\n"
                                  "  Reminder: %d\n"
                                  "  Holiday: %d\n"
                                  "  Other: %d\n",
                           total,
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
                
                case IDC_ALL_DAY: {
                    HWND hwndDlg = GetParent((HWND)lParam);
                    BOOL enabled = !IsDlgButtonChecked(hwndDlg, IDC_ALL_DAY);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_HOUR), enabled);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_MIN), enabled);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_END_HOUR), enabled);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_END_MIN), enabled);
                    break;
                }
                
                case IDC_REMINDER: {
                    HWND hwndDlg = GetParent((HWND)lParam);
                    BOOL enabled = IsDlgButtonChecked(hwndDlg, IDC_REMINDER);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_REMINDER_MIN), enabled);
                    break;
                }
                
                case IDC_SAVE: {
                    HWND hwndDlg = GetParent((HWND)lParam);
                    char desc[MAX_DESC], loc[MAX_LOC];
                    GetDlgItemText(hwndDlg, IDC_DESC, desc, MAX_DESC);
                    GetDlgItemText(hwndDlg, IDC_LOC, loc, MAX_LOC);
                    
                    if (strlen(desc) == 0) {
                        MessageBox(hwndDlg, "Description cannot be empty!", "Error", MB_OK | MB_ICONERROR);
                        break;
                    }
                    
                    // Get the date from the dialog's stored data
                    SYSTEMTIME st;
                    MonthCal_GetCurSel(hwndCalendar, &st);
                    Date date = {st.wDay, st.wMonth, st.wYear};
                    
                    Time start = {
                        GetDlgItemInt(hwndDlg, IDC_HOUR, NULL, FALSE),
                        GetDlgItemInt(hwndDlg, IDC_MIN, NULL, FALSE)
                    };
                    Time end = {
                        GetDlgItemInt(hwndDlg, IDC_END_HOUR, NULL, FALSE),
                        GetDlgItemInt(hwndDlg, IDC_END_MIN, NULL, FALSE)
                    };
                    
                    int all_day = IsDlgButtonChecked(hwndDlg, IDC_ALL_DAY);
                    Priority pri = (Priority)SendDlgItemMessage(hwndDlg, IDC_PRIORITY, CB_GETCURSEL, 0, 0);
                    Category cat = (Category)SendDlgItemMessage(hwndDlg, IDC_CATEGORY, CB_GETCURSEL, 0, 0);
                    RecurrenceType rec = (RecurrenceType)SendDlgItemMessage(hwndDlg, IDC_RECUR, CB_GETCURSEL, 0, 0);
                    
                    int reminder = 0;
                    if (IsDlgButtonChecked(hwndDlg, IDC_REMINDER)) {
                        reminder = GetDlgItemInt(hwndDlg, IDC_REMINDER_MIN, NULL, FALSE);
                    }
                    
                    Event *e = create_event(date, start, end, desc, loc, pri, cat, all_day, rec, reminder);
                    if (e) {
                        add_event_to_list(e);
                        save_events();
                        DestroyWindow(hwndDlg);
                    }
                    break;
                }
                
                case IDC_CANCEL: {
                    HWND hwndDlg = GetParent((HWND)lParam);
                    DestroyWindow(hwndDlg);
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_DATE_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Register window class
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
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONERROR);
        return 0;
    }
    
    // Create main window
    hwndMain = CreateWindowEx(0, "CalendarManager",
                             "Calendar Manager Pro - Windows Edition",
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, 1200, 700,
                             NULL, NULL, hInstance, NULL);
    
    if (!hwndMain) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONERROR);
        return 0;
    }
    
    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}